/* @todo This is hard wired for 888. Need to honour Newfor mag and page settings */
#include "packetsubtitle.h"

using namespace vbit;

PacketSubtitle::PacketSubtitle(ttx::Configure *configure) :
	_swap(0),
	_state(SUBTITLE_STATE_IDLE),
	_rowCount(0),
  _configure(configure),
	_repeatCount(_configure->GetSubtitleRepeats()),
	_C8Flag(true)
{
  //ctor
  std::cerr << "PacketSubtitle constructed" << std::endl;
}

PacketSubtitle::~PacketSubtitle()
{
  //dtor
}

Packet* PacketSubtitle::GetPacket(Packet* p)
{
  // std::cerr << "[PacketSubtitle::GetPacket] State=" << _state << std::endl;
  // @todo These values should be decoded from the Newfor feed instead of being hard wired
	uint8_t mag=8 & 0x07; // 0 is mag 8!
	uint8_t page=0x88;    // These are in hex
 

	_mtx.lock();						// lock the critical section
	switch (_state)
	{
	case SUBTITLE_STATE_IDLE : // This can not happen. We can't put out a packet if we are in idle.
		std::cerr << "[PacketSubtitle::GetPacket] can not happen" << std::endl;
	  break;
  case SUBTITLE_STATE_HEADER:
		// std::cerr << "[PacketSubtitle::GetPacket] Header. repeat count=" << (int)_repeatCount << std::endl;
		// Construct the header packet and then wait for a field
		{
		  uint8_t status=PAGESTATUS_C4_ERASEPAGE | PAGESTATUS_C6_SUBTITLE; // Erase page + Subtitle // @todo Clear these flags to see a bug. There is a page cleardown bug we could fix
		  // uint8_t status=0; // Erase page + Subtitle // @todo Use this one instead to see a bug
      // The first transmission should have the Update Indicator set. Repeat transmissions do not.
      if (_C8Flag)
      {
        status|=PAGESTATUS_C8_UPDATE;
        _C8Flag=false;
      }
      p->Header(mag, page, 0, status); // Create the header
		}
		p->HeaderText("XENOXXX INDUSTRIES         CLOCK");	// Only Jason will see this if he decodes a tape.
    ClearEvent(EVENT_FIELD);      // Suspend all packets until the next field
		_state=SUBTITLE_STATE_TEXT_ROW;
		_rowCount=1;	// Set up iterator for page rows
	  break;
  case SUBTITLE_STATE_TEXT_ROW:
		// 1) Copy the next non-null row to p
    if (_rowCount<24)
    {
			std::cerr << "[PacketSubtitle::GetPacket] Sending row=" << (int) _rowCount << " string=#" << _page[_swap].GetRow(_rowCount)->GetLine() << "#" << std::endl;
      p->SetRow(mag,_rowCount,_page[_swap].GetRow(_rowCount)->GetLine(),CODING_7BIT_TEXT);
			_rowCount++;
    }
    else // Out of rows? Terminate
    {
      _state=SUBTITLE_STATE_IDLE;
      // @todo. Note that this is the wrong place to terminate as we have no packet to send
			// @todo Check that IsReady prevents this branch from ever being taken
    }
	  break;
  case SUBTITLE_STATE_NUMBER_ITEMS:
		std::cerr << "[PacketSubtitle::IsReady] This is impossible" << std::endl;
	  break;

	} // switch
	_mtx.unlock();					// unlock the critical section
	// p->Dump();
  return p;
}

bool PacketSubtitle::IsReady(bool force)
{
  // This will return FALSE until there is a subtitle row to go out.
  // After the header row goes out it must wait for the next field like any other page
  // What states are there? No subtitles, header sent and waiting.
	// Must call GetPacket if this returns true
  (void)force; // silence error about unused parameter
  bool result=false;
	_mtx.lock();						// lock the critical section
	switch (_state)
	{
	case SUBTITLE_STATE_IDLE : // Process starts with EVENT_SUBTITLE
		if (GetEvent(EVENT_SUBTITLE))
		{
			ClearEvent(EVENT_SUBTITLE);
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
    // Iterate through to the next non blank row.
  	for (;_rowCount<24;_rowCount++)
		{
			if (!_page[_swap].GetRow(_rowCount)->IsBlank())
			{
				break;
			}
		}
		// Return false and reset state machine if there are no more rows
		if (_rowCount<24)
		{
			result=true;
		}
		else
		{
      _state=SUBTITLE_STATE_IDLE; // Subtitle is done so state is idle
		  if (_repeatCount==0)
      {
        result=false; // No more repeats
      }
      else
      {
        _repeatCount--;
        SetEvent(EVENT_SUBTITLE); // Set up to repeat transmission
        result=true;
      }
		}
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
	_mtx.lock();				// lock the critical section
	// std::cerr << "[PacketSubtitle::SendSubtitle] Got page: " << std::endl;

	_swap=(_swap+1)%2;	// swap the double buffering
	_page[_swap].Copy(page); // deep copy page

/*
#ifdef WIN32
	_page[_swap].SavePage("j:\\dev\\vbit2\\subtitleTemp.tti"); // Debug. Send the page representation to a local file
#else
	_page[_swap].SavePage("/dev/stderr"); // Debug. Send the page representation to the error console
	_page[_swap].SavePage("/dev/shm/tempSubtitles.tti"); // Debug. Send the page representation to a file
	_page[_swap].DebugDump();
#endif // WIN32
*/

	// std::cerr << "[PacketSubtitle::SendSubtitle] End of page: " << std::endl;
	SetEvent(EVENT_SUBTITLE);

  _repeatCount=_configure->GetSubtitleRepeats(); // 	// transmission repeat counter

  _C8Flag=true; // New subtitle sets C8 flag

	_mtx.unlock();					// unlock the critical section
}

bool PacketSubtitle::IsIdle()
{
  return _state==SUBTITLE_STATE_IDLE; // Used to block magazine 8 while subtitles are going out
}
