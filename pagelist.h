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
#include "packetmag.h"

namespace ttx
{
    /** @brief A PageList maintains the set of all teletext pages in a teletext service
     *  It can load, save, add, delete, edit pages.
     *  Internally each magazine has its own list of pages.
     *
     */
    class PageList
    {
        public:
            /** @brief Create am empty page list
             */
            PageList(Configure *configure, vbit::Debug *debug);
            ~PageList();

            vbit::PacketMag **GetMagazines(){vbit::PacketMag **p=_mag;return p;};

            /** Return the page object that was loaded from <filename>
            * @param filename The filename of the page we are looking for.
            */
            TTXPageStream* Locate(std::string filename);

            // Probably want a nextPage function to scan using a wildcard

            /**
            * \brief Match - Find and mark all pages that match the page identity
            * \param page - A page identity string
            * \return - The number of pages that matched this identity
            */
            int Match(char* page);

            /** Add a teletext page to the proper magazine
            * @param page TTXPageStream object that has already been loaded
            */
            void AddPage(TTXPageStream* page, bool noupdate=false);
            
            /** Add a teletext page to the correct list for its type
            */
            void UpdatePageLists(TTXPageStream* page, bool noupdate=false);

            /** Clear all the exists flags
            */
            void ClearFlags();
            /** Delete all pages that no longer exist
            */
            void DeleteOldPages();


            /** \brief Iterate through all pages
            *  \return Returns the next page or nullptr if we are at the end
            */
            TTXPageStream* NextPage();

            /** \brief Iterate through all pages
            *  \return Returns the previous page or nullptr if we are at the beginning
            */
            TTXPageStream* PrevPage();

            /** \brief Last Page in the selected list
            *  \return Returns the last selected page or nullptr if there isn't one
            */
            TTXPageStream* LastPage();

            /** \brief Iterate through selected pages (using the P command)
            *  \return Returns the next page or nullptr if we are at the end
            */
            TTXPageStream* NextSelectedPage();

            /** \brief Reset the page iterator to the beginning
            *  Afterwards repeatedly call NextPage() until it returns nullptr
            *  \return the initial page or nullptr if there is no page
            */
            TTXPageStream* FirstPage();

            void CheckForPacket29OrCustomHeader(TTXPageStream* page);

        private:
            Configure* _configure; // The configuration object
            vbit::Debug* _debug;
            std::list<TTXPageStream*> _pageList[8]; /// The list of Pages in this service. One list per magazine
            vbit::PacketMag* _mag[8];

            // iterators through selected pages. (use the same iterator for D command and MD, L etc.)
            uint8_t _iterMag;  /// Magazine number for the iterator
            std::list<TTXPageStream*>::iterator _iter;  /// pages in a magazine
            TTXPageStream* _iterSubpage;    /// Subpages in a carousel
    };
}

#endif
