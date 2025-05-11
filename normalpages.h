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

        std::shared_ptr<TTXPageStream> NextPage();

        void addPage(std::shared_ptr<TTXPageStream> p);

    protected:

    private:
        int _mag;
        PageList* _pageList;
        Debug* _debug;
        std::list<std::shared_ptr<TTXPageStream>> _NormalPagesList;
        std::list<std::shared_ptr<TTXPageStream>>::iterator _iter;
        std::shared_ptr<TTXPageStream> _page;
        bool _needSorting;
        
        template <typename TTXPageStream>
        struct pageLessThan
        {
            bool operator()(const std::shared_ptr<TTXPageStream>a, const std::shared_ptr<TTXPageStream>b) const{
                return a->GetPageNumber() < b->GetPageNumber();
            }
        };
};

}

#endif // _NORMALPAGES_H
