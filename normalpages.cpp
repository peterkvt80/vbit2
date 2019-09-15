
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
        /* remove pointers from this list if the pages are marked for deletion */
        
        if (_page->GetStatusFlag()==TTXPageStream::MARKED && _page->GetNormalFlag()) // only remove it once
        {
            std::cerr << "[NormalPages::NextPage] Deleted " << _page->GetSourcePage() << std::endl;
            _iter = _NormalPagesList.erase(_iter);
            _page->SetNormalFlag(false);
            if (!(_page->GetSpecialFlag() || _page->GetCarouselFlag()))
                _page->SetState(TTXPageStream::GONE); // if we are last mark it gone
            _page = *_iter;
            goto loop; // jump back to try for the next page
        }
        
        if (_page->Special())
        {
            std::cerr << "[NormalPages::NextPage] page became Special"  << std::hex << _page->GetPageNumber() << std::endl;
            _iter = _NormalPagesList.erase(_iter);
            _page->SetNormalFlag(false);
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


