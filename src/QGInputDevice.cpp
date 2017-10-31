#include "QGInputDevice.h"

#include <iostream>
#include <stdexcept>
#include <string>

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
