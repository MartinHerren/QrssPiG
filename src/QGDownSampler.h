#pragma once

class QGDownSampler {
public:
	QGDownSampler(unsigned int rate);
	~QGDownSampler();

	void testRTFilter();

private:
	unsigned int _rate;
};
