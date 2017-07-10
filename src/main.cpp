#include <boost/program_options.hpp>

#include "QrssPiG.h"

int main(int argc, char *argv[]) {
	QrssPiG *pig;

	srand((unsigned) time(NULL));

	try {
		using namespace boost::program_options;

		options_description desc{"Options"};
		desc.add_options()
		("help,h", "Help screen")
		("configfile,c", value<std::string>(), "Config file")
		("format,F", value<std::string>()->default_value("rtlsdr"), "Format, 'rtlsdr' or 'hackrf'")
		("samplerate,s", value<int>()->default_value(2000), "Samplerate in S/s")
		("directory,d", value<std::string>()->default_value("./"), "Output directory")
		("sshhost,o", value<std::string>()->default_value(""), "Ssh host")
		("sshuser,u", value<std::string>()->default_value(""), "Ssh user")
		("sshport,p", value<int>()->default_value(0), "Ssh port");

		variables_map vm;
		store(parse_command_line(argc, argv, desc), vm);

		if (vm.count("help")) {
			std::cout << desc << std::endl;
			exit(0);
		}

		if (vm.count("configfile")) {
			std::string configFile = vm["configfile"].as<std::string>();

			pig = new QrssPiG(configFile);
		} else {
			bool unsignedIQ = true;
			int sampleRate = 2000;
			std::string directory;
			std::string sshHost;
			std::string sshUser;
			int sshPort = 22;

			if (vm.count("format")) {
				std::string f = vm["format"].as<std::string>();

				if ((f.compare("rtlsdr") == 0) || (f.compare("unsigned") == 0)) {
					unsignedIQ = true;
				} else if ((f.compare("hackrf") == 0) || (f.compare("signed") == 0)) {
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

			if (vm.count("directory")) {
				directory = vm["directory"].as<std::string>();
			}

			if (vm.count("sshhost")) {
				sshHost = vm["sshhost"].as<std::string>();
			}

			if (vm.count("sshuser")) {
				sshUser = vm["sshuser"].as<std::string>();
			}

			if (vm.count("sshport")) {
				sshPort = vm["sshport"].as<int>();
			}

			pig = new QrssPiG(2048, unsignedIQ, sampleRate, directory, sshHost, sshUser, sshPort);
		}
	} catch (const boost::program_options::error &ex) {
		std::cerr << ex.what() << std::endl;
		exit(-1);
	}

	unsigned char i, q;

	std::cin >> std::noskipws;
	while (std::cin >> i >> q) {
		std::complex<double> iq(i/127., q/127.);

		pig->addIQ(iq);
	}

	delete pig;

	return 0;
}
