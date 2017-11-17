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

QGInputDevice::QGInputDevice(const YAML::Node &config) {
    _sampleRate = 48000;
    _baseFreq = 0;
    _ppm = 0;

    if (config["samplerate"]) _sampleRate = config["samplerate"].as<unsigned int>();
    if (config["basefreq"]) _baseFreq = config["basefreq"].as<unsigned int>();
    if (config["ppm"]) _ppm = config["ppm"].as<int>();

    _running = false;
}

void QGInputDevice::setCb(std::function<void(const std::complex<float>*)>cb, unsigned int chunkSize) {
    _cb = cb;
    _chunkSize = chunkSize;

    // TODO calculate buffer size frm _sampleRate and configurable size. Multiple of chuncksize
    _bufferCapacity = _sampleRate;
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
    } while (running || (_bufferSize > _chunkSize));
}

void QGInputDevice::stop() {
    // Called from signal handler, only do signal handler safe stuff here !
    _running = false;
}

void QGInputDevice::ListDevices() {
    std::vector<std::string> list;

    std::cout << "Available input devices:" << std::endl;
#ifdef HAVE_LIBALSA
    list = QGInputAlsa::listDevices();
    if (list.size())
	    for (auto &s: list) std::cout << "Alsa:\t" << s << std::endl;
#endif //HAVE_LIBALSA
#ifdef HAVE_LIBHACKRF
    list = QGInputHackRF::listDevices();
    if (list.size())
	    for (auto &s: list) std::cout << "HackRf:\t" << s << std::endl;
#endif //HAVE_LIBHACKRF
#ifdef HAVE_LIBRTLSDR
    list = QGInputRtlSdr::listDevices();
    if (list.size())
	    for (auto &s: list) std::cout << "RtlSdr:\t" << s << std::endl;
#endif //HAVE_LIBRTLSDR
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
