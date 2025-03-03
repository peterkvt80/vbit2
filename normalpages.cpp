
#include "normalpages.h"

using namespace vbit;

NormalPages::NormalPages(int mag, std::list<TTXPageStream*>* pageSet, Debug *debug) :
    _mag(mag),
    _pageSet(pageSet),
    _debug(debug)
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
    p->SetNormalFlag(true);
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

    while(true)
    {
        if (_iter == _NormalPagesList.end())
        {
            _page = nullptr;
            return _page;
        }
        
        if (_page)
        {
            if (_page->GetLock()) // try to lock this page against changes
            {
                /* remove pointers from this list if the pages are marked for deletion */
                
                if ((_page->GetStatusFlag()==TTXPageStream::MARKED || _page->GetStatusFlag()==TTXPageStream::REMOVE) && _page->GetNormalFlag()) // only remove it once
                {
                    _debug->Log(Debug::LogLevels::logINFO,"[NormalPages::NextPage] Removed " + _page->GetFilename());
                    _iter = _NormalPagesList.erase(_iter);
                    _page->SetNormalFlag(false);
                    if (!(_page->GetSpecialFlag() || _page->GetCarouselFlag() || _page->GetUpdatedFlag()))
                    {
                        // we are last
                        _pageSet->remove(_page); // and remove it from the pageSet
                        _debug->SetMagazineSize(_mag, _pageSet->size());
                        
                        if (_page->GetStatusFlag()==TTXPageStream::REMOVE)
                            _page->SetState(TTXPageStream::FOUND);
                        else // MARKED
                            _page->SetState(TTXPageStream::GONE);
                    }
                } 
                else if (_page->Special())
                {
                    std::stringstream ss;
                    ss << "[NormalPages::NextPage] page became Special "  << std::hex << (_page->GetPageNumber() >> 8);
                    _debug->Log(Debug::LogLevels::logINFO,ss.str());
                    _iter = _NormalPagesList.erase(_iter);
                    _page->SetNormalFlag(false);
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
