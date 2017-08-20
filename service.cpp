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
	_fieldCounter(0)
{
  // @todo Put priority into config and add commands to allow updates.
  uint8_t priority[8]={5,3,3,6,3,3,5,9};	// 1=High priority,9=low. Note: priority[0] is mag 8

  vbit::Mag **magList=_pageList->GetMagazines();
  // Register all the packet sources
  for (uint8_t mag=0;mag<8;mag++)
  {
    vbit::Mag* m=magList[mag];
    std::list<TTXPageStream>* p=m->Get_pageSet();
    _register(new PacketMag(mag,p,_configure,priority[mag]));
  }
  // @todo Add packet sources for subtitles, databroadcast and packet 830
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
  std::cerr << "[Service::worker]This is the worker process" << std::endl;
  std::list<vbit::PacketSource*>::const_iterator iterator=_Sources.begin(); // Iterator for packet sources

  vbit::Packet* pkt=new vbit::Packet(8,25,"                                        ");  // This just allocates storage.

  static vbit::Packet* filler=new vbit::Packet(8,25,"                                        ");  // @todo Again, we should have a pre-prepared quiet packet to avoid eating the heap

  std::cerr << "[Service::worker]Loop starts" << std::endl;
	while(1) // normal
	{
	  // If counters (or other trigger) causes an event then send the events
    _updateEvents(); // Must only call this ONCE per transmitted row

	  // Iterate through the packet sources until we get a packet to transmit

    vbit::PacketSource* p;
	  do
    {
      if (iterator==_Sources.end())
      {
        iterator=_Sources.begin();
        // @todo Count iterations here and break out with a filler to prevent a deadlock
      }
      p=(*iterator);
      ++iterator;
    }
    while (!p->IsReady());

    p->GetPacket(pkt); // @todo Don't annoy Alistair by putting this bug back in


    // Transmit the packet
    // @todo Does this code make sense?
    if (pkt!=NULL) // How could this be NULL? After the last packet of a page has been sent.
    {
      std::cout.write(pkt->tx(), 42); // Transmit the packet - using cout.write to ensure writing 42 bytes even if it contains a null.
    }
    else
    {
      std::cout << filler->tx();
    } // blocked

	} // while
	return 99; // don't really want to return anything
} // worker

void Service::_updateEvents()
{
  // Step the counters
  _lineCounter++;
  if (_lineCounter%LINESPERFIELD==0)
  {
    _lineCounter=0;
    _fieldCounter++;
    if (_fieldCounter>=50)
    {
      _fieldCounter=0;
      // Could implement a seconds counter here if we needed it
    }
    // New field, so set the FIELD event in all the sources.
    for (std::list<vbit::PacketSource*>::const_iterator iterator = _Sources.begin(), end = _Sources.end(); iterator != end; ++iterator)
    {
      (*iterator)->SetEvent(EVENT_FIELD);
    }
    // Packet 830?
    if (_fieldCounter%10==0)
    {
      Event ev;
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
  }
  // @todo Subtitle events. Flag when a subtitle is ready to go up
  // @todo Databroadcast events. Flag when there is data in the buffer.
}
