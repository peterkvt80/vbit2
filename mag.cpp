#include "mag.h"

using namespace vbit;

Mag::Mag(int mag, std::list<TTXPageStream>* pageSet) :
    _pageSet(pageSet), _page(NULL), _magNumber(mag), _headerFlag(false), _thisRow(0), _state(STATE_HEADER)
{
    //ctor
    if (_pageSet->size()>0)
    {
        //std::cerr << "[Mag::Mag] enters. page size=" << _pageSet->size() << std::endl;
        _it=_pageSet->begin();
        //_it->DebugDump();
        _page=&*_it;
    }
    _carousel=new vbit::Carousel();
    //std::cerr << "[Mag::Mag] exits" << std::endl;
}

Mag::~Mag()
{
    //std::cerr << "[Mag::Mag] ~ " << std::endl;
    delete _carousel;
    //dtor
}

int Mag::GetPageCount()
{
    return _pageSet->size();
}

/** The next page in a carousel */
TTXPageStream* Mag::GetCarouselPage()
{
    TTXPageStream* p=_carousel->nextCarousel();
    /*
    if (p==NULL)
    {
        std::cerr << "There are no carousels ready to go at the moment" << std::endl;
    }
    */
    return p; // @todo Look at the carousel list and see if one is ready go
}

Packet* Mag::GetPacket()
{
    int thisPage;
		int thisSubcode;
		int thisMag;
		Packet* p=NULL;
		int thisStatus;
		int* links=NULL;

		static vbit::Packet* filler=new Packet(8,25,"                                        "); // filler
    // Returns one packet at a time from a page.
    // We enter with _CurrentPage pointing to the first page
    // std::cerr << "[Mag::GetPacket] called " << std::endl;

    // So what page will we send?
    // Pages are two types: single pages and carousels.
    // We won't implement non-carousel page timing. (ie. to make the index page appear more often)
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
    // std::cerr << "[GetPacket] PageSize " << _pageSet->size() << std::endl;
/*
        vbit::Mag* m=_mag[i];
        std::list<TTXPageStream> p=m->Get_pageSet();
        for (std::list<TTXPageStream>::iterator it=p.begin();it!=p.end();++it)
*/
    // If there is no page, we should send a filler
    if (_pageSet->size()<1)
    {
      _outp("V"); // If this goes wrong we are getting _pageSet wrong
        return filler; // @todo make this a filler (or quiet or NULL)
    }

    //std::cerr << "[GetPacket] DEBUG DUMP 1 " << std::endl;
    //_page->DebugDump();

    TTXLine* txt;

		_headerFlag=false;

		// If this is row 0, we MUST be doing a header
		/*
		if (state==STATE_HEADER)
		{
			if (_page->GetLineCounter()>0)
				std::cerr << "Mag=" << _magNumber << " LineCounter=" << _page->GetLineCounter() << std::endl;
		}
*/
    std::string s;
    /*
    if (_magNumber==5)
    {
        std::cout << "*S" << (int)_state << "*";
    }
    */
		switch (_state)
		{
		case STATE_HEADER: // Decide which page goes next
          _outp("h");
				_headerFlag=true;
        _page=GetCarouselPage(); // Is there a carousel page due?


        if (_page) // Carousel? Step to the next subpage
				{
          _outp("c");
					_page->StepNextSubpage();
				}
				else  // No carousel? Take the next page in the main sequence
        {
          _outp("n");
            ++_it;
            if (_it==_pageSet->end())
            {
                _it=_pageSet->begin();
            }
            // Get pointer to the page we are sending
            _page=&*_it;
        }
				// @todo: This assert fails!
        // assert(_page->GetPageNumber()>>16 == _magNumber); // Make sure that we always point to the correct magazine

				_thisRow=0;

				// When a single page is changed into a carousel
        if (_page->IsCarousel() != _page->GetCarouselFlag())
        {
            _page->SetCarouselFlag(_page->IsCarousel());
            if (_page->IsCarousel())
            {
                // std::cerr << "This page has become a carousel. Add it to the list" << std::endl;
                _carousel->addPage(_page);
            }
            else
            {
                std::cerr << "@todo This page has no longer a carousel. Remove it from the list" << std::endl;
                exit(3); //
            }
        }

        // Assemble the header. (we can simplify this code or leave it for the optimiser)
        thisPage=_page->GetPageNumber();
				thisPage=(thisPage/0x100) % 0x100; // Remove this line for Continuous Random Acquisition of Pages.
        thisSubcode=_page->GetSubCode();
        thisStatus=_page->GetPageStatus();
        p=new Packet();
        p->Header(_magNumber,thisPage,thisSubcode,thisStatus);// loads of stuff to do here!

      //p->HeaderText("CEEFAX 1 MPP DAY DD MTH 12:34.56"); // Placeholder 32 characters. This gets replaced later
        p->HeaderText("CEEFAX 1 %%# %%a %d %%b 12:34.56"); // Placeholder 32 characters. This gets replaced later


        p->Parity(13);
				assert(p!=NULL);

				_state=STATE_FASTEXT;
				break;

		case STATE_FASTEXT:
			links=_page->GetLinkSet();
			p=new Packet(); // @TODO. Worry about the heap!
			p->SetMRAG(_magNumber,27);
			p->Fastext(links,_magNumber);
			_state=STATE_PACKET26;
			break;
		case STATE_PACKET26:
			_state=STATE_PACKET27;
			if (false) break; // Put the real code in here
		case STATE_PACKET27:
			_state=STATE_PACKET28;
			if (false) break; // Put the real code in here
		case STATE_PACKET28:
			_state=STATE_TEXTROW;
			if (false) break; // Fall through until we put the real code in here
		case STATE_TEXTROW:
		  // Find the next row that isn't NULL
		  for (_thisRow++;_thisRow<=24;_thisRow++)
      {
        txt=_page->GetTxRow(_thisRow);
        if (txt!=NULL)
                  break;
      }
      // Didn't find? End of this page.
		  if (_thisRow>24 || txt==NULL)
			{
				p=NULL;
				_state=STATE_HEADER;
				_thisRow=0;
				_outp("H");
			}
			else
			{
        _outp("J");
				if (txt->GetLine().empty() && false) // If the row is empty then skip it (Kill this for now)
				{
					// std::cerr << "[Mag::GetPacket] Empty row" << std::hex << _page->GetPageNumber() << std::dec << std::endl;
					p=NULL;
				}
				else
				{
					// Assemble the packet
					thisMag=_magNumber;

					p=new Packet(thisMag, _thisRow, txt->GetLine());
					p->Parity();
					assert(p->IsHeader()!=true);
				}
			}

			break;
		} // switch
    if (p==NULL) _outp("q");
    return p;
}


void Mag::_outp(std::string s)
{
  return;
  if (_magNumber==5)
    std::cout << s;
}

