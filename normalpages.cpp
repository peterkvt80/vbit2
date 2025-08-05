
#include "normalpages.h"

using namespace vbit;

NormalPages::NormalPages(int mag, PageList *pageList, Debug *debug) :
    _mag(mag),
    _pageList(pageList),
    _debug(debug)
{
    _iter=_NormalPagesList.begin();
    _page=nullptr;
}

NormalPages::~NormalPages()
{

}

void NormalPages::addPage(std::shared_ptr<TTXPageStream> p)
{
    p->SetNormalFlag(true);
    
    for (std::list<std::shared_ptr<TTXPageStream>>::iterator it=_NormalPagesList.begin();it!=_NormalPagesList.end();++it)
    {
        // find first page with a higher number
        std::shared_ptr<TTXPageStream> ptr = *it;
        if (ptr->GetPageNumber() > p->GetPageNumber())
        {
            _NormalPagesList.insert(it,p);
            return;
        }
    }
    // if we are here we ran to the end of the list without a match
    _NormalPagesList.push_back(p);
}

std::shared_ptr<TTXPageStream> NormalPages::NextPage()
{
    if (_page == nullptr)
    {
        _iter=_NormalPagesList.begin();
        _page = *_iter;
    }
    else
    {
        ++_iter;
        _page = *_iter;
    }

    while(true)
    {
        if (_iter == _NormalPagesList.end())
        {
            _page = nullptr;
            return _page;
        }
        
        if (_page)
        {
            if (_page->GetOneShotFlag())
            {
                _page->SetNormalFlag(false);
                _iter = _NormalPagesList.erase(_iter); // remove oneshot pages from the page list
                _page = *_iter;
                continue;
            }
            
            if (_page->GetLock()) // try to lock this page against changes
            {
                /* remove pointers from this list if the pages are marked for deletion */
                if (_page->GetIsMarked() && _page->GetNormalFlag()) // only remove it once
                {
                    std::stringstream ss;
                    ss << "[NormalPages::NextPage] Deleted " << std::hex << (_page->GetPageNumber());
                    _debug->Log(Debug::LogLevels::logINFO,ss.str());
                    _iter = _NormalPagesList.erase(_iter);
                    _page->SetNormalFlag(false);
                    _pageList->RemovePage(_page); // try to remove it from the pagelist immediately - will free the lock
                    _page = *_iter;
                    continue; // jump back to loop
                }
                else if (_page->Special())
                {
                    std::stringstream ss;
                    ss << "[NormalPages::NextPage] page became Special "  << std::hex << (_page->GetPageNumber());
                    _debug->Log(Debug::LogLevels::logINFO,ss.str());
                    _iter = _NormalPagesList.erase(_iter);
                    _page->SetNormalFlag(false);
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
