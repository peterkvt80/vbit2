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
	_NetworkIdentificationCode(0x0000),
	_serviceStatusString(19, ' ')
{
	std::cerr << "[Configure::Configure] Started" << std::endl;
	strncpy(_configFile,CONFIGFILE,MAXPATH-1);
#ifdef _WIN32
	strncpy(_pageDir,"i:\\temp\\teletext",MAXPATH-1);
#else
	strcpy(_pageDir,"/home/pi/teletext");
#endif
	// This is where the default header template is defined.
	_headerTemplate = "TEEFAX 1 %%# %%a %d %%b" "\x03" "%H:%M.%S";
	
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
	std::string path;
	path = _pageDir;
	path += "/";
	path += _configFile;
	LoadConfigFile(path);
	/// @todo scan the command line for other overrides.
	std::cerr << "[Configure::Configure] Exits" << std::endl;
}

Configure::~Configure()
{
	std::cerr << "[Configure] Destructor" << std::endl;
}

int Configure::LoadConfigFile(std::string filename)
{
	std::ifstream filein(filename.c_str()); // Open the file
	
	std::vector<std::string>::iterator iter;
	// these are all the valid strings for config lines
	std::vector<std::string> nameStrings{"header_template", "initial_teletext_page"};
	
	if (filein.is_open()){
		std::cerr << "[Configure::LoadConfigFile] opened " << filename << std::endl;
		
		std::string line;
		std::string name;
		std::string value;
		while (std::getline(filein >> std::ws, line)){
			if (line.front() != ';'){ // ignore comments
				/// todo: parsing!
				std::size_t delim = line.find("=", 0);
				int error = 0;
				
				if (delim != std::string::npos){
					name = line.substr(0, delim);
					value = line.substr(delim + 1);
					iter = find(nameStrings.begin(), nameStrings.end(), name);
					if(iter != nameStrings.end()){
						// matched string
						switch(iter - nameStrings.begin()){
							case 0: // header_template
								_headerTemplate.assign(value, 0, 32);
								break;
							
							case 1: // initial_teletext_page
								if (value.size() >= 3){
									size_t idx;
									int magpage;
									int subcode;
									try {
										magpage = stoi(std::string(value, 0, 3), &idx, 16);
									} catch (const std::invalid_argument& ia) {
										error = 1;
										break;
									}
									if (magpage < 0x100 || magpage > 0x8FF || (magpage & 0xFF) == 0xFF){
										error = 1;
										break;
									}
									if (value.size() > 3){
										if ((value.size() != 8) || (value.at(idx) != ':')){
											error = 1;
											break;
										}
										try {
											subcode = stoi(std::string(value, 4, 4), &idx, 16);
										} catch (const std::invalid_argument& ia) {
											error = 1;
											break;
										}
										if (subcode < 0x0000 || subcode > 0x3F7F || subcode & 0xC080){
											error = 1;
											break;
										}
										_initialSubcode = subcode;
									}
									_initialMag = magpage / 0x100;
									_initialPage = magpage % 0x100;
									break;
								} 
								error = 1;
								break;
						}
					} else {
						error = 1; // unrecognised config line
					}
				}
				if (error)
					std::cerr << "[Configure::LoadConfigFile] invalid config line: " << line << std::endl;
			}
		}
		filein.close();
		return 0;
	} else {
		std::cerr << "[Configure::LoadConfigFile] open failed" << std::endl;
		return -1;
	}
}