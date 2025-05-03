/** Configure
 */
#include "configure.h"

using namespace vbit;

int Configure::DirExists(std::string *path)
{
    struct stat info;

    if(stat(path->c_str(), &info ) != 0)
        return 0;
    else if(info.st_mode & S_IFDIR)
        return 1;
    else
        return 0;
}

Configure::Configure(Debug *debug, int argc, char** argv) :
    _debug(debug),
    // settings for generation of packet 8/30
    _initialMag(1),
    _initialPage(0x00),
    _initialSubcode(0x3F7F),
    _NetworkIdentificationCode(0x0000),
    _CountryNetworkIdentificationCode(0x0000),
    _reservedBytes{0x15, 0x15, 0x15, 0x15}, // initialise reserved bytes to hamming 8/4 encoded 0
    _serviceStatusString(20, ' ')
{
    _configFile = CONFIGFILE;
    
#ifdef RASPBIAN
    _pageDir = "/home/pi/teletext";
#else
    _pageDir = "./pages"; // a relative path as a sensible default
#endif
    // This is where the default header template is defined.
    _headerTemplate = "VBIT2    %%# %%a %d %%b" "\x03" "%H:%M:%S";
    
    _reverseBits = false;

    _rowAdaptive = false;
    _linesPerField = 16; // default to 16 lines per field
    _datacastLines = 0; // no dedicated datacast lines

    _multiplexedSignalFlag = false; // using this would require changing all the line counting and a way to send full field through raspi-teletext - something for the distant future when everything else is done...
    
    _OutputFormat = T42; // t42 output is the default behaviour
    
    _PID = 0x20; // default PID is 0x20
    
    _packetServerPort = 0; // port 0 disables packet server
    _interfaceServerPort = 0;
    
    uint8_t priority[8]={9,3,3,6,3,3,5,6}; // 1=High priority,9=low. Note: priority[0] is mag 8
    
    for (int i=0; i<8; i++)
        _magazinePriority[i] = priority[i];

    //Scan the command line for overriding the pages file.
    if (argc>1)
    {
        for (int i=1;i<argc;++i)
        {
            std::string arg = argv[i];
            if (arg == "--dir")
            {
                if (i + 1 < argc)
                    _pageDir = argv[++i];
                else
                {
                    std::cerr << "--dir requires an argument\n";
                    exit(EXIT_FAILURE);
                }
            }
            else if (arg == "--format")
            {
                if (i + 1 < argc)
                {
                    arg = argv[++i];
                    
                    if (arg == "none")
                    {
                        _OutputFormat = None;
                    }
                    else if (arg == "t42")
                    {
                        _OutputFormat = T42;
                    }
                    else if (arg == "raw")
                    {
                        _OutputFormat = Raw;
                    }
                    else if (arg == "ts")
                    {
                        _OutputFormat = TS;
                    }
                    else if (arg == "tsnpts")
                    {
                        _OutputFormat = TSNPTS;
                    }
                    else
                    {
                        std::cerr << "invalid --format type\n";
                        exit(EXIT_FAILURE);
                    }
                    
                    if (_reverseBits && _OutputFormat != T42)
                    {
                        std::cerr << "--reverse requires t42 format\n";
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    std::cerr << "--format requires an argument\n";
                    exit(EXIT_FAILURE);
                }
            }
            else if (arg == "--reverse")
            {
                _reverseBits = true;
                
                if (_OutputFormat != T42)
                {
                    std::cerr << "--reverse requires t42 format\n";
                    exit(EXIT_FAILURE);
                }
            }
            else if (arg == "--pid")
            {
                if (i + 1 < argc)
                {
                    std::istringstream ss(argv[++i]);
                    ss >> _PID;
                    if (_PID < 0x20 || _PID >= 0x1FFF || !ss.eof())
                    {
                        std::cerr << "invalid PID\n";
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    std::cerr << "--pid requires an argument\n";
                    exit(EXIT_FAILURE);
                }
            }
            else if (arg == "--reserved")
            {
                if (i + 1 < argc)
                {
                    // Take a 32 bit hexadecimal value to set the four reserved bytes in the BSDP
                    // Store bytes big endian so that the order digits appear on the command line is the same as they appear in packet
                    errno = 0;
                    char *end_ptr;
                    unsigned long l = std::strtoul(argv[++i], &end_ptr, 16);
                    if (errno == 0 && *end_ptr == '\0')
                    {
                        _reservedBytes[0] = (l >> 24) & 0xff;
                        _reservedBytes[1] = (l >> 16) & 0xff;
                        _reservedBytes[2] = (l >> 8) & 0xff;
                        _reservedBytes[3] = (l >> 0) & 0xff;
                    }
                    else
                    {
                        std::cerr << "invalid reserved bytes argument\n";
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    std::cerr << "--reserved requires an argument\n";
                    exit(EXIT_FAILURE);
                }
            }
            else if (arg == "--debug")
            {
                if (i + 1 < argc)
                {
                    errno = 0;
                    char *end_ptr;
                    long l = std::strtol(argv[++i], &end_ptr, 10);
                    if (errno == 0 && *end_ptr == '\0' && l > -1)
                    {
                        switch(l){
                            case 0:
                                _debug->SetDebugLevel(Debug::LogLevels::logNONE);
                                break;
                            case 1:
                                _debug->SetDebugLevel(Debug::LogLevels::logERROR);
                                break;
                            case 2:
                                _debug->SetDebugLevel(Debug::LogLevels::logWARN);
                                break;
                            case 3:
                                _debug->SetDebugLevel(Debug::LogLevels::logINFO);
                                break;
                            case 4:
                            default:
                                _debug->SetDebugLevel(Debug::LogLevels::logDEBUG);
                                break;
                        }
                        
                    }
                    else
                    {
                        std::cerr << "invalid debug level argument\n";
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    std::cerr << "--debug requires an argument\n";
                    exit(EXIT_FAILURE);
                }
            }
            else if (arg == "--packetserver")
            {
                if (i + 1 < argc)
                {
                    errno = 0;
                    char *end_ptr;
                    long l = std::strtol(argv[++i], &end_ptr, 10);
                    if (errno == 0 && *end_ptr == '\0' && l > 0 && l < 65536)
                    {
                        _packetServerPort = (int)l;
                    }
                    else
                    {
                        std::cerr << "invalid server port number\n";
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    std::cerr << "--packetserver requires a port number\n";
                    exit(EXIT_FAILURE);
                }
            }
            else if (arg == "--datacast")
            {
                if (i + 1 < argc)
                {
                    errno = 0;
                    char *end_ptr;
                    long l = std::strtol(argv[++i], &end_ptr, 10);
                    if (errno == 0 && *end_ptr == '\0' && l > 0 && l < 65536)
                    {
                        _interfaceServerPort = (int)l;
                    }
                    else
                    {
                        std::cerr << "invalid server port number\n";
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    std::cerr << "--datacast requires a port number\n";
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                std::cerr << "unrecognised argument: " << arg << std::endl;
                exit(EXIT_FAILURE);
            }
        }
    }
    
    if (!DirExists(&_pageDir))
    {
        std::stringstream ss;
        ss << _pageDir << " does not exist or is not a directory\n";
        std::cerr << ss.str();
        exit(EXIT_FAILURE);
    }
    
    // TODO: allow overriding config file from command line
    _debug->Log(Debug::LogLevels::logINFO,"[Configure::Configure] Pages directory is " + _pageDir);
    _debug->Log(Debug::LogLevels::logINFO,"[Configure::Configure] Config file is " + _configFile);
    
    std::string path;
    path = _pageDir;
    path += "/";
    path += _configFile;
    LoadConfigFile(path); // load main config file (vbit.conf)
    
    LoadConfigFile(path+".override"); // allow overriding main config file for local configuration where main config is in version control
    
    if (_datacastLines > _linesPerField)
    {
        _debug->Log(Debug::LogLevels::logERROR,"[Configure] datacast lines cannot be greater than lines per field");
        _datacastLines = _linesPerField; // clamp
    }
}

Configure::~Configure()
{
    
}

void Configure::SetHeaderTemplate(std::string str)
{
    str.resize(32,' ');
    _headerTemplate.assign(str);
}

int Configure::LoadConfigFile(std::string filename)
{
    std::ifstream filein(filename.c_str()); // Open the file

    std::vector<std::string>::iterator iter;
    // these are all the valid strings for config lines
    std::vector<std::string> nameStrings{ "header_template", "initial_teletext_page", "row_adaptive_mode", "network_identification_code", "country_network_identification", "full_field", "status_display","lines_per_field","datacast_lines","magazine_priority"};

    if (filein.is_open())
    {
        _debug->Log(Debug::LogLevels::logINFO,"[Configure::LoadConfigFile] opened " + filename);

        std::string line;
        std::string name;
        std::string value;
        TTXLine* header = new TTXLine();
        while (std::getline(filein >> std::ws, line))
        {
            if (line.front() != ';') // ignore comments
            { 
                std::size_t delim = line.find("=", 0);
                int error = 0;

                if (delim != std::string::npos)
                {
                    name = line.substr(0, delim);
                    value = line.substr(delim + 1);
                    iter = find(nameStrings.begin(), nameStrings.end(), name);
                    if(iter != nameStrings.end())
                    {
                        // matched string
                        switch(iter - nameStrings.begin())
                        {
                            case 0: // header_template
                            {
                                header->Setm_textline(value,true); // use to process escape codes
                                SetHeaderTemplate(header->GetLine());
                                break;
                            }
                            case 1: // initial_teletext_page
                            {
                                if (value.size() >= 3)
                                {
                                    size_t idx;
                                    int magpage;
                                    int subcode;
                                    try
                                    {
                                        magpage = stoi(std::string(value, 0, 3), &idx, 16);
                                    }
                                    catch (const std::invalid_argument& ia)
                                    {
                                        error = 1;
                                        break;
                                    }
                                    if (magpage < 0x100 || magpage > 0x8FF || (magpage & 0xFF) == 0xFF)
                                    {
                                        error = 1;
                                        break;
                                    }
                                    if (value.size() > 3)
                                    {
                                        if ((value.size() != 8) || (value.at(idx) != ':'))
                                        {
                                            error = 1;
                                            break;
                                        }
                                        try
                                        {
                                            subcode = stoi(std::string(value, 4, 4), &idx, 16);
                                        }
                                        catch (const std::invalid_argument& ia)
                                        {
                                            error = 1;
                                            break;
                                        }
                                        if (subcode < 0x0000 || subcode > 0x3F7F || subcode & 0xC080)
                                        {
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
                            case 2: // row_adaptive_mode
                            {
                                if (!value.compare("true"))
                                {
                                    _rowAdaptive = true;
                                }
                                else if (!value.compare("false"))
                                {
                                    _rowAdaptive = false;
                                }
                                else
                                {
                                    error = 1;
                                }
                                break;
                            }
                            case 3: // "network_identification_code" - four character hex. eg. FA6F
                            {
                                if (value.size() == 4)
                                {
                                    size_t idx;
                                    try
                                    {
                                        _NetworkIdentificationCode = stoi(std::string(value, 0, 4), &idx, 16);
                                    }
                                    catch (const std::invalid_argument& ia)
                                    {
                                        error = 1;
                                        break;
                                    }
                                }
                                else
                                {
                                    error = 1;
                                }
                                break;
                            }
                            case 4: // "country_network_identification" - four character hex. eg. 2C2F
                            {
                                if (value.size() == 4)
                                {
                                    size_t idx;
                                    try
                                    {
                                        _CountryNetworkIdentificationCode = stoi(std::string(value, 0, 4), &idx, 16);
                                    }
                                    catch (const std::invalid_argument& ia)
                                    {
                                        error = 1;
                                        break;
                                    }
                                }
                                else
                                {
                                    error = 1;
                                }
                                break;
                            }
                            case 5: // "full_field"
                            {
                                break;
                            }
                            case 6: // "status_display"
                            {
                                SetServiceStatusString(value);
                                break;
                            }
                            case 7: // "lines_per_field"
                            {
                                if (value.size() > 0 && value.size() < 4)
                                {
                                    try
                                    {
                                        _linesPerField = stoi(std::string(value, 0, 3));
                                    }
                                    catch (const std::invalid_argument& ia)
                                    {
                                        error = 1;
                                        break;
                                    }
                                }
                                else
                                {
                                    error = 1;
                                }
                                break;
                            }
                            case 8: // datacast_lines
                            {
                                if (value.size() > 0 && value.size() < 4)
                                {
                                    try
                                    {
                                        _datacastLines = stoi(std::string(value, 0, 3));
                                    }
                                    catch (const std::invalid_argument& ia)
                                    {
                                        error = 1;
                                        break;
                                    }
                                }
                                else
                                {
                                    error = 1;
                                }
                                break;
                            }
                            case 9: // "magazine_priority"
                            {
                                std::stringstream ss(value);
                                std::string temps;
                                int tmp[8];
                                int i;
                                for (i=0; i<8; i++)
                                {
                                    if (std::getline(ss, temps, ','))
                                    {
                                        try
                                        {
                                            tmp[i] = stoi(temps);
                                        }
                                        catch (const std::invalid_argument& ia)
                                        {
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
                        }
                    }
                    else
                    {
                        error = 1; // unrecognised config line
                    }
                }
                if (error)
                {
                    _debug->Log(Debug::LogLevels::logERROR,"[Configure::LoadConfigFile] invalid config line: " + line);
                }
            }
        }
        filein.close();
        return 0;
    }
    else
    {
        _debug->Log(Debug::LogLevels::logWARN,"[Configure::LoadConfigFile] open failed");
        return -1;
    }
}
