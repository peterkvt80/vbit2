#include "filemonitor.h"

using namespace vbit;

File::File(std::string filename) :
    _page(new TTXPageStream()),
    _filename(filename),
    _fileStatus(NEW)
{
    _page->LoadPage(filename);
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
        
        if (std::string(dirp->d_name).find(".tti") != std::string::npos)
        {
            // Now we want to process changes
            // 1) Is it a new page? Then add it.
            std::shared_ptr<File> f = Locate(name);
            if (f) // File was found
            {
                f->SetState(File::FOUND); // Mark this page as existing on the drive
                std::shared_ptr<TTXPageStream> page = f->GetPage();
                if (!(page->GetIsMarked())) // file is not mid-deletion
                {
                    if (attrib.st_mtime!=f->GetModifiedTime()) // File exists. Has it changed?
                    {
                        // We just load the new page and update the modified time
                        
                        if (page->GetLock()) // try to lock page
                        {
                            int curnum = page->GetPageNumber()>>8;
                            page->LoadPage(name);
                            if (page->GetPageNumber()>>8 != curnum)
                            {
                                // page number changed
                                page->MarkForDeletion(); // mark page for deletion from service
                                f->SetState(File::NOTFOUND); // let this file object get deleted and reloaded
                            }
                            else
                            {
                                page->IncrementUpdateCount();
                                int status = page->GetPageStatus();
                                if (!(_pageList->Contains(page)))
                                {
                                    // this page is not currently in pagelist
                                    _pageList->AddPage(page, !(status & PAGESTATUS_C8_UPDATE)); // noupdate unless C8 is set
                                }
                                else
                                {
                                    _pageList->UpdatePageLists(page, !(status & PAGESTATUS_C8_UPDATE)); // noupdate unless C8 is set
                                }
                                page->SetPageStatus(status | PAGESTATUS_C8_UPDATE); // always set C8 on an updated page
                                
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
                //if (!firstrun){ // suppress logspam on first run
                    _debug->Log(Debug::LogLevels::logINFO,"[FileMonitor::run] Adding a new page " + std::string(dirp->d_name));
                //}
                // A new file. Create the page object and add it to the page list.
                
                std::shared_ptr<File> f(new File(name));
                if (f)
                {
                    f->SetModifiedTime(attrib.st_mtime); // set timestamp
                    std::shared_ptr<TTXPageStream> page = f->GetPage();
                    
                    // don't add to updated pages list if this is the initial load
                    int status = page->GetPageStatus();
                    _pageList->AddPage(page, firstrun || !(status & PAGESTATUS_C8_UPDATE)); // noupdate unless C8 is set
                    page->SetPageStatus(status | PAGESTATUS_C8_UPDATE); // always set C8 on an newly loaded page
                    
                    _FilesList.push_back(f);
                }
                else
                {
                    _debug->Log(Debug::LogLevels::logWARN,"[FileMonitor::run] Failed to load" + std::string(dirp->d_name));
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
    int mag=(page->GetPageNumber() >> 16) & 0x7;
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
