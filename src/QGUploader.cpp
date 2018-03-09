#include "QGUploader.h"
#include "Config.h"

#include <cstring>
#include <functional>
#include <iostream>
#include <syslog.h>
#include <thread>

#include "QGUtils.h"
#include "QGPlugin.h"

#include "plugins/output/image/QGImage.h"

std::vector<std::string> QGUploader::listModules() {
    return (new QGPlugin<QGUploader>("upload"))->list_plugins();
}

std::unique_ptr<QGUploader> QGUploader::CreateUploader(const YAML::Node &config) {
    if (!config["type"]) throw std::runtime_error("YAML: uploader must have a type");
    QGPlugin<QGUploader>* plugin = new QGPlugin<QGUploader>("upload");
    QGUploader *obj = plugin->create(config);
    return std::unique_ptr<QGUploader>(obj);
}

QGUploader::QGUploader(const YAML::Node &config) {
    _fileNameTmpl = "";
    _verbose = false;
    _pushIntermediate = false;

    if (config["filename"]) _fileNameTmpl = config["filename"].as<std::string>();
    if (config["verbose"]) _verbose = config["verbose"].as<bool>();
    if (config["intermediate"]) _pushIntermediate = config["intermediate"].as<bool>();
}

// Push is done in a thread on a copy of the data.
// Parent class handles copying of the data, creation of the thread and finally free the data
// wait param can be set to true to block until pushed. Used on program exit

void QGUploader::push(const QGImage* image, const char *data, int dataSize, bool intermediate, bool wait) {
    if (intermediate && !_pushIntermediate) return;

    char *d = new char[dataSize];

    if (!d) throw std::runtime_error("Out of memory");

    std::memcpy(d, data, dataSize);

    std::string fileName;
	QGUtils::formatFilename(_fileNameTmpl.length() > 0 ? _fileNameTmpl : image->fileNameTmpl(), fileName, image->processor.inputDevice.baseFreq(), image->frameStart());
    fileName += "." + image->fileNameExt();

    if (wait)
        std::thread(std::bind(&QGUploader::_pushThread, this, fileName, d, dataSize)).join();
    else
        std::thread(std::bind(&QGUploader::_pushThread, this, fileName, d, dataSize)).detach();
}

void QGUploader::_pushThread(std::string fileName, const char *data, int dataSize) {
    std::string uri;

    try {
        _pushThreadImpl(fileName, data, dataSize, uri);
		syslog(LOG_ERR, "pushed %s", uri.c_str());
        std::cout << "pushed " << uri << std::endl;
    } catch (const std::exception &e) {
		syslog(LOG_ERR, "pushing %s failed: %s", uri.c_str(), e.what());
        std::cout << "pushing " << uri << " failed: " << e.what() << std::endl;
    }
    // Delete data allocated above so specific implementations don't have to worry
    delete [] data;
}
