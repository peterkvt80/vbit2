/* Packet source for datacast channels */

#include "packetDatacast.h"

using namespace vbit;

PacketDatacast::PacketDatacast(uint8_t datachannel, ttx::Configure* configure) :
    _datachannel(datachannel)
{
    uint16_t datacastLines = configure->GetDatacastLines();
    if (datacastLines == 0 || datacastLines > 4)
        datacastLines = 4; /* cap at 4 lines */
    _bufferSize = datacastLines*4; /* assign space for around 4 fields */
    
    for (int i=0; i<_bufferSize; i++){
        // build packet buffer
        _packetBuffer.push_back(new vbit::Packet(8,25,"                                        "));
    }
    // set head and tail indices
    _head = 0;
    _tail = 0;
}

PacketDatacast::~PacketDatacast()
{
    
}

int PacketDatacast::PushRaw(std::vector<uint8_t> *data)
{
    /* push 40 bytes of raw packet data into the buffer */
    Packet* p = GetFreeBuffer();
    if (p == nullptr)
        return -1;
    
    p->SetPacketRaw(*data); // copy data into buffer packet
    _head = (_head + 1) % _bufferSize; // advance head on circular buffer
    
    return 0;
}

int PacketDatacast::PushIDLA(uint8_t flags, uint8_t ial, uint32_t spa, uint8_t ri, uint8_t ci, std::vector<uint8_t> *data)
{
    /* push a format A datacast packet into the buffer */
    int bytes = 0;
    Packet* p = GetFreeBuffer();
    if (p != nullptr)
    {
        bytes = p->IDLA(_datachannel, flags, ial, spa, ri, ci, *data);
        _head = (_head + 1) % _bufferSize; // advance head on circular buffer
    }
    
    return bytes;
}

Packet* PacketDatacast::GetFreeBuffer()
{
    /* gets a pointer to the next free buffer packet or a null pointer if buffer is full
       Does NOT actually advance the head to avoid a race condition */
    
    if ((_head + 1) % _bufferSize == _tail)
        return nullptr; // buffer full
    else
        return _packetBuffer[_head];
}

Packet* PacketDatacast::GetPacket(Packet* p)
{
    if (_tail == _head)
    {
        // generate some hardcoded datacast filler
        std::string str = "VBIT2 Datacast Service       ";
        std::vector<uint8_t> data(str.begin(), str.end());
        p->IDLA(8, Packet::IDLA_DL, 6, 0xfffffe, 0, 0, data);
        // TODO: make channel, address, and content of filler configurable
    }
    else
    {
        // copy data from buffer to packet
        p->SetMRAG(_datachannel & 0x7,((_datachannel & 8) >> 3) + 30);
        p->SetPacketRaw(std::vector<uint8_t>(_packetBuffer[_tail]->Get_packet().begin()+5, _packetBuffer[_tail]->Get_packet().end()));
        
        _tail = (_tail + 1) % _bufferSize; // advance tail on circular buffer
    }
    
    return p;
}

bool PacketDatacast::IsReady(bool force)
{
    bool result=false;
    
    if (GetEvent(EVENT_DATABROADCAST))
    {
        // Don't clear event, Service::_updateEvents explicitly turns it off for non datacast lines
        
        if (_tail != _head)
            result = true;
        else
            result = force;
    }
    return result;
}
