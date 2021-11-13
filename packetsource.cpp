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

void PacketSource::SetEvent(Event event)
{
    // An event is recorded by setting the corresponding flag
    // GetPacket() will clear any event flags when it needs to wait.
    _eventList[event]=true;
}
