#ifndef _SPECIALPAGES_H
#define _SPECIALPAGES_H

#include <list>

#include "debug.h"
#include "ttxpagestream.h"

// list of special pages

namespace vbit
{

class SpecialPages
{
    public:
        /** Default constructor */
        SpecialPages(Debug *debug);
        /** Default destructor */
        virtual ~SpecialPages();

        TTXPageStream* NextPage();
        
        void ResetIter();

        void addPage(TTXPageStream* p);

        void deletePage(TTXPageStream* p);


    protected:

    private:
        Debug* _debug;
        std::list<TTXPageStream*> _specialPagesList;
        std::list<TTXPageStream*>::iterator _iter;
        TTXPageStream* _page;
};

}

#endif // _SPECIALPAGES_H
