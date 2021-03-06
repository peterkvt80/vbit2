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

        void addPage(TTXPageStream* p);

    protected:

    private:
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
