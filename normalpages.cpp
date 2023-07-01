
#include "normalpages.h"

using namespace vbit;

NormalPages::NormalPages()
{
    _iter=_NormalPagesList.begin();
    _page=nullptr;
    _needSorting = false;
}

NormalPages::~NormalPages()
{

}

void NormalPages::addPage(TTXPageStream* p)
{
    _NormalPagesList.push_front(p);
    _needSorting = true; // set flag to indicate that list should be sorted before next cycle
}

TTXPageStream* NormalPages::NextPage()
{
    if (_page == nullptr)
    {
        if (_needSorting)
        {
            // sort the page list by page number. Only check this at the start of each cycle
            _NormalPagesList.sort(pageLessThan<TTXPageStream>());
            _needSorting = false; // clear flag
        }
        _iter=_NormalPagesList.begin();
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
            std::stringstream ss;
            ss << "[NormalPages::NextPage] Deleted " << _page->GetSourcePage() << "\n";
            std::cerr << ss.str();
            _iter = _NormalPagesList.erase(_iter);
            _page->SetNormalFlag(false);
            if (!(_page->GetSpecialFlag() || _page->GetCarouselFlag() || _page->GetUpdatedFlag()))
                _page->SetState(TTXPageStream::GONE); // if we are last mark it gone
            _page = *_iter;
            goto loop; // jump back to try for the next page
        }
        
        if (_page->Special())
        {
            std::stringstream ss;
            ss << "[NormalPages::NextPage] page became Special"  << std::hex << _page->GetPageNumber() << "\n";
            std::cerr << ss.str();
            _iter = _NormalPagesList.erase(_iter);
            _page->SetNormalFlag(false);
            _page = *_iter;
            goto loop; // jump back to try for the next page
        }
        
        if (((_page->GetPageNumber()>>8) & 0xFF) == 0xFF){ // never return page mFF from the page list
            ++_iter;
            _page = *_iter;
            goto loop; // jump back to try for the next page
        }
    }
    
    return _page;
}
