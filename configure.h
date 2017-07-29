#ifndef _CONFIGURE_H_
#define _CONFIGURE_H_
/** Configure holds settings like source folder, header info, packet 830 and magazine priority.
 *  It could also hold data about the teletext server that the client should connect to.
 *  /// @todo Rewrite this as a singleton
 *  
 */

#include <iostream>
#include <fstream>
#include <stdint.h>
#include <cstring>
#include <sys/stat.h>
#include <vector>
#include <algorithm>

#define CONFIGFILE "vbit.conf" // default config file name
#define MAXPATH 132

namespace ttx 

{
class Configure
{
public:
	//Configure();
	/** Constructor can take overrides from the command line
	 */
	Configure(int argc=0, char** argv=NULL);
	~Configure();
	
	inline char* GetPageDirectory(){return _pageDir;};
	
	std::string GetHeaderTemplate(){return _headerTemplate;}
	
private:
	int DirExists(char *path);
	
	int LoadConfigFile(std::string filename);
	
	// template string for generating header packets
	std::string _headerTemplate;

	// settings for generation of packet 8/30
	uint8_t _multiplexedSignalFlag; 	// 0 indicates teletext is multiplexed with video, 1 means full frame teletext.
	uint8_t _initialMag;
	uint8_t _initialPage;
	uint16_t _initialSubcode;
	uint16_t _NetworkIdentificationCode;
	char _serviceStatusString[21];
	char _configFile[MAXPATH]; /// Configuration file name --config
	char _pageDir[MAXPATH]; /// Configuration file name --dir
};

}

#endif
