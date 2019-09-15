#ifndef _NORMALPAGES_H
#define _NORMALPAGES_H

#include <list>

#include "ttxpagestream.h"

// list of normal pages

namespace vbit
{

class NormalPages
{
    public:
        /** Default constructor */
        NormalPages();
        /** Default destructor */
        virtual ~NormalPages();

        TTXPageStream* NextPage();
        
        void ResetIter();

        void addPage(TTXPageStream* p);
        
        void sortPages();


    protected:

    private:
        std::list<TTXPageStream*> _NormalPagesList;
        std::list<TTXPageStream*>::iterator _iter;
        TTXPageStream* _page;
};

}

#endif // _NORMALPAGES_H
