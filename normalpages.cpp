
#include "normalpages.h"

using namespace vbit;

NormalPages::NormalPages()
{
    ResetIter();
}

NormalPages::~NormalPages()
{

}

void NormalPages::addPage(TTXPageStream* p)
{
    _NormalPagesList.push_front(p);
}

void NormalPages::deletePage(TTXPageStream* p)
{
    _NormalPagesList.remove(p);
    ResetIter();
}

TTXPageStream* NormalPages::NextPage()
{
    if (_page == nullptr)
    {
        ++_iter;
        _page = *_iter;
    }
    else
    {
        ++_iter;
        _page = *_iter;
    }
    
    if (_iter == _NormalPagesList.end())
    {
        _page = nullptr;
    }
    
    if (_page)
    {
        if (_page->GetStatusFlag()==TTXPageStream::MARKED)
        {
            std::cerr << "[NormalPages::NextPage] Deleted " << _page->GetSourcePage() << std::endl;
            _page->SetState(TTXPageStream::GONE);
            _NormalPagesList.remove(_page);
            ResetIter();
            return nullptr;
        }
    }
    
    return _page;
}

void NormalPages::ResetIter()
{
    _iter=_NormalPagesList.begin();
    _page=nullptr;
}
