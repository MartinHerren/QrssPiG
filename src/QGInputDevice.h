#pragma once

#include <atomic>
#include <complex>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include <yaml-cpp/yaml.h>

class QGInputDevice {
public:
	static std::vector<std::string> listModules();
	static std::vector<std::pair<std::string, std::vector<std::string>>> listDevices();

protected:
	QGInputDevice(const YAML::Node &config);

public:
	virtual ~QGInputDevice() {};

	void setCb(std::function<void(const std::complex<float>*)>cb, unsigned int chunkSize);

	void run();
	void stop();

	unsigned int sampleRate() { return _sampleRate; };
	unsigned int baseFreq() { return _baseFreq; };
	float ppm() { return _ppm; };

	static std::unique_ptr<QGInputDevice> CreateInputDevice(const YAML::Node &config);

protected:
	unsigned int _sampleRate;
	unsigned int _baseFreq;
	float _ppm;
	int _bufferlength;

	unsigned int _bufferCapacity;
	std::atomic<unsigned int> _bufferSize;
	unsigned int _bufferHead;
	unsigned int _bufferTail;
	std::vector<std::complex<float>> _buffer;

	std::atomic<bool> _running;

private:
	void _bufferMonitor();
	virtual void _startDevice() = 0;
	virtual void _stopDevice() = 0;

	std::function<void(const std::complex<float>*)> _cb;
	unsigned int _chunkSize;

	bool _debugBufferMonitor;
};
