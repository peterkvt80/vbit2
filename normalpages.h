#ifndef _NORMALPAGES_H
#define _NORMALPAGES_H

#include <list>
#include <mutex>

#include "debug.h"
#include "ttxpagestream.h"
#include "pagelist.h"

// list of normal pages

namespace vbit
{

class NormalPages
{
    public:
        /** Default constructor */
        NormalPages(int mag, PageList *pageList, Debug *debug);
        /** Default destructor */
        virtual ~NormalPages();

        TTXPageStream* NextPage();

        void addPage(TTXPageStream* p);

    protected:

    private:
        int _mag;
        PageList* _pageList;
        Debug* _debug;
        std::list<TTXPageStream*> _NormalPagesList;
        std::list<TTXPageStream*>::iterator _iter;
        TTXPageStream* _page;
        bool _needSorting;
        
        template <typename TTXPageStream>
        struct pageLessThan
        {
            bool operator()(const TTXPageStream *a, const TTXPageStream *b) const{
                return a->GetPageNumber() < b->GetPageNumber();
            }
        };
};

}

#endif // _NORMALPAGES_H
