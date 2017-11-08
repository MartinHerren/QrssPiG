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

QGInputDevice::QGInputDevice(const YAML::Node &config, std::function<void(const std::complex<float>*, unsigned int)>cb) {
    _sampleRate = 48000;
    _baseFreq = 0;
    _ppm = 0;

    if (config["samplerate"]) _sampleRate = config["samplerate"].as<unsigned int>();
    if (config["basefreq"]) _baseFreq = config["basefreq"].as<unsigned int>();
    if (config["ppm"]) _ppm = config["ppm"].as<int>();

    _cb = cb;

// TODO calculate buffer size frm _sampleRate and configurable size. Multiple of chuncksize
	_bufferCapacity = 10 * 32 * 262144; // ~10 seconds of buffer @ 8MS/s
	_buffer.resize(_bufferCapacity);
	_bufferSize = 0;
	_bufferHead = 0;
	_bufferTail = 0;
}

void QGInputDevice::run() {
	if (_running.try_lock()) {
        _startDevice();

//TODO get correct chunksize
        while (!_running.try_lock()) { // loop until lock as been freed by stop()
            if (_bufferSize > 8192*10) {
                for (int j = 0; j < 8192*10; j += 32) {
                    _cb(&_buffer[_bufferTail], 32);
                    _bufferTail += 32;
                    _bufferTail %= _bufferCapacity;
                }

                // Update size
                _bufferMutex.lock();
                _bufferSize -= 8192*10;
                _bufferMutex.unlock();
            } else {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        }
        _running.unlock(); // Cleanup lock acquired on exit condition
    }
}

void QGInputDevice::stop() {
	if (!_running.try_lock()) {
        _bufferMutex.unlock(); // To ensure _stopDevice won't stick
        _stopDevice();
	}

	_running.unlock();// Enable run() to exit, or release the lock we just gained if it was not running
}

void QGInputDevice::_incBuffer(size_t added) {
    _bufferMutex.lock();
    _bufferSize += added;
    _bufferMutex.unlock();
}

std::unique_ptr<QGInputDevice> QGInputDevice::CreateInputDevice(const YAML::Node &config, std::function<void(const std::complex<float>*, unsigned int)>cb) {
    if (!config["type"] || (config["type"].as<std::string>().compare("stdin") == 0)) {
        std::cout << "Input type stdin" << std::endl;
        return std::unique_ptr<QGInputDevice>(new QGInputStdIn(config, cb));
    } else if (config["type"].as<std::string>().compare("alsa") == 0) {
#ifdef HAVE_LIBALSA
        std::cout << "Input type alsa" << std::endl;
        return std::unique_ptr<QGInputDevice>(new QGInputAlsa(config, cb));
#else
        throw std::runtime_error(std::string("QGInputDevice: alsa support not builtin into this build"));
#endif //HAVE_LIBALSA
    } else if (config["type"].as<std::string>().compare("hackrf") == 0) {
#ifdef HAVE_LIBHACKRF
        std::cout << "Input type hackrf" << std::endl;
        return std::unique_ptr<QGInputDevice>(new QGInputHackRF(config, cb));
#else
        throw std::runtime_error(std::string("QGInputDevice: hackrf support not builtin into this build"));
#endif //HAVE_LIBHACKRF
    } else if (config["type"].as<std::string>().compare("rtlsdr") == 0) {
#ifdef HAVE_LIBRTLSDR
        std::cout << "Input type rtlsdr" << std::endl;
        return std::unique_ptr<QGInputDevice>(new QGInputRtlSdr(config, cb));
#else
        throw std::runtime_error(std::string("QGInputDevice: rtlsdr support not uiltin into this build"));
#endif //HAVE_LIBRTLSDR
    } else {
        throw std::runtime_error(std::string("QGInputDevice: unknown type ") + config["type"].as<std::string>());
    }
}
