/** Implements a packet source for magazines
 */

#include "packetmag.h"

using namespace vbit;

// @todo Initialise the magazine data
PacketMag::PacketMag(uint8_t mag, std::list<TTXPageStream>* pageSet, ttx::Configure *configure, uint8_t priority) :
    _pageSet(pageSet),
    _configure(configure),
    _page(NULL),
    _magNumber(mag),
    _priority(priority),
    _priorityCount(priority),
    _headerFlag(false),
    _state(PACKETSTATE_HEADER),
    _thisRow(0),
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
}

PacketMag::~PacketMag()
{
  //dtor
  delete _carousel;
}

// @todo Invent a packet sequencer similar to mag.cpp which this will replace
Packet* PacketMag::GetPacket(Packet* p)
{
  int thisPageNum;
  unsigned int thisSubcode;
  int thisStatus;
  int* links=NULL;

  static vbit::Packet* filler=new Packet(8,25,"                                        "); // filler

  // We should only call GetPacket if IsReady has returned true
  if (!IsReady())
  {
      std::cerr << "[PacketMag::GetPacket] Packet not ready. This must not happen" << std::endl;
      exit(0);
  }

  // If there is no page, we should send a filler
  if (_pageSet->size()<1)
  {
    return filler;
  }

  switch (_state)
  {
    case PACKETSTATE_HEADER: // Start to send out a new page, which may be a simple page or one of a carousel
      ClearEvent(EVENT_FIELD); // This will suspend all packets until the next field.

      _page=_carousel->nextCarousel(); // The next carousel page

      // But before that, do some housekeeping

      // Is this page deleted?
      if (_page && _page->GetStatusFlag()==TTXPageStream::MARKED)
      {
        _carousel->deletePage(_page);
        _page=NULL;
      }

      if (_page) // Carousel? Step to the next subpage
      {
        //_outp("c");
        _page->StepNextSubpage();
        //std::cerr << "[Mag::GetPacket] Header thisSubcode=" << std::hex << _page->GetCarouselPage()->GetSubCode() << std::endl;

      }
      else  // No carousel? Take the next page in the main sequence
      {
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
          std::cerr << "This can not happen" << std::endl;
          exit(0);
          // Page is a carousel. This can not happen
          //_page=NULL;
          //return NULL;
        }
      }
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
        //std::cerr << "@todo This page has no longer a carousel. Remove it from the list" << std::endl;
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
		_state=PACKETSTATE_FASTEXT; // a non zero FL row will override an OL,27 row
	} else {
		_lastTxt=_page->GetTxRow(27); // Get _lastTxt ready for packet 27 processing
		_state=PACKETSTATE_PACKET27;
	}
      break;
    case PACKETSTATE_TEXTROW:
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
          _state=PACKETSTATE_HEADER;
          _thisRow=0;
          //_outp("H");
        } else {
          // otherwise go on to X/26
          _lastTxt=_page->GetTxRow(26);
          _state=PACKETSTATE_PACKET26;
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
    case PACKETSTATE_FASTEXT:
      p->SetMRAG(_magNumber,27);
      links=_page->GetLinkSet();
      p->Fastext(links,_magNumber);
      _lastTxt=_page->GetTxRow(28); // Get _lastTxt ready for packet 28 processing
      _state=PACKETSTATE_PACKET28;
      break;
  default:
    _state=PACKETSTATE_HEADER;// For now, do the next page
    // Other packets that Alistair will want implemented go here
  }

  return p; //
}

/** Is there a packet ready to go?
 *  If the ready flag is set
 *  and the priority count allows a packet to go out
 */
bool PacketMag::IsReady()
{
  // This happens unless we have just sent out a header
  if (GetEvent(EVENT_FIELD))
  {
    // If we send a header we want to wait for this to get set GetEvent(EVENT_FIELD)
    _priorityCount--;
    if (_priorityCount==0)
    {
      _priorityCount=_priority;
      return true;
    }
  }
  return false;
};
