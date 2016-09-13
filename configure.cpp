/** Configure
 */
#include "configure.h"

using namespace ttx;

Configure::Configure(int argc, char** argv) :

	// settings for generation of packet 8/30
	_multiplexedSignalFlag(0),
	_initialMag(1),
	_initialPage(0x00),
	_initialSubcode(0x3F7F),
	_NetworkIdentificationCode(0x0000)
{
	std::cout << "[Configure::Configure] Started" << std::endl;
	strncpy(_configFile,"vbit.conf",MAXPATH-1);
	strncpy(_pageDir,"i:\\temp\\teletext",MAXPATH-1); // Can't use ~ because this is not the shell
	// This is where the default header template is defined.
	// Work out the c++ string way of doing this
	// sprintf(headerTemplate," VBIT-PI %%%%# %%%%a %%d %%%%b%c%%H:%%M/%%S",0x83); // include alpha yellow code
	//char headerTemplate[33];
	//char serviceStatusString[21];
	//Scan the command line for overriding the pages file.
	std::cout << "Parameters=" << argc << " " << std::endl;
	if (argc>1)
	{
		for (int i=1;i<argc-1;i++)
		{
			if (strncmp(argv[i],"--dir",5)==0)
			{
				strncpy(_pageDir,argv[i+1],MAXPATH-1);
				break;
			}
		}
	}
	/// @ scan for overriding the configuration file
	std::cout << "Pages directory is " << _pageDir << std::endl;
	std::cout << "Config file is " << _configFile << std::endl;
	/// @todo load the configuration file.
	/// @todo scan the command line for other overrides.
}

Configure::~Configure()
{
	std::cout << "[Configure] Destructor" << std::endl;
}
