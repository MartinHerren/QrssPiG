#include <boost/program_options.hpp>
#include <complex>
#include <iostream>

inline std::complex<double> genIQ(double f, double t) {
	double wt = 2. * M_PI * f * t;
	return std::complex<double>(cos(wt), sin(wt));
}

int main(int argc, char *argv[]) {
	bool unsignedIQ;
	int sampleRate;
	int numSeconds;
	double frequency;
	double amplitude;
	
	try {
		using namespace boost::program_options;
		
		options_description desc{"Options"};
		desc.add_options()
			("help,h", "Help screen")
			("format,F", value<std::string>()->default_value("rtlsdr"), "Format, 'rtlsdr' or 'hackrf'")
			("samplerate,s", value<int>()->default_value(2000), "Samplerate in S/s")
			("duration,d", value<int>()->default_value(2), "Duration in seconds")
			("frequency,f", value<double>()->default_value(100000.), "Frequency in Hz")
			("amplitude,a", value<double>()->default_value(.1), "Amplitude");
		
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
		
		if (vm.count("duration")) {
			numSeconds = vm["duration"].as<int>();
		}
		
		if (vm.count("frequency")) {
			frequency = vm["frequency"].as<double>();
		}
		
		if (vm.count("amplitude")) {
			amplitude = vm["amplitude"].as<double>();
		}
	} catch (const boost::program_options::error &ex) {
		std::cerr << ex.what() << std::endl;
		exit(-1);
	}

	//srand((unsigned) time(NULL));
	
	for (double t = 0.; t < numSeconds; t += 1./sampleRate) {
		//double r = (double)rand()/RAND_MAX - .5;
		std::complex<double> iq = amplitude * genIQ(frequency, t);

		// Hard clip magnitude to [-1,1] interval
		double iqAbs = abs(iq);
		if (iqAbs > 1) iq /= iqAbs;

		// Shift I/Q to [0,2] interval for unsigned output
		if (unsignedIQ) iq += std::complex<double>(1., 1.);
		
		unsigned char i = ((unsigned char)trunc(127 * iq.real())) & 0xff;
		unsigned char q = ((unsigned char)trunc(127 * iq.imag())) & 0xff;
		
		std::cout << i << q;
	}

	return 0;
}
