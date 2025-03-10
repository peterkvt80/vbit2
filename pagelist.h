#ifndef _PAGELIST_H_
#define _PAGELIST_H_

#include <iostream>
#include <string>
#include <dirent.h>
#include <errno.h>
#include <vector>
#include <list>

#include "configure.h"
#include "debug.h"
#include "ttxpagestream.h"

namespace vbit
{
    class PacketMag; // forward declaration

    /** @brief A PageList maintains the set of all teletext pages in a teletext service
     *  Internally each magazine has its own list of pages.
     */
    class PageList
    {
        public:
            /** @brief Create am empty page list
             */
            PageList(Configure *configure, Debug *debug);
            ~PageList();

            PacketMag **GetMagazines(){PacketMag **p=_mag;return p;};

            /** Return the page object with the specified page number */
            TTXPageStream* Locate(int PageNumber);
            
            bool Contains(TTXPageStream* page);

            /** Add a teletext page to the proper magazine */
            void AddPage(TTXPageStream* page, bool noupdate=false);
            
            void RemovePage(TTXPageStream* page);
            
            /** Add a teletext page to the correct list for its type */
            void UpdatePageLists(TTXPageStream* page, bool noupdate=false);

            void CheckForPacket29OrCustomHeader(TTXPageStream* page);
            
            int GetSize(int mag);

        private:
            Configure* _configure; // The configuration object
            Debug* _debug;
            std::list<TTXPageStream*> _pageList[8]; /// The list of Pages in this service. One list per magazine
            PacketMag* _mag[8];
    };
}

#endif
