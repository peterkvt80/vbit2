/** Configure
 */
#include "configure.h"

using namespace ttx;

int Configure::DirExists(char *path){
	struct stat info;
	
	if(stat(path, &info ) != 0)
		return 0;
	else if(info.st_mode & S_IFDIR)
		return 1;
	else
		return 0;
}

Configure::Configure(int argc, char** argv) :

	// settings for generation of packet 8/30
	_multiplexedSignalFlag(0),
	_initialMag(1),
	_initialPage(0x00),
	_initialSubcode(0x3F7F),
	_NetworkIdentificationCode(0x0000)
{
	std::cerr << "[Configure::Configure] Started" << std::endl;
	strncpy(_configFile,"vbit.conf",MAXPATH-1);
#ifdef _WIN32
	strncpy(_pageDir,"i:\\temp\\teletext",MAXPATH-1);
#else
	strcpy(_pageDir,"/home/pi/teletext");
#endif
	// This is where the default header template is defined.
	// Work out the c++ string way of doing this
	// sprintf(headerTemplate," VBIT-PI %%%%# %%%%a %%d %%%%b%c%%H:%%M/%%S",0x83); // include alpha yellow code
	//char headerTemplate[33];
	//char serviceStatusString[21];
	//Scan the command line for overriding the pages file.
	std::cerr << "[Configure::Configure] Parameters=" << argc << " " << std::endl;
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
	if (!DirExists(_pageDir)){
		std::cerr << "[Configure::Configure] " << _pageDir << " does not exist or is not a directory" << std::endl;
		exit(EXIT_FAILURE);
	}
	
	/// @ scan for overriding the configuration file
	std::cerr << "[Configure::Configure] Pages directory is " << _pageDir << std::endl;
	std::cerr << "[Configure::Configure] Config file is " << _configFile << std::endl;
	/// @todo load the configuration file.
	/// @todo scan the command line for other overrides.
	std::cerr << "[Configure::Configure] Exits" << std::endl;
}

Configure::~Configure()
{
	std::cerr << "[Configure] Destructor" << std::endl;
}
