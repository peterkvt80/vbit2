#include "packetDebug.h"

using namespace vbit;

PacketDebug::PacketDebug(ttx::Configure* configure, Debug* debug) :
    _configure(configure),
    _debug(debug),
    _debugPacketCI(0), // continuity counter for debug datacast packets
    _startupTime(time(NULL)),
    _masterClockSeconds(0),
    _masterClockFields(0),
    _systemClock(0),
    _debugType(FORMAT1),
    _clockFlags(CFLAGRESYNC),
    _magFlags(0)
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
    /* Generate simple datacast packets for timing measurement and magazine monitoring.
       The user data payload for each format type is padded to a fixed 26 bytes long.
       The first byte of each packet payload indicates the format type.
    */
    
    std::vector<uint8_t> data;
    int mag;
    
    data.push_back(_debugType); // format type in this packet
    
    // all types start with the current master clock
    data.push_back(_masterClockSeconds >> 24);
    data.push_back(_masterClockSeconds >> 16);
    data.push_back(_masterClockSeconds >> 8);
    data.push_back(_masterClockSeconds);
    data.push_back(_masterClockFields);
    
    // 6 of 26 bytes used
    
    switch(_debugType)
    {
        default:
        case NONE: // this should never get sent
        {
            break;
        }
        case FORMAT1: // packet type 1, system time
        {
            _debugType = FORMAT2; // cycle to format 2 on next packet
            
            data.push_back(VBIT2_DEBUG_VERSION); // Debug stream format version
            
            // clock flags
            data.push_back(_clockFlags);
            _clockFlags = 0; // clear flags
            
            // current system clock
            data.push_back(_systemClock >> 24);
            data.push_back(_systemClock >> 16);
            data.push_back(_systemClock >> 8);
            data.push_back(_systemClock);
            
            // vbit2 startup timestamp
            data.push_back(_startupTime >> 24);
            data.push_back(_startupTime >> 16);
            data.push_back(_startupTime >> 8);
            data.push_back(_startupTime);
            
            // 16 of 26 bytes used
            
            break;
        }
        case FORMAT2: // packet type 2, magazine data
        {
            _debugType = FORMAT1; // cycle to format 1 on next packet
            
            // magazine flags
            data.push_back(_magFlags);
            _magFlags = 0; // clear flags
            
            for (mag=0; mag<8; mag++){
                data.push_back(_magDurations[mag]);
            }
            // these 8 bytes may result in a dummy byte!
            
            for (mag=0; mag<8; mag++){
                data.push_back(_magSizes[mag]);
            }
            // these 8 bytes may result in a dummy byte!
            
            // 22 of 26 bytes used
            
            break;
        }
    }
    
    data.insert(data.end(), 26 - data.size(), 0); // pad payload data to 26 bytes long (leave three bytes available in packet for dummies)
    
    // generate format A datacast packet with explicit data length, and implicit continuity indicator
    p->IDLA(_datachannel, Packet::IDLA_DL, 6, _servicePacketAddress, 0, _debugPacketCI++, data); 
    
    return p;
}

bool PacketDebug::IsReady(bool force)
{
    (void)force; // silence error about unused parameter
    bool result=false;
    
    if (GetEvent(EVENT_FIELD))
    {
        ClearEvent(EVENT_FIELD);
        
        if (_debugType > FORMAT1) // we are in the middle of sending debug data already
        {
            result = true;
        }
        else
        {
            // update magazine data
            int mag;
            std::array<int, 8> magDurations = _debug->GetMagCycleDurations(); // get magazine durations in number of fields
            for (mag=0; mag<8; mag++){
                if(magDurations[mag] < 0 || magDurations[mag] / 50 > 255)
                    magDurations[mag] = 255; // clamp to 255 seconds
                else
                    magDurations[mag] = magDurations[mag] / 50; // truncate to whole seconds
                
                if (_magDurations[mag] != magDurations[mag])
                {
                    _magFlags |= MFLAGDURATION; // mag durations have changed!
                    _magDurations[mag] = magDurations[mag]; // update our internal variable
                }
            }
            
            std::array<int, 8> magSizes = _debug->GetMagSizes(); // get number of pages in each magazine
            for (mag=0; mag<8; mag++){
                if (_magSizes[mag] != magSizes[mag])
                {
                    _magFlags |= MFLAGPAGES; // mag sizes have changed!
                    _magSizes[mag] = magSizes[mag]; // update our internal variable
                }
            }
            
            if (_clockFlags || _magFlags || force) // generate packets if any of the content has changed (or we are forcing output to use this as datacast filler)
                result = true;
        }
    }
    
    return result;
}

void PacketDebug::TimeAndField(MasterClock::timeStruct masterClock, time_t systemClock, bool resync)
{
    // update the clocks in _debugData struct - called once per field by Service::_updateEvents()
    _masterClockSeconds = masterClock.seconds;
    _masterClockFields = masterClock.fields;
    _systemClock = systemClock;
    if (resync)
        _clockFlags |= CFLAGRESYNC; // set flag
}
