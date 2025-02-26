
#include "specialpages.h"

using namespace vbit;

SpecialPages::SpecialPages(Debug *debug) :
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

void SpecialPages::deletePage(TTXPageStream* p)
{
    _specialPagesList.remove(p);
    ResetIter();
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

loop:
    if (_iter == _specialPagesList.end())
    {
        _page = nullptr;
    }
    else if (_page->IsCarousel() && _page->GetCarouselPage() == NULL)
    {
        _page->StepNextSubpageNoLoop(); // ensure we don't point at a null subpage
    }
    
    if (_page)
    {
        if (_page->GetStatusFlag()==TTXPageStream::MARKED && _page->GetSpecialFlag()) // only remove it once
        {
            _debug->Log(Debug::LogLevels::logINFO,"[SpecialPages::NextPage] Deleted " + _page->GetSourcePage());
            _page->SetState(TTXPageStream::GONE);
            _iter = _specialPagesList.erase(_iter);
            _page->SetSpecialFlag(false);
            if (!(_page->GetNormalFlag() || _page->GetCarouselFlag() || _page->GetUpdatedFlag()))
                _page->SetState(TTXPageStream::GONE); // if we are last mark it gone
            _page = *_iter;
            goto loop;
        }
        else if (!(_page->Special()))
        {
            std::stringstream ss;
            ss << "[SpecialPages::NextPage()] no longer special " << std::hex << (_page->GetPageNumber() >> 8);
            _debug->Log(Debug::LogLevels::logINFO,ss.str());
            _iter = _specialPagesList.erase(_iter);
            _page->SetSpecialFlag(false);
            _page = *_iter;
            goto loop;
        }
        
        if (((_page->GetPageNumber()>>8) & 0xFF) == 0xFF){ // never return page mFF from the page list
            ++_iter;
            _page = *_iter;
            goto loop; // jump back to try for the next page
        }
    }
    
    return _page;
}

void SpecialPages::ResetIter()
{
    _iter=_specialPagesList.begin();
    _page=nullptr;
}
