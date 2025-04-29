#include "filemonitor.h"

using namespace vbit;

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
            TTXPageStream* q = Locate(name);
            if (q) // File was found
            {
                if (!(q->GetStatusFlag()==TTXPageStream::MARKED || q->GetStatusFlag()==TTXPageStream::REMOVE || q->GetStatusFlag()==TTXPageStream::GONE)) // file is not mid-deletion
                {
                    q->SetState(TTXPageStream::FOUND); // Mark this page as existing on the drive
                    if (attrib.st_mtime!=q->GetModifiedTime()) // File exists. Has it changed?
                    {
                        // We just load the new page and update the modified time
                        
                        q->LoadPage(name); // waits for mutex
                        q->IncrementUpdateCount();
                        int status = q->GetPageStatus();
                        if (!(_pageList->Contains(q)))
                        {
                            // this page is not currently in pagelist
                            _pageList->AddPage(q, !(status & PAGESTATUS_C8_UPDATE)); // noupdate unless C8 is set
                        }
                        else
                        {
                            _pageList->UpdatePageLists(q, !(status & PAGESTATUS_C8_UPDATE)); // noupdate unless C8 is set
                        }
                        q->SetPageStatus(status | PAGESTATUS_C8_UPDATE); // always set C8 on an updated page
                        
                        _pageList->CheckForPacket29OrCustomHeader(q);
                        
                        q->SetModifiedTime(attrib.st_mtime);
                        
                        q->FreeLock(); // must unlock or everything will grind to a halt
                        
                    }
                }
            }
            else
            {
                if (!firstrun){ // suppress logspam on first run
                    _debug->Log(Debug::LogLevels::logINFO,"[FileMonitor::run] Adding a new page " + std::string(dirp->d_name));
                }
                // A new file. Create the page object and add it to the page list.
                
                if ((q=new TTXPageStream(name)))
                {
                    // don't add to updated pages list if this is the initial load
                    int status = q->GetPageStatus();
                    _pageList->AddPage(q, firstrun || !(status & PAGESTATUS_C8_UPDATE)); // noupdate unless C8 is set
                    q->SetPageStatus(status | PAGESTATUS_C8_UPDATE); // always set C8 on an newly loaded page
                    
                    _FilesList.push_back(q);
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


// Find a page by filename
TTXPageStream* FileMonitor::Locate(std::string filename)
{
    for (std::list<TTXPageStream*>::iterator p=_FilesList.begin();p!=_FilesList.end();++p)
    {
        TTXPageStream* ptr = *p;
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
    for (std::list<TTXPageStream*>::iterator p=_FilesList.begin();p!=_FilesList.end();++p)
    {
        TTXPageStream* ptr = *p;
        // Don't unmark a file that was MARKED. Once condemned it won't be pardoned
        if (ptr->GetStatusFlag()==TTXPageStream::FOUND || ptr->GetStatusFlag()==TTXPageStream::NEW)
        {
            ptr->SetState(TTXPageStream::NOTFOUND);
        }
    }
}

void FileMonitor::DeleteOldPages()
{
    for (std::list<TTXPageStream*>::iterator p=_FilesList.begin();p!=_FilesList.end();++p)
    {
        TTXPageStream* ptr = *p;
        if (ptr->GetStatusFlag()==TTXPageStream::GONE)
        {
            Delete29AndHeader(ptr);
            
            _FilesList.remove(*p--);
            // page has been removed from lists so safe to delete
            delete ptr; // this finally deletes the pagestream/page object
        }
        else if (ptr->GetStatusFlag()==TTXPageStream::NOTFOUND)
        {
            if (!(_pageList->Contains(ptr)))
            {
                // Page is not in pagelist so won't be seen by service thread
                // delete it directly
                _debug->Log(Debug::LogLevels::logINFO,"[FileMonitor::DeleteOldPages] Deleted " + ptr->GetFilename());
                
                Delete29AndHeader(ptr);
                _FilesList.remove(*p--);
                // page has been removed from all lists so safe to delete
                delete ptr; // this finally deletes the pagestream/page object
            }
            else
            {
                // Pages marked here get deleted in the Service thread
                ptr->SetState(TTXPageStream::MARKED);
            }
        }
    }
}

void FileMonitor::Delete29AndHeader(TTXPageStream* page)
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
