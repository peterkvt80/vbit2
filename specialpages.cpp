
#include "specialpages.h"

using namespace vbit;

SpecialPages::SpecialPages(int mag, PageList *pageList, Debug *debug) :
    _mag(mag),
    _pageList(pageList),
    _debug(debug)
{
    ResetIter();
}

SpecialPages::~SpecialPages()
{

}

void SpecialPages::addPage(TTXPageStream* p)
{
    p->SetSpecialFlag(true);
    _specialPagesList.push_front(p);
}

TTXPageStream* SpecialPages::NextPage()
{
    if (_page == nullptr)
    {
        ++_iter;
        _page = *_iter;
    }
    else
    {
        if (_page->IsCarousel())
        {
            _page->StepNextSubpageNoLoop();
            if (_page->GetCarouselPage() != NULL)
            {
                return _page;
            }
        }

        ++_iter;
        _page = *_iter;
    }

    while(true)
    {
        if (_iter == _specialPagesList.end())
        {
            _page = nullptr;
            return _page;
        }
        
        if (_page)
        {
            if (_page->GetLock()) // try to lock this page against changes
            {
                if (_page->IsCarousel() && _page->GetCarouselPage() == nullptr)
                {
                    _page->StepNextSubpageNoLoop(); // ensure we don't point at a null subpage
                }
                
                if ((_page->GetStatusFlag()==TTXPageStream::MARKED || _page->GetStatusFlag()==TTXPageStream::REMOVE) && _page->GetSpecialFlag()) // only remove it once
                {
                    _debug->Log(Debug::LogLevels::logINFO,"[SpecialPages::NextPage] Deleted " + _page->GetFilename());
                    _iter = _specialPagesList.erase(_iter);
                    _page->SetSpecialFlag(false);
                    _pageList->RemovePage(_page); // try to remove it from the pagelist immediately - will free the lock
                    _page = *_iter;
                    continue; // jump back to loop
                }
                else if (!(_page->Special()))
                {
                    std::stringstream ss;
                    ss << "[SpecialPages::NextPage()] no longer special " << std::hex << (_page->GetPageNumber() >> 8);
                    _debug->Log(Debug::LogLevels::logINFO,ss.str());
                    _iter = _specialPagesList.erase(_iter);
                    _page->SetSpecialFlag(false);
                }
                else if (((_page->GetPageNumber()>>8) & 0xFF) == 0xFF) // never return page mFF from the page list
                {
                    ++_iter;
                }
                else
                {
                    return _page; // return page locked
                }
                
                _page->FreeLock(); // must unlock page again
                _page = *_iter;
            }
        }
    }
}

void SpecialPages::ResetIter()
{
    _iter=_specialPagesList.begin();
    _page=nullptr;
}
