#include "QGDownSampler.h"

int main(int argc, char *argv[]) {
	QGDownSampler *d = new QGDownSampler(1000000/6000);

	d->testRTFilter();

	return 0;
}
