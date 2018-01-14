
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
    
    if (_iter == _specialPagesList.end())
    {
        _page = nullptr;
    }
	else if (_page->IsCarousel() && _page->GetCarouselPage() == NULL)
	{
		_page->StepNextSubpageNoLoop(); // ensure we don't point at a null subpage
	}
    
    return _page;
}

void SpecialPages::ResetIter()
{
    _iter=_specialPagesList.begin();
    _page=nullptr;
}
