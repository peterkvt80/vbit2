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
  : _pageList(NULL)
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
  std::cerr << "[FileMonitor::run] File monitoring started" << std::endl;

  std::string path=_configure->GetPageDirectory() ; //
  std::cerr << "[FileMonitor::run] Monitoring " << path << std::endl;

  while (true)
  {
    DIR *dp;
    struct dirent *dirp;

    std::cerr << "tick ";

   	// Open the directory
    if ( (dp = opendir(path.c_str())) == NULL)
    {
      std::cerr << "Error(" << errno << ") opening " << path << std::endl;
      return;
    }

    // Load the filenames into a list
    while ((dirp = readdir(dp)) != NULL)
    {
      // Select only pages that might be teletext. tti or ttix at the moment.
      char* p=strcasestr(dirp->d_name,".tti");
			std::cerr << path << "/" << dirp->d_name << std::endl;
      if (p)
      {
        std::string name;
        name=path;
        name+="/";
        name+=dirp->d_name;
        // Find the modification time
				struct tm* clock;               // create a time structure
				struct stat attrib;         // create a file attribute structure
				stat(name.c_str(), &attrib);     // get the attributes of the file
				clock = gmtime(&(attrib.st_mtime)); // Get the last modified time and put it into the time structure

        std::cerr << path << "/" << dirp->d_name << std::dec << " time:" << clock->tm_hour << ":" << clock->tm_min << std::endl;
      }
    }
    closedir(dp);
    std::cerr << "FINISHED LOADING PAGES" << std::endl;

    // Wait for ms/1000 seconds
    struct timespec rec;
    int ms=5000;
    rec.tv_sec = ms / 1000;
    rec.tv_nsec=(ms % 1000) *1000000;
    nanosleep(&rec,NULL);
  }
} // run
