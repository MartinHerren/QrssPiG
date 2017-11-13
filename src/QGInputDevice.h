#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include <yaml-cpp/yaml.h>

class QGInputDevice {
protected:
	QGInputDevice(const YAML::Node &config);

public:
	virtual ~QGInputDevice() {};

	void setCb(std::function<void(const std::complex<float>*)>cb, unsigned int chunkSize);

	void run();
	void stop();

	unsigned int sampleRate() { return _sampleRate; };
	unsigned int baseFreq() { return _baseFreq; };
	int ppm() { return _ppm; };

	static std::unique_ptr<QGInputDevice> CreateInputDevice(const YAML::Node &config);

protected:
	void _adjBufferSize(int adj);

	unsigned int _sampleRate;
	unsigned int _baseFreq;
	int _ppm;

	std::mutex _bufferMutex;
	size_t _bufferCapacity;
	size_t _bufferSize;
	volatile size_t _bufferHead;
	volatile size_t _bufferTail;
	std::vector<std::complex<float>> _buffer;

	std::atomic<bool> _running;

private:
	virtual void _startDevice() = 0;
	virtual void _stopDevice() = 0;

	std::function<void(const std::complex<float>*)> _cb;
	unsigned int _chunkSize;
};
