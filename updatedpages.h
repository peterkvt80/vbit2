#ifndef _UPDATEDPAGES_H
#define _UPDATEDPAGES_H

#include <list>

#include "ttxpagestream.h"

// list of updated pages

namespace vbit
{

class UpdatedPages
{
    public:
        /** Default constructor */
        UpdatedPages();
        /** Default destructor */
        virtual ~UpdatedPages();

        TTXPageStream* NextPage();

        void addPage(TTXPageStream* p);
        
        bool waiting(){ return _UpdatedPagesList.size() > 0; };

    protected:

    private:
        std::list<TTXPageStream*> _UpdatedPagesList;
        std::list<TTXPageStream*>::iterator _iter;
        TTXPageStream* _page;
        
};

}

#endif // _UPDATEDPAGES_H
