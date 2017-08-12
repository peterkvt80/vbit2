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
    _priorityCount(priority)
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
Packet* PacketMag::GetPacket()
{
  static vbit::Packet* filler=new Packet(8,25,"                                        "); // filler


  // If there is no page, we should send a filler
  if (_pageSet->size()<1)
  {
    return filler;
  }

  // If we send a header we go into a wait state
  ClearEvent(EVENT_FIELD); // @todo Only when we send the header
  return filler; // Dummy return for now
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
