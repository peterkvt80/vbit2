#ifndef _UPDATEDPAGES_H
#define _UPDATEDPAGES_H

#include <list>

#include "debug.h"
#include "ttxpagestream.h"
#include "pagelist.h"

// list of updated pages

namespace vbit
{

class UpdatedPages
{
    public:
        /** Default constructor */
        UpdatedPages(int mag, PageList *pageList, Debug *debug);
        /** Default destructor */
        virtual ~UpdatedPages();

        std::shared_ptr<TTXPageStream> NextPage();

        void addPage(std::shared_ptr<TTXPageStream> p);
        
        bool waiting(){ return _UpdatedPagesList.size() > 0; };

    protected:

    private:
        int _mag;
        PageList* _pageList;
        Debug* _debug;
        std::list<std::shared_ptr<TTXPageStream>> _UpdatedPagesList;
        std::list<std::shared_ptr<TTXPageStream>>::iterator _iter;
        std::shared_ptr<TTXPageStream> _page;
        
};

}

#endif // _UPDATEDPAGES_H
