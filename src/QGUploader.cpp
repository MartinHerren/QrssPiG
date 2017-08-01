#include "QGUploader.h"

#include <cstring>
#include <iostream>
#include <thread>

// Push is done in a thread on a copy of the data.
// Parent class handles copying of the data, creation of the thread and finally free the data
// wait param can be set to true to block until pushed. Used on program exit

void QGUploader::push(const std::string &fileName, const char *data, int dataSize, bool wait) {
    char *d = new char[dataSize];

    if (!d) throw std::runtime_error("Out of memory");

    std::memcpy(d, data, dataSize);

    if (wait)
        std::thread(std::bind(&QGUploader::_pushThread, this, fileName, d, dataSize)).join();
    else
        std::thread(std::bind(&QGUploader::_pushThread, this, fileName, d, dataSize)).detach();
}

void QGUploader::_pushThread(std::string fileName, const char *data, int dataSize) {
    _pushThreadImpl(fileName, data, dataSize);

    // Delete data allocated above so specific implementations don't have to worry
    delete [] data;
std::cout << "pushed " << fileName << std::endl;
}
