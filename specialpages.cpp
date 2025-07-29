
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

void SpecialPages::addPage(std::shared_ptr<TTXPageStream> p)
{
    p->SetSpecialFlag(true);
    for (std::list<std::shared_ptr<TTXPageStream>>::iterator it=_specialPagesList.begin();it!=_specialPagesList.end();++it)
    {
        // find first page with a higher number
        std::shared_ptr<TTXPageStream> ptr = *it;
        if (ptr->GetPageNumber() > p->GetPageNumber())
        {
            _specialPagesList.insert(it,p);
            return;
        }
    }
    // if we are here we ran to the end of the list without a match
    _specialPagesList.push_back(p);
}

std::shared_ptr<TTXPageStream> SpecialPages::NextPage()
{
    if (_page == nullptr)
    {
        ++_iter;
        _page = *_iter;
    }
    else
    {
        if (_page->GetLock()) // try to lock this page against changes
        {
            _page->StepNextSubpageNoLoop(); // next subpage if a carousel
            if (_page->GetSubpage() != nullptr) // make sure there is a subpage
            {
                return _page; // return page locked
            }
            _page->FreeLock(); // must unlock page again
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
                if (_page->GetSubpage() == nullptr)
                {
                    _page->StepNextSubpageNoLoop(); // step to first subpage
                }
                
                if (_page->GetIsMarked() && _page->GetSpecialFlag()) // only remove it once
                {
                    std::stringstream ss;
                    ss << "[SpecialPages::NextPage] Deleted " << std::hex << _page->GetPageNumber();
                    _debug->Log(Debug::LogLevels::logINFO,ss.str());
                    _iter = _specialPagesList.erase(_iter);
                    _page->SetSpecialFlag(false);
                    _pageList->RemovePage(_page); // try to remove it from the pagelist immediately - will free the lock
                    _page = *_iter;
                    continue; // jump back to loop
                }
                else if (!(_page->Special()))
                {
                    std::stringstream ss;
                    ss << "[SpecialPages::NextPage()] no longer special " << std::hex << _page->GetPageNumber();
                    _debug->Log(Debug::LogLevels::logINFO,ss.str());
                    _iter = _specialPagesList.erase(_iter);
                    _page->SetSpecialFlag(false);
                }
                else if ((_page->GetPageNumber() & 0xFF) == 0xFF) // never return page mFF from the page list
                {
                    ++_iter;
                }
                else if (_page->GetSubpageCount() == 0) // skip pages with no subpages
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
            else
            {
                // skip page
                ++_iter;
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
