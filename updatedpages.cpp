
#include "updatedpages.h"

using namespace vbit;

UpdatedPages::UpdatedPages(int mag, std::list<TTXPageStream*>* pageSet, Debug *debug) :
    _mag(mag),
    _pageSet(pageSet),
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

    while(true)
    {
        if (_iter == _UpdatedPagesList.end())
        {
            _page = nullptr;
            return _page;
        }
        
        if (_page)
        {
            if (_page->GetLock()) // try to lock this page against changes
            {
                /* remove pointers from this list if the pages are marked for deletion */
                if ((_page->GetStatusFlag()==TTXPageStream::MARKED || _page->GetStatusFlag()==TTXPageStream::REMOVE) && _page->GetUpdatedFlag()) // only remove it once
                {
                    _debug->Log(Debug::LogLevels::logINFO,"[UpdatedPages::NextPage] Deleted " + _page->GetFilename());
                    _iter = _UpdatedPagesList.erase(_iter);
                    _page->SetUpdatedFlag(false);
                    if (!(_page->GetSpecialFlag() || _page->GetCarouselFlag() || _page->GetNormalFlag()))
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
                else
                {
                    _iter = _UpdatedPagesList.erase(_iter); // remove page from this list after transmitting it
                    _page->SetUpdatedFlag(false);
                    return _page; // return locked page
                }
                
                _page->FreeLock(); // must unlock page again
                _page = *_iter;
            }
        }
    }
}
