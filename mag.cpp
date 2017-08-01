#include "mag.h"

using namespace vbit;

Mag::Mag(int mag, std::list<TTXPageStream>* pageSet, ttx::Configure *configure) :
    _pageSet(pageSet),
    _configure(configure),
    _page(NULL),
    _magNumber(mag),
    _headerFlag(false),
    _thisRow(0),
    _state(STATE_HEADER),
    _lastTxt(NULL)
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

Packet* Mag::GetPacket(Packet* p)
{
  int thisPageNum;
  unsigned int thisSubcode;
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
   
   /* pages can be have varying packet codings for X/1 to X/25. Pages using the standard coding we
      want to send X/26/0 to X/26/15 first, then X/1 to X/25 afterwards.
      For pages which carry data not for display in packets X/1 to X/25 however we must send these first (so the packets including any X/26 are all sent in sequential order of packet and designation code
      Currently any page using a coding other than 7-bit with parity sends in this sequential order */
/*
        vbit::Mag* m=_mag[i];
        std::list<TTXPageStream> p=m->Get_pageSet();
        for (std::list<TTXPageStream>::iterator it=p.begin();it!=p.end();++it)
*/
  // If there is no page, we should send a filler
  if (_pageSet->size()<1)
  {
    //_outp("V"); // If this goes wrong we are getting _pageSet wrong
    return filler;
  }

  //std::cerr << "[GetPacket] DEBUG DUMP 1 " << std::endl;
  //_page->DebugDump();

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
    //_outp("h");
    _headerFlag=true;
    _page=_carousel->nextCarousel(); // The next carousel page

    // Is this page deleted?
    if (_page && _page->GetStatusFlag()==TTXPageStream::MARKED)
    {
      std::cerr << "[Mag::GetPacket] Delete this carousel" << std::endl;
      _carousel->deletePage(_page);
      _page=NULL;
      // @todo We are not done. This just deletes a pointer to the page. It is still in _pageList
    }


    if (_page) // Carousel? Step to the next subpage
    {
      //_outp("c");
      _page->StepNextSubpage();
			//std::cerr << "[Mag::GetPacket] Header thisSubcode=" << std::hex << _page->GetCarouselPage()->GetSubCode() << std::endl;

    }
    else  // No carousel? Take the next page in the main sequence
    {
      // std::cerr << "X";
      if (_it==_pageSet->end())
      {
        std::cerr << "This can not happen" << std::endl;
        exit(0);
      }
      ++_it;
      if (_it==_pageSet->end())
      {
          _it=_pageSet->begin();
      }
      // Get pointer to the page we are sending
      // todo: Find a way to skip carousels without going into an infinite loop
      _page=&*_it;
      // If it is marked for deletion, then remove it and send a filler instead.
      if (_page->GetStatusFlag()==TTXPageStream::MARKED)
      {
        _pageSet->remove(*(_it++));
        _page=NULL;
        return filler;
        // Stays in HEADER mode so that we run this again
      }
      if (_page->IsCarousel() && _page->GetCarouselFlag()) // Don't let registered carousel pages into the main page sequence
      {
        // todo:
        // std::cerr << "Page is a carousel. This can not happen" << std::endl;
        _page=NULL;
        return NULL;
      }

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
        // @todo Implement this
        std::cerr << "@todo This page has no longer a carousel. Remove it from the list" << std::endl;
        //exit(3); //
      }
    }

    // Assemble the header. (we can simplify this code or leave it for the optimiser)
    thisPageNum=_page->GetPageNumber();
    thisPageNum=(thisPageNum/0x100) % 0x100; // Remove this line for Continuous Random Acquisition of Pages.
    if (_page->IsCarousel())
		{
			thisSubcode=_page->GetCarouselPage()->GetSubCode();
		}
		else
		{
			thisSubcode=_page->GetSubCode();
		}

    thisStatus=_page->GetPageStatus();
    // p=new Packet();
    p->Header(_magNumber,thisPageNum,thisSubcode,thisStatus);// loads of stuff to do here!

    p->HeaderText(_configure->GetHeaderTemplate()); // Placeholder 32 characters. This gets replaced later


    //p->Parity(13); // don't apply parity here it will screw up the template. parity for the header is done by tx() later
    assert(p!=NULL);
	
	links=_page->GetLinkSet();
	if ((links[0] & links[1] & links[2] & links[3] & links[4] & links[5]) != 0x8FF){ // only create if links were initialised
		_state=STATE_FASTEXT; // a non zero FL row will override an OL,27 row
	} else { 
		_lastTxt=_page->GetTxRow(27); // Get _lastTxt ready for packet 27 processing
		_state=STATE_PACKET27;
	}
	break;
   
  case STATE_FASTEXT:
		p->SetMRAG(_magNumber,27);
		links=_page->GetLinkSet();
		p->Fastext(links,_magNumber);
		_lastTxt=_page->GetTxRow(28); // Get _lastTxt ready for packet 28 processing
		_state=STATE_PACKET28;
	break;

  case STATE_PACKET27:
		if (_lastTxt)
		{
			//std::cerr << "Packet 27 length=" << _lastTxt->GetLine().length() << std::endl;
			//_lastTxt->Dump();
			p->SetRow(_magNumber, 27, _lastTxt->GetLine(), 0); // TODO coding for navigation packets
			_lastTxt=_lastTxt->GetNextLine();
			break;
		}
		_lastTxt=_page->GetTxRow(28); // Get _lastTxt ready for packet 28 processing
		_state=STATE_PACKET28; // Fall through
  case STATE_PACKET28:
		if (_lastTxt)
		{
			//std::cerr << "Packet 28 length=" << _lastTxt->GetLine().length() << std::endl;
			//_lastTxt->Dump();
			
			p->SetRow(_magNumber, 28, _lastTxt->GetLine(), CODING_13_TRIPLETS);
			_lastTxt=_lastTxt->GetNextLine();
			break;
		}
		else if (_page->GetRegion())
		{
			// create X/28/0 packet for pages which have a region set with RE in file
			// it is important that pages with X/28/0,2,3,4 packets don't set a region otherwise an extra X/28/0 will be generated. TTXPage::SetRow sets the region to 0 for these packets just in case.
			
			// this could almost certainly be done more efficiently but it's quite confusing and this is more readable for when it all goes wrong.
			std::string val = "@@@tGpCuW@twwCpRA`UBWwDsWwuwwwUwWwuWwE@@"; // default X/28/0 packet
			int region = _page->GetRegion();
			int NOS = (_page->GetPageStatus() & 0x380) >> 7;
			int language = NOS | (region << 3);
			int triplet = 0x3C000 | (language << 7); // construct triplet 1
			val.replace(1,1,1,(triplet & 0x3F) | 0x40);
			val.replace(2,1,1,((triplet & 0xFC0) >> 6) | 0x40);
			val.replace(3,1,1,((triplet & 0x3F000) >> 12) | 0x40);
			//std::cerr << "[Mag::GetPacket] region:" << std::hex << region << " nos:" << std::hex << NOS << " triplet:" << std::hex << triplet << std::endl;
			p->SetRow(_magNumber, 28, val, CODING_13_TRIPLETS);
			_lastTxt=_page->GetTxRow(26); // Get _lastTxt ready for packet 26 processing
			_state=STATE_PACKET26;
			break;
		}
		if (_page->GetPageCoding() == CODING_7BIT_TEXT){
			// X/26 packets next in normal pages
			_lastTxt=_page->GetTxRow(26); // Get _lastTxt ready for packet 26 processing
			_state=STATE_PACKET26; // Fall through
		} else {
			// do X/1 to X/25 first and go back to X/26 after
			_state=STATE_TEXTROW;
			break;
		}
  case STATE_PACKET26:
		if (_lastTxt)
		{
			//std::cerr << "Mag=" << _magNumber << " Page=" << std::hex << _page->GetPageNumber() << std::dec << " Packet 26 length=" << _lastTxt->GetLine().length() << std::endl;
			//_lastTxt->Dump();
			// p->SetMRAG(_magNumber,26); // This line *should* be redundant, and it is
			p->SetRow(_magNumber, 26, _lastTxt->GetLine(), CODING_13_TRIPLETS);
			// Do we have another line?
			_lastTxt=_lastTxt->GetNextLine();
			//std::cerr << "*";
			break;
		}
		if (_page->GetPageCoding() == CODING_7BIT_TEXT){
			_state=STATE_TEXTROW; // Fall through to text rows on normal pages
		} else {
			// otherwise we end the page here
			p=NULL;
			_state=STATE_HEADER;
			_thisRow=0;
			break;
		}
  case STATE_TEXTROW:
    // Find the next row that isn't NULL
    for (_thisRow++;_thisRow<26;_thisRow++)
    {
      _lastTxt=_page->GetTxRow(_thisRow);
      if (_lastTxt!=NULL)
                break;
    }
    // Didn't find? End of this page.
    if (_thisRow>25 || _lastTxt==NULL)
    {
      if(_page->GetPageCoding() == CODING_7BIT_TEXT){
        // if this is a normal page we've finished
        p=NULL;
        _state=STATE_HEADER;
        _thisRow=0;
        //_outp("H");
      } else {
        // otherwise go on to X/26
        _lastTxt=_page->GetTxRow(26);
        _state=STATE_PACKET26;
      }
    }
    else
    {
      //_outp("J");
      if (_lastTxt->IsBlank() && _configure->GetRowAdaptive()) // If the row is empty then skip it
      {
        // std::cerr << "[Mag::GetPacket] Empty row" << std::hex << _page->GetPageNumber() << std::dec << std::endl;
        p=NULL;
      }
      else
      {
        // Assemble the packet
        p->SetRow(_magNumber, _thisRow, _lastTxt->GetLine(), _page->GetPageCoding());
        if (_page->GetPageCoding() == CODING_7BIT_TEXT)
            p->Parity(); // only set parity for normal text rows
        assert(p->IsHeader()!=true);
      }
    }

    break;
  } // switch
  return p;
}

void Mag::_outp(std::string s)
{
  return;
  if (_magNumber==5)
    std::cerr << s;
}

