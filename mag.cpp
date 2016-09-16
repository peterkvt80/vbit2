#include "mag.h"

using namespace vbit;

Mag::Mag(int mag, std::list<TTXPageStream>* pageSet) :
    _pageSet(pageSet), _page(NULL), _magNumber(mag), _headerFlag(false)
{
    //ctor
    if (_pageSet->size()>0)
    {
        std::cerr << "[Mag::Mag] enters. page size=" << _pageSet->size() << std::endl;
        _it=_pageSet->begin();
        _it->DebugDump();
        _page=&*_it;
    }
    std::cerr << "[Mag::Mag] exits" << std::endl;
}

Mag::~Mag()
{
    std::cerr << "[Mag::Mag] ~ " << std::endl;
    //dtor
}

int Mag::GetPageCount()
{
    return _pageSet->size();
}

Packet *Mag::GetPacket()
{
    // This returns one packet at a time from a page.
    // We enter with _CurrentPage pointing to the first page
    std::cerr << "[Mag::GetPacket] called " << std::endl;

    // So what page will we send?
    // Pages are two types: single pages and carousels.
    // We won't implement an individual page timing. (ie. to make the index page appear more regularly)
    // The page selection algorithm will then be:
    /* 1) Every page starts off as non-carousel. A flag indicates this state.
     * 2) The page list is iterated through. If a page set has only one page then output that page.
     * 3) If it is flagged as a single page but has multiple pages, add it to a carousel list and flag as multiple.
     * 4) If it is flagged as a multiple page and has multiple pages then ignore it. Skip to the next single page.
     * 5) If it is flagged as a multiple page but only has one page, then delete it from the carousels list and flag as single page.
     * 6) However, before iterating in step 2, do this every second: Look at the carousel list and for each page decrement their timers.
     * When a page reaches 0 then it is taken as the next page, and its timer reset.
     */

     // If there are no pages, we don't have anything. @todo We could go quiet or filler.
    std::cerr << "[GetPacket] PageSize " << _pageSet->size() << std::endl;
/*
        vbit::Mag* m=_mag[i];
        std::list<TTXPageStream> p=m->Get_pageSet();
        for (std::list<TTXPageStream>::iterator it=p.begin();it!=p.end();++it)
*/
    // If there is no page, we should send a filler?
    if (_pageSet->size()<1)
        return new Packet();


    //std::cerr << "[GetPacket] DEBUG DUMP 1 " << std::endl;
    //_page->DebugDump();

    // To begin with, let's not do carousels. Only transmit single pages

    TTXLine* txt=_page->GetNextRow();


    _headerFlag=txt==NULL; // if last line was read

    if (_headerFlag)
    {
        std::cerr << "[GetPacket] Going to the next page " << std::endl;
        // We only deal with single pages as this is the main sequence
        // Iterate to the next page, and loop to beginning if needed
        ++_it;
        if (_it==_pageSet->end())
        {
            _it=_pageSet->begin();
        }
        // Get pointer to the page we are sending
        _page=&*_it;
        // If this page is not a single page then skip it.
        // Send a null to avoid the risk of getting stuck here.
        if (_page->Getm_SubPage()==NULL)
        {
            std::cerr << "[GetPacket] Ignoring carousels for now (todo) " << std::endl;
            return NULL;
        }
        std::cerr << "[GetPacket] Need to create header packet now " << std::endl;
        // Assemble the header
        int thisMag=_page->GetPageNumber();
        int thisRow=5; // @todo placeholder
        Packet* p=new Packet();
        p->Header(thisMag,thisPage,thisSubcode,ThisControl);// loads of stuff to do here!
        return p; /// @todo Should we do this?

    }
    else
        std::cerr << "[GetPacket] Sending " << txt->GetLine() << std::endl;



    // Go to the next page
    /* if (some terminating condition where we move to the next page
    if (last line was read)
    {
        ++_it;
        if (_it==_pageSet.end())
        {
            _it=_pageSet.begin();
        }
        // When we step the iterator we must initialise the iterators on the new page
        p->SetCurrentPage(p);
        p->GetCurrentPage()->SetLineCounter(1);
    }
*/
    std::cerr << "[Mag::GetPacket] Page description=" << _page->GetDescription() << std::endl;

//    Dump whole page list for debug purposes
/*
    for (_it=thing1;_it!=thing2;++_it)
    {
        std::cerr << "[Mag::GetPacket]Filename =" << _it->GetSourcePage() << std::endl;
    }
*/

    /// What do we need here? Static iterators? Member iterators?
    // If a page contains more than one subpage it is a carousel. subpage count>1
    // If a carousel is detected then the page gets a time and a page index.
    // I suppose we can pop a carousel onto a queue. If the time is passed
    // the carousel gets sent out and the page index is incremented.
    // A carousel is skipped in the main sequence.

    // Assemble the packet
    int thisMag=_page->GetPageNumber(); // Hmm, This looks wrong!
    int thisRow=5; // @todo placeholder
    Packet* p=new Packet(thisMag, thisRow, txt->GetLine());
    return p; /// @todo place holder. Need to implement
}

