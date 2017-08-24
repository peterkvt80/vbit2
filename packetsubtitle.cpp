#include "packetsubtitle.h"

using namespace vbit;

PacketSubtitle::PacketSubtitle() :
	_swap(0),
	_event(SUBTITLE_EVENT_IDLE)
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
  // @todo link this to Newfor
  p=new Packet(8,25,"                                        ");
	_mtx.lock();						// lock the critical section
	switch (_event)
	{
	case SUBTITLE_EVENT_IDLE : // This can not happen. We can't put out a packet if we are in idle.
		std::cerr << "[PacketSubtitle::GetPacket] can not happen" << std::endl;
	  break;
  case SUBTITLE_EVENT_HEADER:
		// @todo Construct the header packet and then wait for a field
		// Copy the header to p
		ClearEvent(EVENT_FIELD);
		_event=SUBTITLE_EVENT_TEXT_ROW;
		// @todo Set up iterator for page rows
	  break;
  case SUBTITLE_EVENT_TEXT_ROW:
		// @todo:
		// 1) Copy the next non-null row to p
		// 2) Iterate the row pointer.
		// 3) If no more rows then stop
		_event=SUBTITLE_EVENT_IDLE;
	  break;
	default:
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
	switch (_event)
	{
	case SUBTITLE_EVENT_IDLE : // Process starts with EVENT_SUBTITLE
		if (GetEvent(EVENT_SUBTITLE))
		{
			_event=SUBTITLE_STATE_HEADER;
			result=true;
		}
	  break;
  case SUBTITLE_EVENT_HEADER:
		if (GetEvent(EVENT_FIELD))
		{
			result=true;
		}
	  break;
  case SUBTITLE_EVENT_TEXT_ROW:
		result=true;
	  break;
  case SUBTITLE_EVENT_NUMBER_ITEMS:
		std::cerr << "[PacketSubtitle::IsReady] This is impossible" << std::endl;
	  break;
	default:
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
