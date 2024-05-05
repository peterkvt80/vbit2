/** ***************************************************************************
 * Description       : Class for a single teletext line
 * Compiler          : C++
 *
 * Copyright (C) 2014, Peter Kwan
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaims all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 *************************************************************************** **/
#include "filemonitor.h"

using namespace vbit;
using namespace ttx;

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

    while (true)
    {
        _pageList->ClearFlags(); // Assume that no files exist
        
        readDirectory(path);
        
        // Delete pages that no longer exist (this blocks the thread until the pages are removed)
        _pageList->DeleteOldPages();

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

int FileMonitor::readDirectory(std::string path)
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
            TTXPageStream* q=_pageList->Locate(name);
            if (q) // File was found
            {
                if (!(q->GetStatusFlag()==TTXPageStream::MARKED || q->GetStatusFlag()==TTXPageStream::GONE)) // file is not mid-deletion
                {
                    if (attrib.st_mtime!=q->GetModifiedTime()) // File exists. Has it changed?
                    {
                        // We just load the new page and update the modified time
                        // This isn't good enough.
                        // We need a mutex or semaphore to lock out this page while we do that
                        // lock
                        q->LoadPage(name); // What if this fails? We can see the bool. What to do ?
                        q->IncrementUpdateCount();
                        q->RenumberSubpages();
                        int mag=(q->GetPageNumber() >> 16) & 0x7;
                        
                        if ((!(q->GetSpecialFlag())) && (q->Special()))
                        {
                            // page was not 'special' but now is, add to SpecialPages list
                            q->SetSpecialFlag(true);
                            _pageList->GetMagazines()[mag]->GetSpecialPages()->addPage(q);
                            std::stringstream ss;
                            ss << "[FileMonitor::run] page was normal, is now special " << std::hex << q->GetPageNumber();
                            _debug->Log(Debug::LogLevels::logINFO,ss.str());
                            // page will be removed from NormalPages list by the service thread
                            // page will be removed from Carousel list by the service thread
                        }
                        else if ((q->GetSpecialFlag()) && (!(q->Special())))
                        {
                            // page was 'special' but now isn't, add to NormalPages list
                            _pageList->GetMagazines()[mag]->GetNormalPages()->addPage(q);
                            q->SetNormalFlag(true);
                            std::stringstream ss;
                            ss << "[FileMonitor::run] page was special, is now normal " << std::hex << q->GetPageNumber();
                            _debug->Log(Debug::LogLevels::logINFO,ss.str());
                        }
                        
                        if ((!(q->Special())) && (!(q->GetCarouselFlag())) && q->IsCarousel())
                        {
                            // 'normal' page was not 'carousel' but now is, add to Carousel list
                            q->SetCarouselFlag(true);
                            q->StepNextSubpage(); // ensure we're pointing at a subpage
                            _pageList->GetMagazines()[mag]->GetCarousel()->addPage(q);
                            std::stringstream ss;
                            ss << "[FileMonitor::run] page is now a carousel " << std::hex << q->GetPageNumber();
                            _debug->Log(Debug::LogLevels::logINFO,ss.str());
                        }
                        
                        if (q->GetNormalFlag() && !(q->GetSpecialFlag()) && !(q->GetCarouselFlag()) && !(q->GetUpdatedFlag()))
                        {
                            // add normal, non carousel pages to updatedPages list
                            _pageList->GetMagazines()[mag]->GetUpdatedPages()->addPage(q);
                            q->SetUpdatedFlag(true);
                        }
                        
                        _pageList->CheckForPacket29OrCustomHeader(q);
                        
                        q->SetModifiedTime(attrib.st_mtime);
                        // unlock
                    }
                    q->SetState(TTXPageStream::FOUND); // Mark this page as existing on the drive
                }
            }
            else
            {
                _debug->Log(Debug::LogLevels::logINFO,"[FileMonitor::run] Adding a new page " + std::string(dirp->d_name));
                // A new file. Create the page object and add it to the page list.
                
                if ((q=new TTXPageStream(name)))
                {
                    _pageList->AddPage(q);
                    if((q=_pageList->Locate(name))) // get pointer to copy in list
                    {
                        q->RenumberSubpages();
                        int mag=(q->GetPageNumber() >> 16) & 0x7;
                        if (q->Special())
                        {
                            // Page is 'special'
                            q->SetCarouselFlag(false);
                            q->SetSpecialFlag(true);
                            q->SetNormalFlag(false);
                            q->SetUpdatedFlag(false);
                            _pageList->GetMagazines()[mag]->GetSpecialPages()->addPage(q);
                        }
                        else
                        {
                            // Page is 'normal'
                            q->SetSpecialFlag(false);
                            q->SetNormalFlag(true);
                            _pageList->GetMagazines()[mag]->GetNormalPages()->addPage(q);
                            
                            if (q->IsCarousel())
                            {
                                // Page is also a 'carousel'
                                q->SetCarouselFlag(true);
                                q->StepNextSubpage(); // ensure we're pointing at a subpage
                                _pageList->GetMagazines()[mag]->GetCarousel()->addPage(q);
                            }
                            else
                            {
                                q->SetCarouselFlag(false);
                                // add normal, non carousel pages to updatedPages list
                                _pageList->GetMagazines()[mag]->GetUpdatedPages()->addPage(q);
                                q->SetUpdatedFlag(true);
                            }
                        }
                        
                        _pageList->CheckForPacket29OrCustomHeader(q);
                    }
                    else
                    {
                        _debug->Log(Debug::LogLevels::logERROR,"[FileMonitor::run] Failed to add" + std::string(dirp->d_name)); // should never happen
                    }
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
