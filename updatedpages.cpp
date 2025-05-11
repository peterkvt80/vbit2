
#include "updatedpages.h"

using namespace vbit;

UpdatedPages::UpdatedPages(int mag, PageList *pageList, Debug *debug) :
    _mag(mag),
    _pageList(pageList),
    _debug(debug)
{
    _iter=_UpdatedPagesList.begin();
    _page=nullptr;
}

UpdatedPages::~UpdatedPages()
{

}

void UpdatedPages::addPage(std::shared_ptr<TTXPageStream> p)
{
    p->SetUpdatedFlag(true);
    _UpdatedPagesList.push_front(p);
}

std::shared_ptr<TTXPageStream> UpdatedPages::NextPage()
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
        else
        {
            _page = *_iter;
        }
        
        if (_page)
        {
            if (_page->GetLock()) // try to lock this page against changes
            {
                // don't care if page has been marked for deletion/removal
                // if we were put into this list, we want to get transmitted regardless
                
                _iter = _UpdatedPagesList.erase(_iter); // remove page from this list after transmitting it
                _page->SetUpdatedFlag(false);
                return _page; // return locked page
            }
        }
    }
}
