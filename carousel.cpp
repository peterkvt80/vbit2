
#include "carousel.h"

using namespace vbit;

Carousel::Carousel(int mag, PageList *pageList, Debug *debug) :
    _mag(mag),
    _pageList(pageList),
    _debug(debug)
{
    //ctor
}

Carousel::~Carousel()
{
    //dtor
}

void Carousel::addPage(std::shared_ptr<TTXPageStream> p)
{
    // @todo Don't allow duplicate entries
    p->SetCarouselFlag(true);
    p->SetTransitionTime(p->GetCycleTime()+1);
    _carouselList.push_front(p);
}

std::shared_ptr<TTXPageStream> Carousel::nextCarousel()
{
    std::shared_ptr<TTXPageStream> p;
    if (_carouselList.size()==0) return NULL;
    
    for (std::list<std::shared_ptr<TTXPageStream>>::iterator it=_carouselList.begin();it!=_carouselList.end();++it)
    {
        p=*it;
        if (p->GetLock()) // try to lock this page against changes
        {
            if (p->GetIsMarked() && p->GetCarouselFlag()) // only remove it once
            {
                std::stringstream ss;
                ss << "[Carousel::nextCarousel] Deleted " << std::hex << (p->GetPageNumber() >> 8);
                _debug->Log(Debug::LogLevels::logINFO,ss.str());
                
                p->SetCarouselFlag(false);
                _carouselList.erase(it--);
                
                _pageList->RemovePage(p); // try to remove it from the pagelist immediately - will free the lock
                continue; // jump back to loop
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
