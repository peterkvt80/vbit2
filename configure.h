#ifndef _CONFIGURE_H_
#define _CONFIGURE_H_
/** Configure holds settings like source folder, header info, packet 830 and magazine priority.
 *  It could also hold data about the teletext server that the client should connect to.
 *  /// @todo Rewrite this as a singleton
 *
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdint.h>
#include <cstring>
#include <sys/stat.h>
#include <vector>
#include <array>
#include <algorithm>
#include <stdexcept>

#include "ttxline.h"

#define CONFIGFILE "vbit.conf" // default config file name

#define MAXDEBUGLEVEL 3

namespace ttx

{
class Configure
{
    public:
        enum OutputFormat
        {
            T42,
            Raw,
            PES
        };
        
        //Configure();
        /** Constructor can take overrides from the command line
         */
        Configure(int argc=0, char** argv=NULL);
        ~Configure();
        
        inline std::string GetPageDirectory(){return _pageDir;};
        
        std::string GetHeaderTemplate(){return _headerTemplate;}
        bool GetRowAdaptive(){return _rowAdaptive;}
        std::string GetServiceStatusString(){return _serviceStatusString;}
        bool GetMultiplexedSignalFlag(){return _multiplexedSignalFlag;}
        uint16_t GetNetworkIdentificationCode(){return _NetworkIdentificationCode;}
        std::array<uint8_t, 4> GetReservedBytes(){return _reservedBytes;}
        uint8_t GetInitialMag(){return _initialMag;}
        uint8_t GetInitialPage(){return _initialPage;}
        uint16_t GetInitialSubcode(){return _initialSubcode;}
        uint8_t GetSubtitleRepeats(){return _subtitleRepeats;}
        uint16_t GetCommandPort(){return _commandPort;}
        bool GetCommandPortEnabled(){return _commandPortEnabled;}
        uint16_t GetLinesPerField(){return _linesPerField;}
        bool GetReverseFlag(){return _reverseBits;}
        int GetDebugLevel(){return _debugLevel;}
        int GetMagazinePriority(uint8_t mag){return _magazinePriority[mag];}
        
        OutputFormat GetOutputFormat(){return _OutputFormat;}
        
    private:
        int DirExists(std::string *path);
        
        int LoadConfigFile(std::string filename);
        
        // template string for generating header packets
        std::string _headerTemplate;
        
        uint16_t _commandPort;
        
        bool _rowAdaptive;
        uint16_t _linesPerField;
        
        // settings for generation of packet 8/30
        bool _multiplexedSignalFlag; // false indicates teletext is multiplexed with video, true means full frame teletext.
        int _magazinePriority[8];
        uint8_t _initialMag;
        uint8_t _initialPage;
        uint16_t _initialSubcode;
        uint16_t _NetworkIdentificationCode;
        uint16_t _CountryNetworkIdentificationCode;
        std::array<uint8_t, 4> _reservedBytes; // four bytes which the teletext specification marks reserved
        std::string _serviceStatusString; /// 20 characters
        
        std::string _configFile; /// Configuration file name --config
        std::string _pageDir; /// Configuration file name --dir
        uint8_t _subtitleRepeats; /// Number of times a subtitle repeats (typically 1 or 2).
        bool _commandPortEnabled;
        bool _reverseBits;
        int _debugLevel;
        
        OutputFormat _OutputFormat;
    };
}

#endif
