#ifndef _MAG_H_
#define _MAG_H_

/** Handles streaming from one magazine.
 *  It decides the next packet to transmit in this particular magazine.
 *  Packets from single pages are treated differently from carousels.
 *  Single pages go into a simple cycle where there are transmitted in sequence.
 *  Carousel pages are transmitted at a higher priority and go out according to a timed interval.
 *
 *  Pages are kept in memory. If a new page is added to the PageList then it may change
 *  from single to carousel. This change is detected and the state of the page is changed accordingly.
 *  How we detect a single/carousel is if the root page has a child.
 *
 *  The magazine is a list of pages.
 *
 *  Ideally this should all be const reference as we don't want to change anything in the page...
 *  However, it makes a mess and won't compile.
 *  Until we discover the secret of using const,
 *  the rule here is *never* create an instance or copy of a page here, always use pointer references.
 *  It might be an idea to create a copy constructor that exits so that it isn't inadvertently called.
 *
 */
#include <iostream>
#include <list>
#include "ttxpagestream.h"
#include "packet.h"
#include "carousel.h"

namespace vbit
{

class Mag
{
    public:
        /** Default constructor */
        Mag(int mag, std::list<TTXPageStream>* pageSet);
        /** Default destructor */
        virtual ~Mag();

        /** Access _pageSet
         * \return The current value of _pageSet
         */
        std::list<TTXPageStream>*  Get_pageSet() { return _pageSet; }
        /** GetPacket
         * \return the next packet in the magazine stream
         */
        Packet* GetPacket() ;
        /** Set _pageSet
         * \param val New value to set
         */
      //  void Set_pageSet(std::list<TTXPageStream> val) { _pageSet = val; }

        /** Get page count
         * \return Number of pages in pageSet
         */
        int GetPageCount();

        /** Header flag
         * \return true if the last packet was a header
         */
        inline bool GetHeaderFlag(){return _headerFlag;};

    protected:
    private:
        std::list<TTXPageStream>*  _pageSet; //!< Member variable "_pageSet"
        std::list<TTXPageStream>::iterator _it;
        TTXPageStream* _page; //!< The current page being output
        int _magNumber; //!< The number of this magazine. (where 0 is mag 8)
        bool _headerFlag; //!< True if the last packet was a header

        /**
         * If a carousel page is due to go out we return the page to transmit with pointers set up to the next row.
         * If there are no carousel pages due then return NULL, which means that the next single page goes instead.
         * @return Carousel page complete with a pointer to the first row of the page. NULL if no carousel is due.
         */
        TTXPageStream* GetCarousel();

        Carousel* _carousel;
};

}

#endif // _MAG_H_
