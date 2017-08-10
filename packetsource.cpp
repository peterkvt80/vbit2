#include "packetsource.h"

using namespace vbit;

PacketSource::PacketSource()
{
  //ctor
}

PacketSource::~PacketSource()
{
  //dtor
}


Packet* PacketSource::GetPacket()
{
  Packet* pkt=new Packet(8,25,"                                        ");
  // Do your stuff
  return pkt;
}

bool PacketSource::IsReady()
{
  return false; // @todo This will probably just return a member bool
}

void PacketSource::SetEvent(Event event)
{
  //@todo Use event to clear the event flags which may have been set by the packet source being in wait.
}
