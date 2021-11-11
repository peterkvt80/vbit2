#include <packetDebug.h>

using namespace vbit;

PacketDebug::PacketDebug(ttx::Configure* configure) :
    _configure(configure),
    _debugPacketCI(0) // continuity counter for debug datacast packets
{
    //ctor
    
    // TODO: make channel and address configurable
    _datachannel = 8; 
    _servicePacketAddress = 0xffffff;
}

PacketDebug::~PacketDebug()
{
    //dtor
}

Packet* PacketDebug::GetPacket(Packet* p)
{
    // crude packets for timing measurement and monitoring
    std::vector<uint8_t> data;
    
    // Debug packet header
    data.assign(_debugData.header, _debugData.header + sizeof(_debugData.header)); // "VBIT"
    data.push_back(_debugData.ver); // debug data version number
    
    // current internal master clock
    data.push_back(_debugData.masterClock >> 24);
    data.push_back(_debugData.masterClock >> 16);
    data.push_back(_debugData.masterClock >> 8);
    data.push_back(_debugData.masterClock);
    
    // current field count
    data.push_back(_debugData.fieldCount);
    
    // current system clock
    data.push_back(_debugData.systemClock >> 24);
    data.push_back(_debugData.systemClock >> 16);
    data.push_back(_debugData.systemClock >> 8);
    data.push_back(_debugData.systemClock);
    
    p->IDLA(_datachannel, 6, _servicePacketAddress, _debugPacketCI++, data);
    
    return p;
}

bool PacketDebug::IsReady(bool force)
{
    (void)force; // silence error about unused parameter
    bool result=false;
    
    if (_configure->GetDebugFlag())
    {
        if (GetEvent(EVENT_FIELD))
        {
            ClearEvent(EVENT_FIELD);
            
            result = true;
        }
    }
    
    return result;
}

void PacketDebug::TimeAndField(time_t masterClock, uint8_t fieldCount, time_t systemClock)
{
    // update the clocks in _debugData struct - called once per field by Service::_updateEvents()
    _debugData.masterClock = masterClock;
    _debugData.fieldCount = fieldCount;
    _debugData.systemClock = systemClock;
}
