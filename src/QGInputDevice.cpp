#include "QGInputDevice.h"

#include <iostream>
#include <stdexcept>
#include <string>

#include "Config.h"

#ifdef HAVE_LIBHACKRF
#include "QGInputHackRF.h"
#endif // HAVE_LIBHACKRF

#ifdef HAVE_LIBRTLSDR
#include "QGInputRtlSdr.h"
#endif // HAVE_LIBRTLSDR

QGInputDevice::QGInputDevice(const YAML::Node &config) {
    if (config["samplerate"]) _sampleRate = config["samplerate"].as<int>();
    if (config["basefreq"]) _baseFreq = config["basefreq"].as<int>();
}

QGInputDevice *QGInputDevice::CreateInputDevice(const YAML::Node &config) {
    if (config["device"].as<std::string>().compare("hackrf") == 0) {
        #ifdef HAVE_LIBHACKRF
        std::cout << "Input device hackrf" << std::endl;
        return new QGInputHackRF(config);
        #else
        throw std::runtime_error(std::string("QGInputDevice: hackrf device support not builtin into this build"));
        #endif //HAVE_LIBHACKRF
    } else if (config["device"].as<std::string>().compare("rtlsdr") == 0) {
        #ifdef HAVE_LIBRTLSDR
        std::cout << "Input device rtlsdr" << std::endl;
        return new QGInputRtlSdr(config);
        #else
        throw std::runtime_error(std::string("QGInputDevice: rtlsdr device support not uiltin into this build"));
        #endif //HAVE_LIBRTLSDR
    } else {
        throw std::runtime_error(std::string("QGInputDevice: unknown device ") + config["device"].as<std::string>());
    }
}
