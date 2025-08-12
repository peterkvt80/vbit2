#include "filemonitor.h"

using namespace vbit;

File::File(std::string filename) :
    _page(new TTXPageStream()),
    _filename(filename),
    _fileStatus(NEW)
{
    LoadFile(filename);
}

void File::LoadFile(std::string filename)
{
    _loaded = false;
    if (filename.size() >= 4)
    {
        std::string ext = filename.substr(filename.size() - 4); // get last four characters of string
        
        if (ext == ".tti")
        {
            _loaded = LoadTTI(filename);
        }
        // else other types of file we might want to load in future
    }
}

bool File::LoadTTI(std::string filename)
{
    const std::string cmd[]={"DS","SP","DE","CT","PN","SC","PS","MS","OL","FL","RD","RE","PF"};
    const int cmdCount = 13; // There are 13 possible commands, maybe DT and RT too on really old files
    unsigned int lineNumber;
    int lines=0;
    // Open the file
    std::ifstream filein(filename.c_str());
    _page->ClearPage(); // reset to blank page
    char * ptr;
    unsigned int subcode;
    int pageNumber = 0;
    int pagestatus = 0;
    char timedmode = false;
    int cycletime = 1;
    int region = 0;
    char m;
    
    /* We're going to create subpages in parallel with the old system for a moment... */
    std::shared_ptr<Subpage> s = nullptr;
    
    for (std::string line; std::getline(filein, line, ','); )
    {
        // This shows the command code:
        bool found=false;
        for (int i=0;i<cmdCount && !found; i++)
        {
            if (!line.compare(cmd[i]))
            {
                found=true;
                switch (i)
                {
                    case 0 : // "DS"
                    case 1 : // "SP"
                    case 2 : // "DE"
                    case 7 : // "MS" - Mask
                    case 10 : // "RD" - not sure!
                    {
                        std::getline(filein, line); // consume line
                        break;
                    }
                    case 3 : // "CT" - Cycle time (seconds)
                    {
                        // CT,8,T
                        std::getline(filein, line, ',');
                        cycletime = (atoi(line.c_str()));
                        if (cycletime < 1)
                            cycletime = 1;
                        else if (cycletime > 255)
                            cycletime = 255;
                        
                        std::getline(filein, line);
                        timedmode = (line[0]=='T'?true:false);
                        
                        if (s != nullptr)
                        {
                            s->SetTimedMode(timedmode);
                            s->SetCycleTime(cycletime);
                        }
                        
                        break;
                    }
                    case 4 : // "PN" - Page Number mppss
                    {
                        // Where m=1..8
                        // pp=00 to ff (hex)
                        // ss=00 to 99 (decimal)
                        // PN,10000
                        
                        std::getline(filein, line);
                        if (!pageNumber) // use the first page number we see
                        {
                            if (line.length()<3) // Must have at least three characters for a page number
                                break;
                            m=line[0];
                            if (m<'1' || m>'8') // Magazine must be 1 to 8
                                break;
                            pageNumber=std::strtol(line.c_str(), &ptr, 16);
                            if (line.length()<5 && pageNumber<=0x8ff) // Page number without subpage? Shouldn't happen but you never know.
                            {
                                // leave it alone and hope for the best
                            }
                            else   // Normally has subpage digits, we don't care
                            {
                                pageNumber=(pageNumber & 0xfff00) >> 8;
                            }
                            
                            _page->SetPageNumber(pageNumber);
                        }
                        
                        s = std::shared_ptr<Subpage>(new Subpage()); // create a new subpage
                        // inherit settings from previous subpage
                        s->SetTimedMode(timedmode);
                        s->SetCycleTime(cycletime);
                        s->SetSubpageStatus(pagestatus);
                        s->SetRegion(region);
                        _page->AppendSubpage(s); // add it to the page

                        break;
                    }
                    case 5 : // "SC" - Subcode
                    {
                        // SC,0000
                        std::getline(filein, line);
                        subcode=std::strtol(line.c_str(), &ptr, 16);
                        
                        if (s != nullptr)
                            s->SetSubCode(subcode); // set subcode explicitly
                        
                        break;
                    }
                    case 6 : // "PS" - Page status flags
                    {
                        // PS,8000
                        std::getline(filein, line);
                        pagestatus = std::strtol(line.c_str(), &ptr, 16);
                        if (s != nullptr)
                            s->SetSubpageStatus(pagestatus);
                        
                        break;
                    }
                    case 8 : // "OL" - Output line
                    {
                        std::getline(filein, line, ',');
                        lineNumber=atoi(line.c_str());
                        std::getline(filein, line);
                        if (lineNumber>MAXROW) break;
                        if (s != nullptr)
                        {
                            std::shared_ptr<TTXLine> ttxline(new TTXLine(line));
                            s->SetRow(lineNumber,ttxline);
                            lines++;
                        }
                        
                        // check for and decode OL,28 page function and coding
                        if (lineNumber == 28 && line.length() >= 40)
                        {
                            uint8_t dc = line.at(0) & 0x0F;
                            if (dc == 0 || dc == 2 || dc == 3 || dc == 4)
                            {
                                // packet is X/28/0, X/28/2, X/28/3, or X/28/4
                                int triplet = line.at(1) & 0x3F;
                                triplet |= (line.at(2) & 0x3F) << 6;
                                triplet |= (line.at(3) & 0x3F) << 12; // first triplet contains page function and coding
                                
                                // Page function and coding override previous values
                                _page->SetPageFunctionInt(triplet & 0x0F);
                                _page->SetPageCodingInt((triplet & 0x70) >> 4);
                            }
                        }
                        
                        break;
                    }
                    case 9 : // "FL"; - Fastext links
                    {
                        std::array<FastextLink,6> links;
                        
                        for (int fli=0;fli<6;fli++)
                        {
                            if (fli<5)
                                std::getline(filein, line, ',');
                            else
                                std::getline(filein, line); // Last parameter no comma
                            
                            links[fli].page = std::strtol(line.c_str(), &ptr, 16);
                            links[fli].subpage = 0x3f7f;
                        }
                        
                        if (s != nullptr)
                        {
                            s->SetFastext(links);
                        }
                        break;
                    }
                    case 11 : // "RE"; - Set page region code 0..f
                    {
                        std::getline(filein, line);
                        int region = std::strtol(line.c_str(), &ptr, 16);
                        if (s != nullptr)
                            s->SetRegion(region);
                        break;
                    }
                    case 12 : // "PF"; - not in the tti spec, page function and coding
                    {
                        std::getline(filein, line);
                        if (line.length()<3)
                        {
                            // invalid page function/coding
                        }
                        else
                        {
                            _page->SetPageFunctionInt(std::strtol(line.substr(0,1).c_str(), &ptr, 16));
                            _page->SetPageCodingInt(std::strtol(line.substr(2,1).c_str(), &ptr, 16));
                        }
                        break;
                    }
                    default:
                    {
                        // line not understood
                    }
                } // switch
            } // if matched command
            // If the command was not found then skip the rest of the line
        } // seek command
        if (!found) std::getline(filein, line);
    }
    filein.close(); // Not sure that we need to close it
    _page->RenumberSubpages();
    return (lines>0);
}

FileMonitor::FileMonitor(Configure *configure, Debug *debug, PageList *pageList) :
    _configure(configure),
    _debug(debug),
    _pageList(pageList)
{
    //ctor
}

FileMonitor::FileMonitor()
    : _pageList(nullptr)
{
    //ctor
}

FileMonitor::~FileMonitor()
{
    //dtor
}

void FileMonitor::run()
{
    std::string path=_configure->GetPageDirectory() ;
    _debug->Log(Debug::LogLevels::logINFO,"[FileMonitor::run] Monitoring " + path);
    
    readDirectory(path, true);

    while (true)
    {
        ClearFlags(); // Assume that no files exist
        
        readDirectory(path);
        
        // Delete pages that no longer exist (this blocks the thread until the pages are removed)
        DeleteOldPages();

        // Wait 5 seconds to avoid hogging cpu
        // Sounds like a job for a mutex.
        struct timespec rec;
        int ms;

        ms=5000;
        rec.tv_sec = ms / 1000;
        rec.tv_nsec=(ms % 1000) *1000000;
        nanosleep(&rec,nullptr);
    }
} // run

int FileMonitor::readDirectory(std::string path, bool firstrun)
{
    struct dirent *dirp;
    struct stat attrib;
    
    DIR *dp;

    // Open the directory
    if ( (dp = opendir(path.c_str())) == NULL)
    {
        _debug->Log(Debug::LogLevels::logERROR,"Error(" + std::to_string(errno) + ") opening " + path);
        return errno;
    }
    
    // Load the filenames into a list
    while ((dirp = readdir(dp)) != NULL)
    {
        std::string name;
        name=path;
        name+="/";
        name+=dirp->d_name;
        if (stat(name.c_str(), &attrib) == -1) // get the attributes of the file
            continue; // skip file on failure
        
        if (attrib.st_mode & S_IFDIR)
        {
            // directory entry is another directory
            if (dirp->d_name[0] != '.') // ignore anything beginning with .
            {
                if (readDirectory(name)) // recurse into directory
                {
                    _debug->Log(Debug::LogLevels::logERROR,"Error(" + std::to_string(errno) + ") recursing into " + name);
                }
            }
            continue;
        }
        
        const std::vector<std::string> filetypes{".tti"}; // the filetypes we will attempt to load
        if (name.size() >= 4)
        {
            std::string ext = name.substr(name.size() - 4); // get last four characters of string
            if (find(filetypes.begin(), filetypes.end(), ext) != filetypes.end())
            {
                // Now we want to process changes
                // 1) Is it a new page? Then add it.
                std::shared_ptr<File> f = Locate(name);
                if (f) // File was found
                {
                    f->SetState(File::FOUND); // Mark this page as existing on the drive
                    std::shared_ptr<TTXPageStream> page = f->GetPage();
                    if (attrib.st_mtime!=f->GetModifiedTime()) // File exists. Has it changed?
                    {
                        if (page->GetIsMarked()) // file is mid-deletion
                        {
                            f->SetState(File::NOTFOUND); // let this file object get deleted and reloaded
                        }
                        else
                        {
                            // We just load the new page and update the modified time
                            if (page->GetLock()) // try to lock page
                            {
                                int curnum = page->GetPageNumber();
                                f->LoadFile(name);
                                if (page->GetPageNumber() != curnum)
                                {
                                    // page number changed
                                    page->MarkForDeletion(); // mark page for deletion from service
                                    f->SetState(File::NOTFOUND); // let this file object get deleted and reloaded
                                }
                                else
                                {
                                    if (f->Loaded())
                                    {
                                        if (page->GetOneShotFlag())
                                        {
                                            // file load clears oneshot status
                                            page->SetOneShotFlag(false);
                                            _debug->Log(Debug::LogLevels::logINFO,"[FileMonitor::run] Reloading page from " + std::string(dirp->d_name));
                                        }
                                        
                                        page->IncrementUpdateCount();
                                        int update = false;
                                        
                                        page->StepFirstSubpage(); // Only check update flag on first subpage. Carousels don't get pushed out anyway
                                        if (std::shared_ptr<Subpage> s = page->GetSubpage())
                                            update = (s->GetSubpageStatus() & PAGESTATUS_C8_UPDATE);
                                        
                                        if (!(_pageList->Contains(page)))
                                        {
                                            // this page is not currently in pagelist
                                            _pageList->AddPage(page, !update); // only transmit immediate update if update flag is set
                                        }
                                        else
                                        {
                                            _pageList->UpdatePageLists(page, !update); // only transmit immediate update if update flag is set
                                        }
                                        
                                        page->StepLastSubpage(); // prepare for page to roll to first subpage
                                    }
                                    else
                                    {
                                        _debug->Log(Debug::LogLevels::logWARN,"[FileMonitor::run] Failed to load " + std::string(dirp->d_name));
                                        page->MarkForDeletion(); // mark page for deletion from service
                                    }
                                    
                                    _pageList->CheckForPacket29OrCustomHeader(page);
                                    
                                    f->SetModifiedTime(attrib.st_mtime);
                                }
                                page->FreeLock(); // must unlock or everything will grind to a halt
                            }
                        }
                    }
                }
                else
                {
                    if (!firstrun){ // suppress logspam on first run
                        _debug->Log(Debug::LogLevels::logINFO,"[FileMonitor::run] Adding a new page " + std::string(dirp->d_name));
                    }
                    // A new file. Create the page object and add it to the page list.
                    
                    std::shared_ptr<File> f(new File(name));
                    f->SetModifiedTime(attrib.st_mtime); // set timestamp
                    if (f->Loaded())
                    {
                        std::shared_ptr<TTXPageStream> page = f->GetPage();
                        
                        // don't add to updated pages list if this is the initial startup
                        int update = false;
                        if (std::shared_ptr<Subpage> s = page->GetSubpage())
                            update = (s->GetSubpageStatus() & PAGESTATUS_C8_UPDATE) && !page->IsCarousel(); // only check update flag of single subpages
                        _pageList->AddPage(page, firstrun | !update); // only transmit immediate update if update flag is set and vbit2 isn't starting up
                        _pageList->CheckForPacket29OrCustomHeader(page);
                    }
                    else
                    {
                        _debug->Log(Debug::LogLevels::logWARN,"[FileMonitor::run] Failed to load " + std::string(dirp->d_name));
                    }
                    _FilesList.push_back(f);
                }
            }
        }
    }
    closedir(dp);
    
    return 0;
}


// Find a file by filename
std::shared_ptr<File> FileMonitor::Locate(std::string filename)
{
    for (std::list<std::shared_ptr<File>>::iterator p=_FilesList.begin();p!=_FilesList.end();++p)
    {
        std::shared_ptr<File> ptr = *p;
        if (filename==ptr->GetFilename())
            return ptr;
    }
    
    return NULL; // @todo placeholder What should we do here?
}

// Detect pages that have been deleted from the drive
// Do this by first clearing all the "exists" flags
// As we scan through the list, set the "exists" flag as we match up the drive to the loaded page
void FileMonitor::ClearFlags()
{
    for (std::list<std::shared_ptr<File>>::iterator p=_FilesList.begin();p!=_FilesList.end();++p)
    {
        std::shared_ptr<File> ptr = *p;

        ptr->SetState(File::NOTFOUND);
    }
}

void FileMonitor::DeleteOldPages()
{
    for (std::list<std::shared_ptr<File>>::iterator p=_FilesList.begin();p!=_FilesList.end();++p)
    {
        std::shared_ptr<File> ptr = *p;
        if (ptr->GetStatusFlag()==File::NOTFOUND)
        {
            ptr->GetPage()->MarkForDeletion(); // mark page for deletion from service
            _debug->Log(Debug::LogLevels::logINFO,"[FileMonitor::DeleteOldPages] Deleted " + ptr->GetFilename());
                
            Delete29AndHeader(ptr->GetPage());
            _FilesList.remove(*p--); // remove file from filelist
        }
    }
}

void FileMonitor::Delete29AndHeader(std::shared_ptr<TTXPageStream> page)
{
    int mag=(page->GetPageNumber() >> 8) & 0x7;
    if (page->GetPacket29Flag())
    {
        // Packet 29 was loaded from this page, so remove it.
        _pageList->GetMagazines()[mag]->DeletePacket29();
        _debug->Log(Debug::LogLevels::logINFO,"[PageList::DeleteOldPages] Removing packet 29 from magazine " + std::to_string((mag == 0)?8:mag));
    }
    if (page->GetCustomHeaderFlag())
    {
        // Custom header was loaded from this page, so remove it.
        _pageList->GetMagazines()[mag]->DeleteCustomHeader();
    }
}
