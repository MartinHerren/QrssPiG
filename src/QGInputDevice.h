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
	static std::unique_ptr<QGInputDevice> CreateInputDevice(const YAML::Node &config);

protected:
	QGInputDevice(const YAML::Node &config);

public:
	virtual ~QGInputDevice() {};

	void setCb(std::function<void(const std::complex<float>*)>cb, unsigned int chunkSize);

	void run();
	void stop();

	const std::string& type() const {return _type; };
	unsigned int sampleRate() const { return _sampleRate; };
	unsigned int baseFreq() const { return _baseFreq; };
	float residualPpm() const { return _residualPpm; };

protected:
	std::string _type;
	unsigned int _sampleRate;
	unsigned int _baseFreq;
	float _residualPpm;
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
