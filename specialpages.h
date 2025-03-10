#ifndef _SPECIALPAGES_H
#define _SPECIALPAGES_H

#include <list>

#include "debug.h"
#include "ttxpagestream.h"
#include "pagelist.h"

// list of special pages

namespace vbit
{

class SpecialPages
{
    public:
        /** Default constructor */
        SpecialPages(int mag, PageList *pageList, Debug *debug);
        /** Default destructor */
        virtual ~SpecialPages();

        TTXPageStream* NextPage();
        
        void ResetIter();

        void addPage(TTXPageStream* p);

    protected:

    private:
        int _mag;
        PageList* _pageList;
        Debug* _debug;
        std::list<TTXPageStream*> _specialPagesList;
        std::list<TTXPageStream*>::iterator _iter;
        TTXPageStream* _page;
};

}

#endif // _SPECIALPAGES_H
