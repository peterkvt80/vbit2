
#include "updatedpages.h"

using namespace vbit;

UpdatedPages::UpdatedPages(Debug *debug) :
    _debug(debug)
{
    _iter=_UpdatedPagesList.begin();
    _page=nullptr;
}

UpdatedPages::~UpdatedPages()
{

}

void UpdatedPages::addPage(TTXPageStream* p)
{
    p->SetUpdatedFlag(true);
    _UpdatedPagesList.push_front(p);
}

TTXPageStream* UpdatedPages::NextPage()
{
    if (_page == nullptr)
    {
        _iter=_UpdatedPagesList.begin();
        _page = *_iter;
    }
    else
    {
        ++_iter;
        _page = *_iter;
    }

loop:
    if (_iter == _UpdatedPagesList.end())
    {
        _page = nullptr;
    }
    
    if (_page)
    {
        /* remove pointers from this list if the pages are marked for deletion */
        if (_page->GetStatusFlag()==TTXPageStream::MARKED && _page->GetUpdatedFlag()) // only remove it once
        {
            _debug->Log(Debug::LogLevels::logINFO,"[UpdatedPages::NextPage] Deleted " + _page->GetSourcePage());
            _iter = _UpdatedPagesList.erase(_iter);
            _page->SetUpdatedFlag(false);
            if (!(_page->GetSpecialFlag() || _page->GetCarouselFlag() || _page->GetNormalFlag()))
                _page->SetState(TTXPageStream::GONE); // if we are last mark it gone.
            _page = *_iter;
            goto loop; // jump back to try for the next page
        }
        
        _iter = _UpdatedPagesList.erase(_iter); // remove page from this list after transmitting it
        _page->SetUpdatedFlag(false);
    }
    
    return _page;
}
