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
#include "ttxpagestream.h"

namespace vbit
{
    class File
    {
        public:
            enum Status
            {
              NEW,      // Just created
              NOTFOUND, // Not found yet
              FOUND     // Matched on drive
            };
            
            File(std::string filename);
            std::shared_ptr<TTXPageStream> GetPage(){return _page;};
            
            // The time that the file was modified.
            time_t GetModifiedTime(){return _modifiedTime;};
            void SetModifiedTime(time_t timeVal){_modifiedTime=timeVal;};
            
            void SetState(Status state){_fileStatus=state;};
            Status GetStatusFlag(){return _fileStatus;};
            
            std::string GetFilename() const {return _filename;}
            
            void LoadFile(std::string filename);
            bool Loaded(){return _loaded;}
            
        private:
            std::shared_ptr<TTXPageStream> _page; // the page loaded from this file
            std::string _filename;
            time_t _modifiedTime;   /// Poll this in case the source file changes (Used to detect updates)
            Status _fileStatus; /// Used to mark if we found the file. (Used to detect deletions)
            bool LoadTTI(std::string filename);
            bool _loaded;
    };
    
    /**
     * Watches for changes to teletext page files and adds them to the page list or marks them for removal
     */
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
            std::list<std::shared_ptr<File>> _FilesList;
            int readDirectory(std::string path, bool firstrun=false);
            
            std::shared_ptr<File> Locate(std::string filename);
            void ClearFlags();
            void DeleteOldPages();
            void Delete29AndHeader(std::shared_ptr<TTXPageStream> page);
    };
}

#endif // _FILEMONITOR_H_
