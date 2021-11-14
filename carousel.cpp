
#include "carousel.h"

using namespace vbit;

Carousel::Carousel()
{
    //ctor
}

Carousel::~Carousel()
{
    //dtor
}

void Carousel::addPage(TTXPageStream* p)
{
    // @todo Don't allow duplicate entries
    p->SetTransitionTime(p->GetCycleTime());
    _carouselList.push_front(p);
}

void Carousel::deletePage(TTXPageStream* p)
{
    _carouselList.remove(p);
}

TTXPageStream* Carousel::nextCarousel()
{
    TTXPageStream* p;
    if (_carouselList.size()==0) return NULL;
    
    for (std::list<TTXPageStream*>::iterator it=_carouselList.begin();it!=_carouselList.end();++it)
    {
        p=*it;
        if (p->GetStatusFlag()==TTXPageStream::MARKED && p->GetCarouselFlag()) // only remove it once
        {
            std::stringstream ss;
            ss << "[Carousel::nextCarousel] Deleted " << p->GetSourcePage() << "\n";
            std::cerr << ss.str();
            
            p->SetCarouselFlag(false);
            if (!(p->GetNormalFlag() || p->GetSpecialFlag() || _page->GetUpdatedFlag()))
                p->SetState(TTXPageStream::GONE); // if we are last mark it gone
            _carouselList.erase(it--);
        }
        else if ((!(p->IsCarousel())) || (p->Special()))
        {
            std::stringstream ss;
            ss << "[Carousel::nextCarousel] no longer a carousel " << std::hex << p->GetPageNumber() << "\n";
            std::cerr << ss.str();
            
            p->SetCarouselFlag(false);
            _carouselList.erase(it--);
        }
        else
        {
            if (p->Expired())
            {
                // We found a carousel that is ready to step
                if (p->GetCarouselPage()->GetPageStatus() & PAGESTATUS_C9_INTERRUPTED)
                {
                    // carousel should go out now out of sequence
                    return p;
                }
            }
        }
        p=nullptr;
    }
    return p;
}
