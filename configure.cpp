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
    _initialMag(1),
    _initialPage(0x00),
    _initialSubcode(0x3F7F),
    _NetworkIdentificationCode(0x0000),
    _CountryNetworkIdentificationCode(0x0000),
    _serviceStatusString(20, ' '),
    _subtitleRepeats(1)
{
    std::cerr << "[Configure::Configure] Started" << std::endl;
    strncpy(_configFile,CONFIGFILE,MAXPATH-1);
#ifdef _WIN32
    strncpy(_pageDir,"./pages",MAXPATH-1); // a relative path as a sensible default
#else
    strcpy(_pageDir,"/home/pi/teletext");
#endif
    // This is where the default header template is defined.
    _headerTemplate = "VBIT2    %%# %%a %d %%b" "\x03" "%H:%M:%S";
    
    // the default command interface port
    _commandPort = 5570;
    _commandPortEnabled = false;
    
    _reverseBits = false;

    _rowAdaptive = false;
    _linesPerField = 16; // default to 16 lines per field

    _multiplexedSignalFlag = false; // using this would require changing all the line counting and a way to send full field through raspi-teletext - something for the distant future when everything else is done...
    
    uint8_t priority[8]={9,3,3,6,3,3,5,6}; // 1=High priority,9=low. Note: priority[0] is mag 8
    
    for (int i=0; i<8; i++)
        _magazinePriority[i] = priority[i];

    //Scan the command line for overriding the pages file.
    //std::cerr << "[Configure::Configure] Parameters=" << argc << " " << std::endl;
    if (argc>1)
    {
        for (int i=1;i<argc;i++)
        {
            if (strncmp(argv[i],"--dir",5)==0)
            {
                i++;
                strncpy(_pageDir,argv[i],MAXPATH-1);
            }
            else if (strncmp(argv[i],"--reverse",9)==0)
            {
                _reverseBits = true;
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
    std::vector<std::string> nameStrings{ "header_template", "initial_teletext_page", "row_adaptive_mode", "network_identification_code", "country_network_identification", "full_field", "status_display", "subtitle_repeats","enable_command_port","command_port","lines_per_field","magazine_priority" };

    if (filein.is_open()){
        std::cerr << "[Configure::LoadConfigFile] opened " << filename << std::endl;

        std::string line;
        std::string name;
        std::string value;
        TTXLine* header = new TTXLine();
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
                                header->Setm_textline(value,true);
                                value = header->GetLine();
                                value.resize(32,' ');
                                _headerTemplate.assign(value);
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

                            case 2: // row_adaptive_mode
                                if (!value.compare("true")){
                                    _rowAdaptive = true;
                                } else if (!value.compare("false")){
                                    _rowAdaptive = false;
                                } else {
                                    error = 1;
                                }
                                break;
                            case 3: // "country_network_identification" - four character hex. eg. FA6F
                                if (value.size() == 4){
                                    size_t idx;
                                    try {
                                        _NetworkIdentificationCode = stoi(std::string(value, 0, 4), &idx, 16);
                                    } catch (const std::invalid_argument& ia) {
                                        error = 1;
                                        break;
                                    }
                                } else {
                                    error = 1;
                                }
                                break;
                            case 4: // "country_network_identification" - four character hex. eg. 2C2F
                                if (value.size() == 4){
                                    size_t idx;
                                    try {
                                        _CountryNetworkIdentificationCode = stoi(std::string(value, 0, 4), &idx, 16);
                                    } catch (const std::invalid_argument& ia) {
                                        error = 1;
                                        break;
                                    }
                                } else {
                                    error = 1;
                                }
                                break;
                            case 5: // "full_field"
                                break;
                            case 6: // "status_display"
                                value.resize(20,' '); // string must be 20 characters
                                _serviceStatusString.assign(value);
                                break;
                            case 7: // "subtitle_repeats" - The number of times a subtitle transmission is repeated 0..9
                                if (value.size() == 1){
                                    try {
                                        _subtitleRepeats = stoi(std::string(value, 0, 1));
                                    } catch (const std::invalid_argument& ia) {
                                        error = 1;
                                        break;
                                    }
                                } else {
                                    error = 1;
                                }
                                break;
                            case 8: // "enable_command_port"
                                if (!value.compare("true")){
                                    _commandPortEnabled = true;
                                } else if (!value.compare("false")){
                                    _commandPortEnabled = false;
                                } else {
                                    error = 1;
                                }
                                break;
                            case 9: // "command_port"
                                if (value.size() > 0 && value.size() < 6){
                                    try {
                                        _commandPort = stoi(std::string(value, 0, 5));
                                    } catch (const std::invalid_argument& ia) {
                                        error = 1;
                                        break;
                                    }
                                } else {
                                    error = 1;
                                }
                                break;
                            case 10: // "lines_per_field"
                                if (value.size() > 0 && value.size() < 4){
                                    try {
                                        _linesPerField = stoi(std::string(value, 0, 3));
                                    } catch (const std::invalid_argument& ia) {
                                        error = 1;
                                        break;
                                    }
                                } else {
                                    error = 1;
                                }
                                break;
                            case 11: // "magazine_priority"
                                // TODO: implement parsing from config
                                std::stringstream ss(value);
                                std::string temps;
                                int tmp[8];
                                int i;
                                for (i=0; i<8; i++)
                                {
                                    if (std::getline(ss, temps, ','))
                                    {
                                        try {
                                            tmp[i] = stoi(temps);
                                        } catch (const std::invalid_argument& ia) {
                                            error = 1;
                                            break;
                                        }
                                        if (!(tmp[i] > 0 && tmp[i] < 10)) // must be 1-9
                                        {
                                            error = 1;
                                            break;
                                        }
                                    }
                                    else
                                    {
                                        error = 1;
                                        break;
                                    }
                                }
                                for (i=0; i<8; i++)
                                    _magazinePriority[i] = tmp[i];
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
