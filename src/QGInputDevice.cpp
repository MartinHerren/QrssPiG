#include "QGInputDevice.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

#include "Config.h"

#include "QGInputStdIn.h"

#ifdef HAVE_LIBALSA
#include "QGInputAlsa.h"
#endif // HAVE_LIBALSA

#ifdef HAVE_LIBHACKRF
#include "QGInputHackRF.h"
#endif // HAVE_LIBHACKRF

#ifdef HAVE_LIBRTLSDR
#include "QGInputRtlSdr.h"
#endif // HAVE_LIBRTLSDR

std::vector<std::string> QGInputDevice::listModules() {
    std::vector<std::string> modules;

    modules.push_back("StdIn");
#ifdef HAVE_LIBALSA
    modules.push_back("Alsa");
#endif //HAVE_LIBALSA
#ifdef HAVE_LIBHACKRF
    modules.push_back("HackRF");
#endif //HAVE_LIBHACKRF
#ifdef HAVE_LIBRTLSDR
    modules.push_back("RtlSdr");
#endif //HAVE_LIBRTLSDR

    return modules;
}

std::vector<std::pair<std::string, std::vector<std::string>>> QGInputDevice::listDevices() {
    std::vector<std::pair<std::string, std::vector<std::string>>> devices;

#ifdef HAVE_LIBALSA
    devices.push_back(std::make_pair("Alsa", QGInputAlsa::listDevices()));
#endif //HAVE_LIBALSA
#ifdef HAVE_LIBHACKRF
    devices.push_back(std::make_pair("HackRF", QGInputHackRF::listDevices()));
#endif //HAVE_LIBHACKRF
#ifdef HAVE_LIBRTLSDR
    devices.push_back(std::make_pair("RtlSdr", QGInputRtlSdr::listDevices()));
#endif //HAVE_LIBRTLSDR

    return devices;
}

QGInputDevice::QGInputDevice(const YAML::Node &config) {
    _sampleRate = 48000;
    _baseFreq = 0;
    _ppm = 0;
    _bufferlength = 1000;
    _debugBufferMonitor = false;

    if (config["samplerate"]) _sampleRate = config["samplerate"].as<unsigned int>();
    if (config["basefreq"]) _baseFreq = config["basefreq"].as<unsigned int>();
    if (config["ppm"]) _ppm = config["ppm"].as<int>();
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

std::unique_ptr<QGInputDevice> QGInputDevice::CreateInputDevice(const YAML::Node &config) {
    if (!config["type"] || (config["type"].as<std::string>().compare("stdin") == 0)) {
        std::cout << "Input type stdin" << std::endl;
        return std::unique_ptr<QGInputDevice>(new QGInputStdIn(config));
    } else if (config["type"].as<std::string>().compare("alsa") == 0) {
#ifdef HAVE_LIBALSA
        std::cout << "Input type alsa" << std::endl;
        return std::unique_ptr<QGInputDevice>(new QGInputAlsa(config));
#else
        throw std::runtime_error(std::string("QGInputDevice: alsa support not builtin into this build"));
#endif //HAVE_LIBALSA
    } else if (config["type"].as<std::string>().compare("hackrf") == 0) {
#ifdef HAVE_LIBHACKRF
        std::cout << "Input type hackrf" << std::endl;
        return std::unique_ptr<QGInputDevice>(new QGInputHackRF(config));
#else
        throw std::runtime_error(std::string("QGInputDevice: hackrf support not builtin into this build"));
#endif //HAVE_LIBHACKRF
    } else if (config["type"].as<std::string>().compare("rtlsdr") == 0) {
#ifdef HAVE_LIBRTLSDR
        std::cout << "Input type rtlsdr" << std::endl;
        return std::unique_ptr<QGInputDevice>(new QGInputRtlSdr(config));
#else
        throw std::runtime_error(std::string("QGInputDevice: rtlsdr support not uiltin into this build"));
#endif //HAVE_LIBRTLSDR
    } else {
        throw std::runtime_error(std::string("QGInputDevice: unknown type ") + config["type"].as<std::string>());
    }
}

void QGInputDevice::_bufferMonitor() {
    while (_running || _bufferSize >= _chunkSize) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        std::cout << (_running ? "[Running]" : "[Stopping]") << "\tBuffer: " << _bufferSize << " / " << _bufferCapacity << " (" << (100.*_bufferSize/_bufferCapacity) << "%)" << std::endl;
    }
    return;
}
