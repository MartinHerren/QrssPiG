#include "Config.h"

#include "QGDownSampler.h"

int main(int argc, char *argv[]) {
	QGDownSampler *d = new QGDownSampler(8000./6, 256);

#ifdef HAVE_LIBRTFILTER
	d->testRTFilter();
#endif // HAVE_LIBRTFILTER
#ifdef HAVE_LIBLIQUIDSDR
	d->testLiquidDsp();
#endif // HAVE_LIBLIQUIDSDR

	return 0;
}
