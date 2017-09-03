#include "QGDownSampler.h"

int main(int argc, char *argv[]) {
	QGDownSampler *d = new QGDownSampler(8000./6, 256);

	d->testRTFilter();
	d->testLiquidDsp();

	return 0;
}
