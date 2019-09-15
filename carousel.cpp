
#include "carousel.h"

using namespace vbit;

Carousel::Carousel()
{
    //ctor
    //std::cerr << "[Carousel::Carousel] enters" << std::endl;
}

Carousel::~Carousel()
{
    //std::cerr << "[Carousel::Carousel] deleted";
    //dtor
}

void Carousel::addPage(TTXPageStream* p)
{
    // @todo Don't allow duplicate entries
    p->SetTransitionTime(p->GetCycleTime());
    _carouselList.push_front(p);
    //std::cerr << "[Carousel::addPage]";
}

void Carousel::deletePage(TTXPageStream* p)
{
    //std::cerr << "[Carousel::deletePage]";
    _carouselList.remove(p);
}

TTXPageStream* Carousel::nextCarousel()
{
    TTXPageStream* p;
    // std::cerr << "[nextCarousel] list size = " << _carouselList.size() << std::endl;
    if (_carouselList.size()==0) return NULL;


    for (std::list<TTXPageStream*>::iterator it=_carouselList.begin();it!=_carouselList.end();++it)
    {
        p=*it;
        if (p->GetStatusFlag()==TTXPageStream::MARKED)
        {
            std::cerr << "[Carousel::nextCarousel] Deleted " << p->GetSourcePage() << std::endl;
            p->SetState(TTXPageStream::GONE);
            _carouselList.remove(p);
            return nullptr;
        }
        else if ((!(p->IsCarousel())) || (p->Special()))
        {
            std::cerr << "[Carousel::nextCarousel] no longer a carousel " << std::hex << p->GetPageNumber() << std::endl;
            _carouselList.remove(p);
            p->SetCarouselFlag(false);
            return nullptr;
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
