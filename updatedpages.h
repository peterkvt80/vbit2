#ifndef _UPDATEDPAGES_H
#define _UPDATEDPAGES_H

#include <list>

#include "debug.h"
#include "ttxpagestream.h"

// list of updated pages

namespace vbit
{

class UpdatedPages
{
    public:
        /** Default constructor */
        UpdatedPages(Debug *debug);
        /** Default destructor */
        virtual ~UpdatedPages();

        TTXPageStream* NextPage();

        void addPage(TTXPageStream* p);
        
        bool waiting(){ return _UpdatedPagesList.size() > 0; };

    protected:

    private:
        Debug* _debug;
        std::list<TTXPageStream*> _UpdatedPagesList;
        std::list<TTXPageStream*>::iterator _iter;
        TTXPageStream* _page;
        
};

}

#endif // _UPDATEDPAGES_H
