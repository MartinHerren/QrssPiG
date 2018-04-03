#pragma once

#include <chrono>
#include <complex>
#include <functional>
#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "Config.h"
#include <dlfcn.h>

#include <boost/filesystem.hpp>
#include "QGProcessor.h"

class NoListDevicesException: public std::exception
{
  virtual const char* what() const throw()
  {
    return "No list_devices method found in plugin";
  }
};

template <class T>
class QGPlugin {
protected:
	std::string prefix;

public:
	QGPlugin(std::string prefix) {
		this->prefix = prefix;
	};

	virtual ~QGPlugin() {};

	T* create(const YAML::Node &config, const QGProcessor& processor, unsigned int index);
	T* create(const YAML::Node &config);

	std::vector<std::string> list_plugins();
	bool has_list_devices(std::string type);
	std::vector<std::string> list_devices(std::string type);
private:
	void *load_dl(std::string type);

	std::string build_path(std::string &type) {
		return std::string(INSTALL_PREFIX) + "/lib/qrsspig/lib"+prefix+"_" + type + ".so";
	}
};


template <class T>
void* QGPlugin<T>::load_dl(std::string type) {
	std::string path = type;

	if (type.find("/") != 0) {
		path = this->build_path(type);
	}

	void* handle = dlopen(path.c_str(), RTLD_LAZY);
	if (!handle) {
		throw std::runtime_error(std::string("QGPlugin: can't open ") + path);
	}
  
	return handle;
}

template <class T>
T* QGPlugin<T>::create(const YAML::Node &config, const QGProcessor& processor, unsigned int index) {
	std::string type = "image"; // Default type if not specified for back-compatibility

  if (config["type"]) {
    type = config["type"].as<std::string>();
  }

	void*handle = this->load_dl(type);
	T* (*create)(const YAML::Node &config, const QGProcessor& processor, unsigned int index);	
	create = (T* (*)(const YAML::Node &config, const QGProcessor& processor, unsigned int index))dlsym(handle, "create_object");

	if (!create) {
		throw std::runtime_error(std::string("QGPlugin: can't load create from ") + type);
	}

	T* obj = (T*)create(config, processor, index);

	if (!obj) {
		throw std::runtime_error(std::string("QGPlugin: can't create ") + type);
	}

	return obj;
}

template <class T>
T* QGPlugin<T>::create(const YAML::Node &config) {
	std::string type = config["type"].as<std::string>();

	void*handle = this->load_dl(type);
	T* (*create)(const YAML::Node &config);	
	create = (T* (*)(const YAML::Node &config))dlsym(handle, "create_object");

	if (!create) {
		throw std::runtime_error(std::string("QGPlugin: can't load create from ") + type);
	}

	T* obj = (T*)create(config);

	if (!obj) {
		throw std::runtime_error(std::string("QGPlugin: can't create ") + type);
	}

	return obj;
}

template <class T>
std::vector<std::string> QGPlugin<T>::list_plugins() {
	struct path_leaf_string
    {
	    std::string operator()(const boost::filesystem::directory_entry& entry) const {
		    return entry.path().leaf().string();
	    }
    };

	std::string begins_with = "lib" + prefix + "_";

    boost::filesystem::path p(std::string(INSTALL_PREFIX) + "/lib/qrsspig");
    boost::filesystem::directory_iterator start(p);
    boost::filesystem::directory_iterator end;
    std::vector<std::string> v;
    std::transform(start, end, std::back_inserter(v), path_leaf_string());
    std::vector<std::string> v2;
    std::copy_if(v.begin(),v.end(),std::back_inserter(v2), [&begins_with](const std::string i) {
		    return i.find(begins_with) == 0 && i.find(".so") == i.length()-3;
	    });
    std::vector<std::string> v3;
    std::transform(v2.begin(),v2.end(),std::back_inserter(v3), [&begins_with](const std::string i) {
		    std::string x = i;
		    x.erase(0,begins_with.length());
		    x.erase(x.length()-3,x.length());
		    return x;
	    });

    return v3;
}

template <class T>
bool QGPlugin<T>::has_list_devices(std::string type) {
	void*handle = this->load_dl(type);
	std::vector<std::string> (*list_devices)();	
	list_devices = (std::vector<std::string> (*)())dlsym(handle, "list_devices");
	return list_devices != NULL;
}

template <class T>
std::vector<std::string> QGPlugin<T>::list_devices(std::string type) {
	void*handle = this->load_dl(type);
	std::vector<std::string> (*list_devices)();	
	list_devices = (std::vector<std::string> (*)())dlsym(handle, "list_devices");
	if(!list_devices) {
		throw std::runtime_error(std::string("QGPlugin: Failed to load 'list_devices' for ") + type);
	}
	return list_devices();
}
