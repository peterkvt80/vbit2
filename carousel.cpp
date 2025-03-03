
#include "carousel.h"

using namespace vbit;

Carousel::Carousel(Debug *debug) :
    _debug(debug)
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
    p->SetCarouselFlag(true);
    p->SetTransitionTime(p->GetCycleTime()+1);
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
        if (p->GetLock()) // try to lock this page against changes
        {
            if (p->GetStatusFlag()==TTXPageStream::MARKED && p->GetCarouselFlag()) // only remove it once
            {
                _debug->Log(Debug::LogLevels::logINFO,"[Carousel::nextCarousel] Deleted " + p->GetFilename());
                
                p->SetCarouselFlag(false);
                if (!(p->GetNormalFlag() || p->GetSpecialFlag() || p->GetUpdatedFlag()))
                    p->SetState(TTXPageStream::GONE); // if we are last mark it gone
                _carouselList.erase(it--);
            }
            else if ((!(p->IsCarousel())) || (p->Special()))
            {
                std::stringstream ss;
                ss << "[Carousel::nextCarousel] no longer a carousel " << std::hex << (p->GetPageNumber() >> 8);
                _debug->Log(Debug::LogLevels::logINFO,ss.str());
                
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
                        return p; // return page locked
                    }
                }
            }
            
            p->FreeLock(); // unlock
        }
    }
    return nullptr;
}
