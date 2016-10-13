#include "carousel.h"

using namespace vbit;

Carousel::Carousel()
{
    //ctor
    std::cerr << "[Carousel::Carousel] enters" << std::endl;
}

Carousel::~Carousel()
{
    std::cerr << "[Carousel::Carousel] deleted";
    //dtor
}

void Carousel::addPage(TTXPageStream* p)
{
    // @todo Don't allow duplicate entries
    p->SetTransitionTime();
    _carouselList.push_front(p);
    std::cerr << "[Carousel::addPage]";
}

void Carousel::deletePage(TTXPageStream* p)
{
    std::cerr << "[Carousel::deletePage]";
    _carouselList.remove(p);
}

TTXPageStream* Carousel::nextCarousel()
{
    TTXPageStream* p;
    //std::cerr << "[nextCarousel] list size = " << _carouselList.size() << std::endl;
    if (_carouselList.size()==0) return NULL;


    for (std::list<TTXPageStream*>::iterator it=_carouselList.begin();it!=_carouselList.end();++it)
    {
        p=*it;

        if (p->Expired())
        {
            std::cerr << "[Carousel::nextCarousel] page " << std::hex << p->GetPageNumber() << std::dec << " cycle time=" << p->GetCycleTime() << std::endl;
            p->SetTransitionTime(); // We found a carousel that is ready to step
            // @todo fiddle the iterators to select the next page
            break;
        }
        p=NULL;
    }
#if 0
    char c;
    std::cin >> c;
    if (c=='x')
        exit(3);
#endif
    return p;
}
