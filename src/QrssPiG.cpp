#include "QrssPiG.h"

#include <boost/program_options.hpp>
#include <string>

QrssPiG::QrssPiG(bool unsignedIQ, int sampleRate, int N) :
	_unsignedIQ(unsignedIQ),
	_sampleRate(sampleRate),
	_N(N) {
	_fft = new QGFft(N);
	_fftIn = _fft->getInputBuffer();
	_fftOut = _fft->getOutputBuffer();

	_im = new QGImage(sampleRate, N);
	_up = nullptr;
}

QrssPiG::~QrssPiG() {
	push();

	if (_fft) delete _fft;
	if (_im) delete _im;
	if (_up) delete _up;
}

void QrssPiG::addUploader(const std::string &sshHost, const std::string &sshUser, const std::string &sshDir, int sshPort) {
	if (_up) delete _up;
	_up = new QGUploader(sshHost, sshUser, sshDir, sshPort);
}

void QrssPiG::fft(int y) {
	_fft->process();
	_im->drawLine(_fftOut, y);
}

void QrssPiG::push() {
	std::string s("test");
	int p = 0;

	try {
		_im->save2Buffer();
		if (_up) _up->pushFile(s + std::to_string(p) + ".png", _im->getBuffer(), _im->getBufferSize());
	} catch (const std::exception &e) {
		std::cerr << "Error pushing file: " << e.what() << std::endl;
	}
	p++;
}

double hannW[8000];

int main(int argc, char *argv[]) {
	bool unsignedIQ;
	int sampleRate;
	std::string sshHost;
	std::string sshUser;
	std::string sshDir;
	int sshPort;

	srand((unsigned) time(NULL));

	try {
		using namespace boost::program_options;

		options_description desc{"Options"};
		desc.add_options()
		("help,h", "Help screen")
		("format,F", value<std::string>()->default_value("rtlsdr"), "Format, 'rtlsdr' or 'hackrf'")
		("samplerate,s", value<int>()->default_value(2000), "Samplerate in S/s")
		("sshhost,o", value<std::string>()->default_value(""), "Ssh host")
		("sshuser,u", value<std::string>()->default_value(""), "Ssh user")
		("sshdir,d", value<std::string>()->default_value("."), "Ssh directory")
		("sshport,p", value<int>()->default_value(0), "Ssh port");

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

		if (vm.count("sshhost")) {
			sshHost = vm["sshhost"].as<std::string>();
		}

		if (vm.count("sshuser")) {
			sshUser = vm["sshuser"].as<std::string>();
		}

		if (vm.count("sshdir")) {
			sshDir = vm["sshdir"].as<std::string>();
		}

		if (vm.count("sshport")) {
			sshPort = vm["sshport"].as<int>();
		}
	} catch (const boost::program_options::error &ex) {
		std::cerr << ex.what() << std::endl;
		exit(-1);
	}

	QrssPiG *pig = new QrssPiG(unsignedIQ, sampleRate);
	if (sshHost.length()) pig->addUploader(sshHost, sshUser, sshDir, sshPort);

  // TODO remove N... put hann filter into class
	int N = 2048;

	for (int i = 0; i < N/2; i++) {
		hannW[i] = .5 * (1 - cos((2 * M_PI * i) / (N / 2 - 1)));
	}

	//	std::cin >> std::noskipws;
	int idx = 0;
	int y = 0;
	int resampleCounter = 0;
	int resampleValue = 0;
	unsigned char i, q;

	while (std::cin >> i >> q) {
		if (resampleCounter == 0) {
			pig->_fftIn[idx].real(i / 127.);
			pig->_fftIn[idx].imag(q / 127.);

			// Shift I/Q from [0,2] to [-1,1} interval for unsigned input
			if (unsignedIQ) pig->_fftIn[idx] -= std::complex<double>(-1.,-1);

			pig->_fftIn[idx] *= hannW[idx/2];

			idx++;

			if (idx >= N) {
				pig->fft(y++);

				if (y >= 2000) {
					pig->push();
					y = 0;
				}

				idx = 0;
			}

			resampleCounter++;
			if (resampleCounter >= resampleValue) resampleCounter = 0;
		}
	}

	delete pig;

	return 0;
}
