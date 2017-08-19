#include "packetsource.h"

using namespace vbit;

PacketSource::PacketSource() :
  _readyFlag(false)
{
  //ctor
  // This could be in the initializer list BUT does not work in Visual C++
  for (int i=0;i<EVENT_NUMBER_ITEMS;i++)
  {
    _eventList[i]=false;
  }

}

PacketSource::~PacketSource()
{
  //dtor. Probably nothing to do here
}

/** Need to override this function in a child class
 *  The implementation here is useless so don't call it. In fact I should remove it to avoid confusion.
 */
Packet* PacketSource::GetPacket(Packet* p)
{
  p=new Packet(8,25,"                                        ");
  return p;
}

void PacketSource::SetEvent(Event event)
{
  // An event is recorded by setting the corresponding flag
  // GetPacket() will clear any event flags when it needs to wait.
  _eventList[event]=true;
}
