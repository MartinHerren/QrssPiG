#pragma once

#include "QGInputDevice.h"

#include <memory>

#include <alsa/asoundlib.h>

class QGInputAlsa: public QGInputDevice {
public:
	static std::vector<std::string> listDevices();

	QGInputAlsa(const YAML::Node &config);
	~QGInputAlsa();

private:
	void _startDevice();
	void _stopDevice();

	void _process();

public:
	static void async(snd_async_handler_t *async);

private:
	enum class Channel { MONO, LEFT, RIGHT, IQ, INVIQ };

	std::string _deviceName;
	Channel _channel;

	int _numChannels;
	int _bytesPerSample;
	int _samplesPerCall;

	int _readBufferSize;
	std::unique_ptr<unsigned char[]> _readBuffer;

	snd_pcm_t *_device;
	snd_async_handler_t *_async;
};
