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
  // Add packet sources for subtitles, databroadcast and packet 830
  _register(new Packet830());
  _register(_subtitle=new PacketSubtitle());
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

  std::list<vbit::PacketSource*>::const_iterator first; // Save the first so we know if we have looped.

  vbit::Packet* pkt=new vbit::Packet(8,25,"                                        ");  // This just allocates storage.

  static vbit::Packet* filler=new vbit::Packet(8,25,"                                        ");  // @todo Again, we should have a pre-prepared quiet packet to avoid eating the heap

  std::cerr << "[Service::run]Loop starts" << std::endl;
	while(1) // normal
	{
    //std::cerr << "[Service::run]iterates. VBI line=" << (int) _lineCounter << " (int) field=" << (int) _fieldCounter << std::endl;
	  // If counters (or other trigger) causes an event then send the events
    _updateEvents(); // Must only call this ONCE per transmitted row

	  // Iterate through the packet sources until we get a packet to transmit

    vbit::PacketSource* p;
    first=iterator;
    bool force=false;
    uint8_t sourceCount=0;
    uint8_t listSize=_Sources.size();
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
        std::cerr << "[Service::run] No packet available for this line" << std::endl;
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

    if (p)
    {
      // Big assumption here. pkt must always return a valid packet
      // Which means that GetPacket is not allowed to fail. Is this correct?
      p->GetPacket(pkt); // I know this was the original bug, but really don't want to create it dynamically
      std::cout.write(pkt->tx(), 42); // Transmit the packet - using cout.write to ensure writing 42 bytes even if it contains a null.
    }
    else
    {
      std::cout << filler->tx();
    }

	} // while
	return 99; // don't really want to return anything
} // worker

void Service::_updateEvents()
{
  static int seconds=0;
  // Step the counters
  _lineCounter++;
  if (_lineCounter%LINESPERFIELD==0)
  {
    _lineCounter=0;
    _fieldCounter++;
    if (_fieldCounter>=50)
    {
      _fieldCounter=0;
      seconds++;
      // std::cerr << "Seconds=" << seconds << std::endl;
      // Could implement a seconds counter here if we needed it


      // if (seconds>30) exit(0); // JUST FOR DEBUGGING!!!!  Must remove
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
