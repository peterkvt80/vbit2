
#include "normalpages.h"

using namespace vbit;

NormalPages::NormalPages()
{
    ResetIter();
}

NormalPages::~NormalPages()
{

}

void NormalPages::addPage(TTXPageStream* p)
{
    _NormalPagesList.push_front(p);
}

TTXPageStream* NormalPages::NextPage()
{
    if (_page == nullptr)
    {
        ++_iter;
        _page = *_iter;
    }
    else
    {
        ++_iter;
        _page = *_iter;
    }

loop:
    if (_iter == _NormalPagesList.end())
    {
        _page = nullptr;
    }
    
    if (_page)
    {
        /* remove pointers from this list if the pages are marked for deletion, or have become 'Special' pages, then loop back to get the next page instead */
        
        if (_page->GetStatusFlag()==TTXPageStream::MARKED)
        {
            std::cerr << "[NormalPages::NextPage] Deleted " << _page->GetSourcePage() << std::endl;
            _page->SetState(TTXPageStream::GONE);
            _iter = _NormalPagesList.erase(_iter);
            _page = *_iter;
            goto loop; // jump back to try for the next page
        }
        
        if (_page->Special())
        {
            std::cerr << "[NormalPages::NextPage] page became Special"  << std::hex << _page->GetPageNumber() << std::endl;
            _iter = _NormalPagesList.erase(_iter);
            _page = *_iter;
            goto loop; // jump back to try for the next page
        }
    }
    
    return _page;
}

void NormalPages::ResetIter()
{
    _iter=_NormalPagesList.begin();
    _page=nullptr;
}

template <typename TTXPageStream>
struct pageLessThan
{
    bool operator()(const TTXPageStream *a, const TTXPageStream *b) const{
        return a->GetPageNumber() < b->GetPageNumber();
    }
};

void NormalPages::sortPages()
{
    // sort the page list by page number
    _NormalPagesList.sort(pageLessThan<TTXPageStream>());
}


