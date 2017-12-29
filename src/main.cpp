#include <boost/program_options.hpp>
#include <csignal>

#include "Config.h"
#include "QrssPiG.h"

QrssPiG *gPig = nullptr;

void signalHandler(int signal) { (void)signal; if (gPig) gPig->stop(); }

int main(int argc, char *argv[]) {
	std::cout << QRSSPIG_NAME << " v" << QRSSPIG_VERSION_MAJOR << "." << QRSSPIG_VERSION_MINOR << "." << QRSSPIG_VERSION_PATCH << std::endl;

	try {
		std::string configfile;
		using namespace boost::program_options;

		options_description desc{"Options"};
		desc.add_options()
		("help,h", "Help screen")
		("listmodules,m", "List modules")
		("listdevices,l", "List devices")
		("configfile,c", value<std::string>(&configfile)->required(), "Config file");

		positional_options_description pd;
		pd.add("configfile", 1);

		variables_map vm;
		store(command_line_parser(argc, argv).options(desc).positional(pd).run(), vm);

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

		// Check only now as otherwise -h -m -l options would require the config file option
		vm.notify();

		gPig = new QrssPiG(configfile);

		signal(SIGINT, signalHandler);
		signal(SIGTERM, signalHandler);
		signal(SIGABRT, signalHandler);

		gPig->run();

		delete gPig;
	} catch (const boost::program_options::required_option &ex) {
		std::cerr << "Error: " << ex.what() << std::endl;
		exit(-1);
	} catch (const boost::program_options::error &ex) {
		std::cerr << "Error: " << ex.what() << std::endl;
		exit(-1);
	} catch (const std::exception &ex) {
		std::cerr << "Error: " << ex.what() << std::endl;
		exit(-1);
	}

	return 0;
}
