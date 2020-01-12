/** Service
 */
#include "service.h"

using namespace ttx;
using namespace vbit;

Service::Service()
{
}

Service::Service(Configure *configure, PageList *pageList) :
	_configure(configure),
	_pageList(pageList),
	_lineCounter(0),
	_fieldCounter(50) // roll over immediately
{
  vbit::PacketMag **magList=_pageList->GetMagazines();
  // Register all the packet sources
  for (uint8_t mag=0;mag<8;mag++)
  {
    vbit::PacketMag* m=magList[mag];
    m->SetPriority(_configure->GetMagazinePriority(mag)); // set the mags to the desired priorities
    _register(m); // use the PacketMags created in pageList rather than duplicating them
  }
  // Add packet sources for subtitles, databroadcast and packet 830
  _register(_subtitle=new PacketSubtitle(_configure));
  _register(new Packet830(_configure));
  
  _linesPerField = _configure->GetLinesPerField();
}

Service::~Service()
{
}

void Service::_register(PacketSource *src)
{
  _Sources.push_front(src);
}

int Service::run()
{
  //std::cerr << "[Service::worker] This is the worker process" << std::endl;
  std::list<vbit::PacketSource*>::const_iterator iterator=_Sources.begin(); // Iterator for packet sources

  std::list<vbit::PacketSource*>::const_iterator first; // Save the first so we know if we have looped.

  vbit::Packet* pkt=new vbit::Packet(8,25,"                                        ");  // This just allocates storage.

  static vbit::Packet* filler=new vbit::Packet(8,25,"                                        ");  // A pre-prepared quiet packet to avoid eating the heap
  
    bool reverse = _configure->GetReverseFlag();
    if (reverse)
    {
        std::cerr << "[Service::run] output reversed bytes" << std::endl;
        filler->tx(true); // reverse bytes in filler packet, this modifies the packet so don't reverse it when used below
    }
  
    time(&_then); // prepare timer

  std::cerr << "[Service::run] Loop starts" << std::endl;
  std::cerr << "[Service::run] Lines per field: " << (int)_linesPerField << std::endl;
	while(1)
	{
    //std::cerr << "[Service::run]iterates. VBI line=" << (int) _lineCounter << " (int) field=" << (int) _fieldCounter << std::endl;
	  // If counters (or other trigger) causes an event then send the events

	  // Iterate through the packet sources until we get a packet to transmit

    vbit::PacketSource* p;
    first=iterator;
    bool force=false;
    uint8_t sourceCount=0;
    uint8_t listSize=_Sources.size();

	// Send ONLY one packet per loop
	_updateEvents();

		// Special case for subtitles. Subtitles always go if there is one waiting
		if (_subtitle->IsReady())
		{
			if (_subtitle->GetPacket(pkt) != nullptr){
				std::cout.write(pkt->tx(reverse), 42); // Transmit the packet - using cout.write to ensure writing 42 bytes even if it contains a null.
			} else {
				std::cout.write(filler->tx(), 42);
			}
		}
	  else
		{
			// scan the rest of the available sources
			do
			{
				// Loop back to the first source
				if (iterator==_Sources.end())
				{
					iterator=_Sources.begin();
				}

				// If we have tried all sources with and without force, then break out with a filler to prevent a deadlock
				if (sourceCount>listSize*2)
				{
					p=nullptr;
					// If we get a lot of this maybe there is a problem?
					// std::cerr << "[Service::run] No packet available for this line" << std::endl;
					break;
				}

				// If we have gone around once and got nothing, then force sources to go if possible.
				if (sourceCount>listSize)
				{
					force=true;
				}

				// Get the packet source
				p=(*iterator);
				++iterator;

				sourceCount++; // Count how many sources we tried.
			}
			while (!p->IsReady(force));

			// Did we find a packet? Then send it otherwise put out a filler
			if (p)
			{
				// GetPacket returns nullptr if the pkt isn't valid - if it's null go round again.
				if (p->GetPacket(pkt) != nullptr){
					std::cout.write(pkt->tx(reverse), 42); // Transmit the packet - using cout.write to ensure writing 42 bytes even if it contains a null.
				} else {
					std::cout.write(filler->tx(), 42);
				}
			}
			else
			{
				std::cout.write(filler->tx(), 42);
			}
		}

	} // while forever
	return 99; // can't return but this keeps the compiler happy
} // worker

void Service::_updateEvents()
{
  static int seconds=0;
  // Step the counters
  _lineCounter++;
  time_t now;
  time(&now);
  if (difftime(now,_then) > 0) // system clock second has ticked
  {
      _then = now;
      seconds++;
      _fieldCounter=0; // reset field counter
  }
  if (_lineCounter%_linesPerField==0) // new field
  {
    _lineCounter=0;
    
    if (_fieldCounter==0)
    {
      if (seconds%15==0){ // how often do we want to trigger sending special packets?
        for (std::list<vbit::PacketSource*>::const_iterator iterator = _Sources.begin(), end = _Sources.end(); iterator != end; ++iterator)
        {
          (*iterator)->SetEvent(EVENT_SPECIAL_PAGES);
          (*iterator)->SetEvent(EVENT_PACKET_29);
        }
      }
    }
    // New field, so set the FIELD event in all the sources.
    for (std::list<vbit::PacketSource*>::const_iterator iterator = _Sources.begin(), end = _Sources.end(); iterator != end; ++iterator)
    {
      (*iterator)->SetEvent(EVENT_FIELD);
    }
    
    if (_fieldCounter%10==0 && _fieldCounter<50) // Packet 830 happens every 200ms.
    {
      Event ev=EVENT_P830_FORMAT_1;
      switch (_fieldCounter/10)
      {
      case 0:
        ev=EVENT_P830_FORMAT_1;
        break;
      case 1:
        ev=EVENT_P830_FORMAT_2_LABEL_0;
        break;
      case 2:
        ev=EVENT_P830_FORMAT_2_LABEL_1;
        break;
      case 3:
        ev=EVENT_P830_FORMAT_2_LABEL_2;
        break;
      case 4:
        ev=EVENT_P830_FORMAT_2_LABEL_3;
        break;
      }
      for (std::list<vbit::PacketSource*>::const_iterator iterator = _Sources.begin(), end = _Sources.end(); iterator != end; ++iterator)
      {
        (*iterator)->SetEvent(ev);
      }
    }
    
    _fieldCounter++; // next field
  }
  // @todo Databroadcast events. Flag when there is data in the buffer.
}
