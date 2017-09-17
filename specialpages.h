#ifndef _SPECIALPAGES_H
#define _SPECIALPAGES_H

#include <list>

#include "ttxpagestream.h"

// list of special pages

namespace vbit
{

class SpecialPages
{
    public:
        /** Default constructor */
        SpecialPages();
        /** Default destructor */
        virtual ~SpecialPages();

        TTXPageStream* NextPage();
        
        void ResetIter();

        void addPage(TTXPageStream* p);

        void deletePage(TTXPageStream* p);


    protected:

    private:
        std::list<TTXPageStream*> _specialPagesList;
        std::list<TTXPageStream*>::iterator _iter;
        TTXPageStream* _page;
};

}

#endif // _SPECIALPAGES_H
