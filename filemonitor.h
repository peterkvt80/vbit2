#ifndef _FILEMONITOR_H_
#define _FILEMONITOR_H_

#include <iostream>
#include <sstream>
#include <thread>
#include <list>
#include <strings.h>
#include <sys/stat.h>

#include "configure.h"
#include "pagelist.h"
#include "packetmag.h"

/**
 * @brief Watches for changes to teletext page files and updates the page list as needed
 * www.ibm.com/developerworks/linux/library/l-ubuntu-inotify/index.html
 */

namespace vbit
{
    class FileMonitor
    {
        public:
            /** Default constructor */
            FileMonitor();
            FileMonitor(Configure *configure, Debug *debug, PageList *pageList);
            /** Default destructor */
            virtual ~FileMonitor();

            /**
             * Runs the monitoring thread and does not terminate (at least for now)
             * @return Nothing useful yet. Perhaps return an error status if something goes wrong
             */
            void run();

        protected:

        private:
            Configure* _configure; /// Member reference to the configuration settings
            Debug* _debug;
            PageList* _pageList;
            std::list<TTXPageStream*> _FilesList;
            int readDirectory(std::string path, bool firstrun=false);
            
            TTXPageStream* Locate(std::string filename);
            void ClearFlags();
            void DeleteOldPages();
            void Delete29AndHeader(TTXPageStream* page);
    };
}

#endif // _FILEMONITOR_H_
