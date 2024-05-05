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
        Carousel(Debug *debug);
        /** Default destructor */
        virtual ~Carousel();

        /** Access _pageNumber
         * \return The current value of _pageNumber
         */
        unsigned int Get_pageNumber() { return _pageNumber; }
        /** Set _pageNumber
         * \param val New value to set
         */
        void Set_pageNumber(unsigned int val) { _pageNumber = val; }
        /** Access page
         * \return The current value of page
         */
        TTXPageStream* Getpage() { return _page; }
        /** Set page
         * \param val New value to set
         */
        void Setpage(TTXPageStream* val) { _page = val; }
        /** Access nextPage
         * \return The current value of nextPage
         */
        time_t GetnextPage() { return _nextPage; }
        /** Set nextPage
         * \param val New value to set
         */
        void SetnextPage(time_t val) { _nextPage = val; }

        /* List management */

        /** Clear the carousel list
         */
        void clear();

        /** Add a page to the list
         */
        void addPage(TTXPageStream* p);

        /** Add a page to the list
         */
        void deletePage(TTXPageStream* p);

        /** Find the next carousel page that needs to be transmitted
         *  @return The next carousel if it is time to go or NULL
         */
        TTXPageStream* nextCarousel();


    protected:

    private:
        Debug* _debug;
        unsigned int _pageNumber; //!< Member variable "_pageNumber"
        TTXPageStream* _page; //!< Member variable "page"
        time_t _nextPage; //!< Member variable "nextPage"

        std::list<TTXPageStream*> _carouselList; /// The list of carousel pages

};

}

#endif // _CAROUSEL_H
