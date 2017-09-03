#pragma once

class QGDownSampler {
public:
	QGDownSampler(float rate);
	~QGDownSampler();

	void testRTFilter();
	void testLiquidDsp();

private:
	float _rate;
};
