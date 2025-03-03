#ifndef _CAROUSEL_H
#define _CAROUSEL_H

#include <list>

#include "debug.h"
#include "ttxpagestream.h"

/** Carousel maintains a list of carousel pages.
 *  Each list entry is a page number, a page object and a time
 *  When pages are added and removed we must make sure that they are also updated here.
 *  When the actual time exceeds the nextPage value, we select the next page.
 */

namespace vbit
{

class Carousel
{
    public:
        /** Default constructor */
        Carousel(int mag, std::list<TTXPageStream*>* pageSet, Debug *debug);
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
        std::list<TTXPageStream*>* _pageSet; // the parent pagelist for this magazine
        Debug* _debug;

        std::list<TTXPageStream*> _carouselList; /// The list of carousel pages
};

}

#endif // _CAROUSEL_H
