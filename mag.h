#ifndef _MAG_H_
#define _MAG_H_

/** Handles streaming from one magazine.
 *  The magazine is a list of pages.
 *
 *  Ideally this should all be const reference as we don't want to change anything in the page...
 *  However, it makes a mess and won't compile.
 *  Until we discover the secret of using const,
 *  the rule here is *never* create an instance or copy of a page here, always use pointer references,
 *
 */
#include <iostream>
#include <list>
#include "ttxpagestream.h"
#include "packet.h"

namespace vbit
{

class Mag
{
    public:
        /** Default constructor */
        Mag(std::list<TTXPageStream>* pageSet);
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

    protected:
    private:
        std::list<TTXPageStream>*  _pageSet; //!< Member variable "_pageSet"
        std::list<TTXPageStream>::iterator _it;
        TTXPageStream* _page; //!< The current page being output
};

}

#endif // _MAG_H_
