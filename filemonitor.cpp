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

FileMonitor::FileMonitor(Configure *configure, PageList *pageList) :
	_configure(configure),_pageList(pageList)
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

/**
std::thread FileMonitor::run()
{
	t=new std::thread(&FileMonitor::worker, this); // Start the thread
  //t.join(); // Rejoin after the thread terminates
	return false;
}
*/

void FileMonitor::run()
{
	// @todo This thread will clash. They need proper protection.

  std::string path=_configure->GetPageDirectory() ; //
  std::cerr << "[FileMonitor::run] Monitoring " << path << std::endl;

  while (true)
  {
    DIR *dp;
    struct dirent *dirp;

   	// Open the directory
    if ( (dp = opendir(path.c_str())) == NULL)
    {
      std::cerr << "Error(" << errno << ") opening " << path << std::endl;
      return;
    }

		_pageList->ClearFlags(); // Assume that no files exist

    // Load the filenames into a list
    while ((dirp = readdir(dp)) != NULL)
    {
      // Select only pages that might be teletext. tti or ttix at the moment.
			// strcasestr doesn't seem to be in my Windows compiler.
#ifdef _WIN32
      char* p=strstr(dirp->d_name,".tti");
#else
      char* p=strcasestr(dirp->d_name,".tti");
#endif
//			std::cerr << path << "/" << dirp->d_name << std::endl;
      if (p)
      {
        std::string name;
        name=path;
        name+="/";
        name+=dirp->d_name;
        // Find the modification time
        struct stat attrib;         // create a file attribute structure
        stat(name.c_str(), &attrib);     // get the attributes of the file

        // struct tm* clock = gmtime(&(attrib.st_mtime)); // Get the last modified time and put it into the time structure
        // std::cerr << path << "/" << dirp->d_name << std::dec << " time:" << std::setw(2) << clock->tm_hour << ":" << std::setw(2) << clock->tm_min << std::endl;
        // Now we want to process changes
        // 1) Is it a new page? Then add it.
        TTXPageStream* p=_pageList->Locate(name);
        if (p) // File was found
        {
          //std::cerr << dirp->d_name << " was found" << std::endl;

          if (attrib.st_mtime!=p->GetModifiedTime()) // File exists. Has it changed?
          {
            std::cerr << "File has been modified" << dirp->d_name << std::endl;
            // We just load the new page and update the modified time
            // This isn't good enough.
            // We need a mutex or semaphore to lock out this page while we do that
            // lock
            p->LoadPage(name); // What if this fails? We can see the bool. What to do ?
			//TODO: If this was a normal page and is now a special page or vice versa we have loaded it into the wrong pagelist! FIX ME!
            p->SetModifiedTime(attrib.st_mtime);
            // unlock

          }
					p->SetState(TTXPageStream::FOUND); // Mark this page as existing on the drive
        }
        else
        {
          std::cerr << "[FileMonitor::run] " << " Adding a new page" << dirp->d_name << std::endl;
					// A new file. Create the page object and add it to the page list.
					if ((p=new TTXPageStream(name)))
					{
						_pageList->AddPage(p);
					}
					else
						std::cerr << "[FileMonitor::run] Failed to load" << dirp->d_name << std::endl;
        }
      }
    }
    closedir(dp);
    // std::cerr << "[FileMonitor::run] Finished scan" << std::endl;

		// Delete pages that no longer exist (this blocks the thread until the pages are removed)
		_pageList->DeleteOldPages();

    // Wait 5 seconds.
    // WARNING. We must allow enough time for Service to complete the delete or this process might crash
    // Sounds like a job for a mutex.
    struct timespec rec;
    int ms;

    ms=5000;
    rec.tv_sec = ms / 1000;
    rec.tv_nsec=(ms % 1000) *1000000;
    nanosleep(&rec,nullptr);
  }
} // run
