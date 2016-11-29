#include <boost/program_options.hpp>
#include <complex>
#include <iostream>
#include <string>

#include "QGFft.h"
#include "QGImage.h"

int N = 2048; // FFT size

QGFft *fft;
QGImage *im;

double hannW[8000];

int main(int argc, char *argv[]) {
	bool unsignedIQ;
	int sampleRate;

	std::string s("test");
	srand((unsigned) time(NULL));

	try {
		using namespace boost::program_options;

		options_description desc{"Options"};
		desc.add_options()
			("help,h", "Help screen")
			("format,F", value<std::string>()->default_value("rtlsdr"), "Format, 'rtlsdr' or 'hackrf'")
			("samplerate,s", value<int>()->default_value(2000), "Samplerate in S/s");

		variables_map vm;
		store(parse_command_line(argc, argv, desc), vm);

		if (vm.count("help")) {
			std::cout << desc << std::endl;
			exit(0);
		}

		if (vm.count("format")) {
			std::string f = vm["format"].as<std::string>();

			if (f.compare("rtlsdr") == 0) {
				unsignedIQ = true;
			} else if (f.compare("hackrf") == 0) {
				unsignedIQ = false;
			} else {
				std::cerr << "Invalid option for format: " << f << std::endl;
				std::cerr << desc << std::endl;
				exit(-1);
			}
		}

		if (vm.count("samplerate")) {
			sampleRate = vm["samplerate"].as<int>();
		}
	} catch (const boost::program_options::error &ex) {
		std::cerr << ex.what() << std::endl;
		exit(-1);
	}
	
	fft = new QGFft(N);
	im = new QGImage(sampleRate, N);
	
	std::complex<double> *in = fft->getInputBuffer();
	std::complex<double> *out = fft->getOutputBuffer();
	
	for (int i = 0; i < N/2; i++) {
		hannW[i] = .5 * (1 - cos((2 * M_PI * i) / (N / 2 - 1)));
	}
	
//	std::cin >> std::noskipws;
	int idx = 0;
	int y = 0;
	int p = 0;
	int resampleCounter = 0;
	int resampleValue = 0;
	unsigned char i, q;
	
	while (std::cin >> i >> q) {
		if (resampleCounter == 0) {
		in[idx].real(i / 127.);
		in[idx].imag(q / 127.);

		// Shift I/Q from [0,2] to [-1,1} interval for unsigned input
		if (unsignedIQ) in[idx] -= std::complex<double>(-1.,-1);

		in[idx] *= hannW[idx/2];

		idx++;

		if (idx >= N) {
			fft->process();
			im->drawLine(out, y++);
			idx = 0;
		}
			
		if (y > 2000) {
			im->save(s + std::to_string(p++) + ".png");
			y = 0;
		}
		
		resampleCounter++;
		if (resampleCounter >= resampleValue) resampleCounter = 0;
		}
	}
	
	im->save(s + std::to_string(p++) + ".png");
	
	delete im;
	delete fft;

	return 0;
}
