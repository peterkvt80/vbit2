#ifndef _CAROUSEL_H
#define _CAROUSEL_H

#include <list>

#include "debug.h"
#include "ttxpagestream.h"
#include "pagelist.h"

/** Carousel maintains a list of carousel pages.
 *  nextCarousel() iterates through the list to find pages which have hard cycle timings
 */

namespace vbit
{

class Carousel
{
    public:
        /** Default constructor */
        Carousel(int mag, PageList *pageList, Debug *debug);
        /** Default destructor */
        virtual ~Carousel();

        /** Add a page to the list
         */
        void addPage(TTXPageStream* p);

        /** Find the next carousel page that needs to be transmitted
         *  @return The next carousel if it is time to go or NULL
         */
        TTXPageStream* nextCarousel();


    protected:

    private:
        int _mag;
        PageList* _pageList;
        Debug* _debug;

        std::list<TTXPageStream*> _carouselList; /// The list of carousel pages
};

}

#endif // _CAROUSEL_H
