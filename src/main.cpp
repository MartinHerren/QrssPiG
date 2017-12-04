#include <boost/program_options.hpp>
#include <csignal>

#include "Config.h"
#include "QrssPiG.h"

QrssPiG *gPig = nullptr;

void signalHandler(int signal) { (void)signal; if (gPig) gPig->stop(); }

int main(int argc, char *argv[]) {
	std::cout << QRSSPIG_NAME << " v" << QRSSPIG_VERSION_MAJOR << "." << QRSSPIG_VERSION_MINOR << "." << QRSSPIG_VERSION_PATCH << std::endl;
	signal(SIGINT, signalHandler);
	signal(SIGTERM, signalHandler);
	signal(SIGABRT, signalHandler);

	try {
		using namespace boost::program_options;

		options_description desc{"Options"};
		desc.add_options()
		("help,h", "Help screen")
		("listmodules,m", "List modules")
		("listdevices,l", "List devices")
		("configfile,c", value<std::string>(), "Config file")
		("format,F", value<std::string>()->default_value("rtlsdr"), "Format, 'rtlsdr' or 'hackrf'")
		("samplerate,s", value<int>()->default_value(6000), "Samplerate in S/s")
		("directory,d", value<std::string>()->default_value("./"), "Output directory")
		("sshhost,o", value<std::string>()->default_value(""), "Ssh host")
		("sshuser,u", value<std::string>()->default_value(""), "Ssh user")
		("sshport,p", value<int>()->default_value(0), "Ssh port");

		variables_map vm;
		store(parse_command_line(argc, argv, desc), vm);

		bool stop = false;

		if (vm.count("help")) {
			std::cout << desc << std::endl;
			stop = true;
		}

		if (vm.count("listmodules")) {
			QrssPiG::listModules();
			stop = true;
		}

		if (vm.count("listdevices")) {
			QrssPiG::listDevices();
			stop = true;
		}

		if (stop) exit(0);

		if (vm.count("configfile")) {
			std::string configFile = vm["configfile"].as<std::string>();

			gPig = new QrssPiG(configFile);
		} else {
			std::string format = "rtlsdr";
			int sampleRate = 6000;
			std::string directory;
			std::string sshHost;
			std::string sshUser;
			int sshPort = 22;

			if (vm.count("format")) {
				std::string format = vm["format"].as<std::string>();
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

			gPig = new QrssPiG(format, sampleRate, 2048, directory, sshHost, sshUser, sshPort);
		}
	} catch (const boost::program_options::error &ex) {
		std::cerr << ex.what() << std::endl;
		exit(-1);
	}

	gPig->run();

	delete gPig;

	return 0;
}
