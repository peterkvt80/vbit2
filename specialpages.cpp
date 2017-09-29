
#include "specialpages.h"

using namespace vbit;

SpecialPages::SpecialPages()
{
    ResetIter();
}

SpecialPages::~SpecialPages()
{

}

void SpecialPages::addPage(TTXPageStream* p)
{
    _specialPagesList.push_front(p);
}

void SpecialPages::deletePage(TTXPageStream* p)
{
    if (*_iter == p)
        _iter--; // if iterator is pointing at this page wind it back
    _specialPagesList.remove(p);
    _page = nullptr;
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
        if (_page->GetCarouselFlag())
        {
            _page->StepNextSubpageNoLoop();
            
            if (_page->GetCarouselPage() != NULL)
            {
                return _page;
            }
            else
            {
                // looped through all the carousel pages
                 _page->StepNextSubpage(); // loop back to beginning of carousel
            }
        }
        
        ++_iter;
        _page = *_iter;
    }

    if (_iter == _specialPagesList.end())
    {
        _page = nullptr;
    }
    
    return _page;
}

void SpecialPages::ResetIter()
{
    _iter=_specialPagesList.begin();
    _page=nullptr;
}
