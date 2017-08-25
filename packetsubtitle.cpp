#include "packetsubtitle.h"

using namespace vbit;

PacketSubtitle::PacketSubtitle() :
	_swap(0),
	_state(SUBTITLE_STATE_IDLE),
	_rowCount(0)
{
  //ctor
}

PacketSubtitle::~PacketSubtitle()
{
  //dtor
}

Packet* PacketSubtitle::GetPacket(Packet* p)
{
  // std::cerr << "[PacketSubtitle::GetPacket] GetPacket" << std::endl;
  // @todo Put these values into the configuration instead of being hard wired
	uint8_t mag=8;
	uint8_t page=88;

  p=new Packet(8,25,"                                        ");
	_mtx.lock();						// lock the critical section
	switch (_state)
	{
	case SUBTITLE_STATE_IDLE : // This can not happen. We can't put out a packet if we are in idle.
		std::cerr << "[PacketSubtitle::GetPacket] can not happen" << std::endl;
	  break;
  case SUBTITLE_STATE_HEADER:
		// @todo Construct the header packet and then wait for a field
		// Copy the header to p
		p->Header(mag, page, 0, 0x4002); // Subtitle C6
		ClearEvent(EVENT_FIELD);
		_state=SUBTITLE_STATE_TEXT_ROW;
		_rowCount=1;	// Set up iterator for page rows
	  break;
  case SUBTITLE_STATE_TEXT_ROW:
		// 1) Copy the next non-null row to p
		for (;_rowCount<24 && _page[_swap]->GetRow(_rowCount)->IsBlank();_rowCount++);
    if (_rowCount<24)
    {
//      TTXPage* pg=_page[_swap];
      //TTXLine* ln=pg->GetRow(_rowCount);
      //str:string st=ln->GetLine();
      p->SetRow(mag,_rowCount,_page[_swap]->GetRow(_rowCount)->GetLine(),CODING_7BIT_TEXT);
        // *p=_page[_swap]->GetRow(_rowCount);
    }
    else // Out of rows? Terminate
    {
      _state=SUBTITLE_STATE_IDLE;
      // @todo. Note that this is the wrong place to terminate as we have no packet to send
    }
	  break;
  case SUBTITLE_STATE_NUMBER_ITEMS:
		std::cerr << "[PacketSubtitle::IsReady] This is impossible" << std::endl;
	  break;

	} // switch
	_mtx.unlock();					// unlock the critical section
  return p;
}

bool PacketSubtitle::IsReady(bool force)
{
  // This will return FALSE until there is a subtitle row to go out.
  // After the header row goes out it must wait for the next field like any other page
  // What states are there? No subtitles, header sent and waiting.
	// Must call GetPacket if this returns true
  bool result=false;
	_mtx.lock();						// lock the critical section
	switch (_state)
	{
	case SUBTITLE_STATE_IDLE : // Process starts with EVENT_SUBTITLE
		if (GetEvent(EVENT_SUBTITLE))
		{
			_state=SUBTITLE_STATE_HEADER;
			result=true;
		}
	  break;
  case SUBTITLE_STATE_HEADER:
		if (GetEvent(EVENT_FIELD))
		{
			result=true;
		}
	  break;
  case SUBTITLE_STATE_TEXT_ROW:
    // @todo Iterate through to the next non blank row. Return false and reset state machine if so
		result=true;
	  break;
  case SUBTITLE_STATE_NUMBER_ITEMS:
		std::cerr << "[PacketSubtitle::IsReady] This is impossible" << std::endl;
	  break;
	}
	_mtx.unlock();					// unlock the critical section
  return result;
}

void PacketSubtitle::SendSubtitle(TTXPage* page)
{
  std::cerr << "[] Now do something interesting with the page" << std::endl;
  // @todo buffer the page because Newfor is liable to overwrite it
  // @todo double buffer the pages
  // @todo toggle the buffer
	_mtx.lock();						// lock the critical section
	*(_page[_swap])=*page;	// shallow copy the page
	_swap=(_swap+1)%2;			// swap the double buffering

	page->SavePage("/dev/stderr"); // Debug. Send the page representation to the error console
	SetEvent(EVENT_SUBTITLE);
	_mtx.unlock();					// unlock the critical section
}
