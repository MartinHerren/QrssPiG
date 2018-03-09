#include "QGInputDevice.h"
#include "QGPlugin.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

#include "Config.h"

std::vector<std::string> QGInputDevice::listModules() {
    return (new QGPlugin<QGInputDevice>("input"))->list_plugins();
}

std::vector<std::pair<std::string, std::vector<std::string>>> QGInputDevice::listDevices() {
    std::vector<std::pair<std::string, std::vector<std::string>>> devices;
    for (const auto&module: QGInputDevice::listModules()) {
        QGPlugin<QGInputDevice> *p = new QGPlugin<QGInputDevice>("input");
            if(p->has_list_devices(module)) {
                std::vector<std::string> device_list = p->list_devices(module);
                devices.push_back(std::make_pair(module, device_list));
            }
    }
    return devices;
}

std::unique_ptr<QGInputDevice> QGInputDevice::CreateInputDevice(const YAML::Node &config) {
    if (!config["type"]) throw std::runtime_error("YAML: input must have a type");
    QGPlugin<QGInputDevice>* plugin = new QGPlugin<QGInputDevice>("upload");
    QGInputDevice *obj = plugin->create(config);
    return std::unique_ptr<QGInputDevice>(obj);
}

QGInputDevice::QGInputDevice(const YAML::Node &config) {
    _sampleRate = 48000;
    _baseFreq = 0;
    _residualPpm = 0.;
    _bufferlength = 1000;
    _debugBufferMonitor = false;

    if (config["samplerate"]) _sampleRate = config["samplerate"].as<unsigned int>();
    if (config["basefreq"]) _baseFreq = config["basefreq"].as<unsigned int>();
    if (config["ppm"]) _residualPpm = config["ppm"].as<float>();
    if (config["bufferlength"]) _bufferlength = config["bufferlength"].as<int>();

    if (config["debug"]) {
        if (config["debug"]["buffermonitor"]) _debugBufferMonitor = config["debug"]["buffermonitor"].as<bool>();
    }

    _running = false;
}

void QGInputDevice::setCb(std::function<void(const std::complex<float>*)>cb, unsigned int chunkSize) {
    _cb = cb;
    _chunkSize = chunkSize;

    // buffercapacity based bufferlength in milliseconds, rounded down to multiple of chunksize
    _bufferCapacity = _bufferlength / 1000. * _sampleRate;
    _bufferCapacity = (_bufferCapacity / _chunkSize) * _chunkSize;
    _buffer.resize(_bufferCapacity);
    _bufferSize = 0;
    _bufferHead = 0;
    _bufferTail = 0;
}

void QGInputDevice::run() {
	if (_running) throw std::runtime_error("Already running");

    _running = true;
    bool stopping = false;
    bool running;

    _startDevice();

    std::thread monitor;
    if (_debugBufferMonitor) monitor = std::thread(std::bind(&QGInputDevice::_bufferMonitor, this));

    do {
        running = _running;

        // Detect stop() and shutdown device only once
        if (!running && !stopping) {
            _stopDevice();
            stopping = true;
        }

        if (_bufferSize >= _chunkSize) {
            _cb(&_buffer[_bufferTail]);
            _bufferTail += _chunkSize;
            _bufferTail %= _bufferCapacity;

            // Update size
            _bufferSize -= _chunkSize;
        } else {
	           std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    } while (running || (_bufferSize >= _chunkSize));

    if (_debugBufferMonitor) monitor.join();
}

void QGInputDevice::stop() {
    // Called from signal handler, only do signal handler safe stuff here !
    _running = false;
}

void QGInputDevice::_bufferMonitor() {
    while (_running || _bufferSize >= _chunkSize) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        std::cout << (_running ? "[Running]" : "[Stopping]") << "\tBuffer: " << _bufferSize << " / " << _bufferCapacity << " (" << (100.*_bufferSize/_bufferCapacity) << "%)" << std::endl;
    }
    return;
}
