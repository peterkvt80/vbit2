/** Top level teletext application
 *  @brief Load the configuration, initialise the service and run it.
 * @detail This can be run without parameters in which case it will use the defaults in configure.
 * If a parameter is given then it replaces the defaults.
 * @todo Define options for overriding the config file settings, and the config file itself,
 * This will include path to pages, header packet, priority etc.
 * Example: vbit2 --config teletext/vbit.conf
 * Compiler: c++11
 */

#include "vbit2.h"

using namespace ttx;

/* Options
 * --dir <path to pages>
 * Example (and default)
 * --dir /home/pi/teletext
 * Sets the pages directory and the location of vbit.conf.
 */

int main(int argc, char** argv)
{
	// std::cout << "VBIT2 started" << std::endl;
	/// @todo option of adding a non standard config path
	Configure *configure=new Configure(argc, argv);
	PageList *pageList=new PageList(configure);
	Service *service=new Service(configure, pageList);

	service->run();

	std::cout << "VBIT2 ended" << std::endl;
    system("pause"); // @todo Only apply this line in debug
}

