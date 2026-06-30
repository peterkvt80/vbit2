/* Packet source for datacast channels */

#include "packetDatacast.h"

using namespace vbit;

PacketDatacast::PacketDatacast(uint8_t datachannel, Configure* configure) :
    _datachannel(datachannel),
    _IDLBState(IDLBStateEmpty)
{
    uint16_t datacastLines = configure->GetDatacastLines();
    if (datacastLines == 0 || datacastLines > 4)
        datacastLines = 4; /* cap at 4 lines */
    _bufferSize = datacastLines*4; /* assign space for around 4 fields */
    
    for (int i=0; i<_bufferSize; i++){
        // build packet buffer
        _packetBuffer.push_back(new Packet(8,25));
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
    _mtx.lock(); // lock buffer while modifying
    Packet* p = GetFreeBuffer();
    if (p != nullptr)
    {
        bytes = p->IDLA(_datachannel, flags, ial, spa, ri, ci, *data);
        _head = (_head + 1) % _bufferSize; // advance head on circular buffer
    }
    _mtx.unlock();
    
    return bytes;
}

void PacketDatacast::LoadIDLBRow(uint8_t row, uint8_t *data)
{
    // copy user data into packet data array
    std::copy(data, data+35, _IDLBPacketBlock.begin()+(row*40+3));
    
    uint8_t s0 = 0;
    uint8_t s1 = 0;
    for (int i = 0; i < 35; i++)
    {
        s1 ^= data[i];
        s0 = TimesA[s0]^data[i];
    }
    
    CalculateIDLBSuffixBytes(&s0, &s1);
    
    _IDLBPacketBlock[row*40+38] = s0;
    _IDLBPacketBlock[row*40+39] = s1;
}

void PacketDatacast::CalculateIDLBSuffixBytes(uint8_t *s0, uint8_t *s1)
{
    *s0 = TimesA[*s0]; // This is a bit strange?
    *s0 = TimesA[*s0];
    if (*s1 == *s0) // can't take the log
    {
        *s0 = 0;
        *s1 = *s0;
    }
    else
    {
        int t = BaseALog[*s1^*s0];
        *s0 = AToPower[(255 + t - BaseALog[3]) % 255];
        *s1 = *s0 ^ *s1;
    }
}

void PacketDatacast::CalculateIDLBProtectionBytes()
{
    for (int col=0; col<35; col++)
    {
        uint8_t p = 0;
        uint8_t q = 0;
        uint8_t d;
        for (int i=0; i<14; i++)
        {
            d = _IDLBPacketBlock[40*i+col+3];
            q ^= d;
            p = TimesA[p]^d;
        }
        
        CalculateIDLBSuffixBytes(&p, &q);
        _IDLBPacketBlock[40*14+col+3] = p;
        _IDLBPacketBlock[40*15+col+3] = q;
    }
    
    for (int row = 14; row < 16; row++)
    {
        uint8_t s0 = 0;
        uint8_t s1 = 0;
        for (int i = 0; i < 35; i++)
        {
            uint8_t d = _IDLBPacketBlock[row*40+i+3];
            s1 ^= d;
            s0 = TimesA[s0]^d;
        }
        
        CalculateIDLBSuffixBytes(&s0, &s1);
        
        _IDLBPacketBlock[row*40+38] = s0;
        _IDLBPacketBlock[row*40+39] = s1;
    }
}

int PacketDatacast::PushIDLBHalf(bool halfFlag, uint8_t an, uint8_t ai, std::array<uint8_t, 245> *data)
{
    /* attmempt to push half an IDLB bundle */
    
    if (_IDLBState == IDLBStateEmpty)
    {
        if (!halfFlag) // first half of new bundle
        {
            _an = an;
            _ai = ai;
            for (int i = 0; i < 7; i++)
                LoadIDLBRow(i, data->begin()+(i*35));

            _IDLBState = IDLBStateHalf;
            return 245; // ok, loaded 245 bytes
        }
        else
        {
            _IDLBState=IDLBStateEmpty; // invalidate buffer
            return -1; // error, can't load a second half without the first half
        }
    }
    else if (_IDLBState == IDLBStateHalf)
    {
        if (halfFlag) // second half of new bundle
        {
            if (an == _an && ai == _ai)
            {
                for (int i = 0; i < 7; i++)
                    LoadIDLBRow(i+7, data->begin()+(i*35));
                
                CalculateIDLBProtectionBytes();
                
                for (int i = 0; i < 16; i++)
                {
                    // set correct IDL B header
                    _IDLBPacketBlock[i*40] = Hamming8EncodeTable[(0x1 | ((_an & 3) << 2))]; // format type
                    _IDLBPacketBlock[i*40+1] = Hamming8EncodeTable[_ai & 0xf]; // application identifier
                    _IDLBPacketBlock[i*40+2] = Hamming8EncodeTable[i & 0xf]; // continuity index
                }
                
                _IDLBNextRow = 0;
                _IDLBState = IDLBStateLoaded;
                return 245; // ok, loaded 245 bytes
            }
            else
            {
                _IDLBState=IDLBStateEmpty; // invalidate buffer
                return -1; // error, an and ai must match
            }
        }
        else
        {
            _IDLBState=IDLBStateEmpty; // invalidate buffer
            return -1; // error, can't load new first half yet
        }
    }
    else // IDLBStateLoaded
    {
        return 0; // buffer is full (no bytes loaded)
    }
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
    
    if (_IDLBState == IDLBStateLoaded) // IDL B data is waiting
    {
        if (_mtx.try_lock()) // skip if interface thread has buffer locked
        {
            // push IDL B packets to packet buffer
            // TODO: this will hog buffer - should maybe try to share a bit better
            Packet* p = GetFreeBuffer();
            while (p != nullptr)
            {
                p->SetPacketRaw(std::vector<uint8_t>(_IDLBPacketBlock.begin()+40*_IDLBNextRow, _IDLBPacketBlock.begin()+40*_IDLBNextRow+40));
                _head = (_head + 1) % _bufferSize; // advance head on circular buffer
                
                _IDLBNextRow++;
                if (_IDLBNextRow > 15)
                    _IDLBState = IDLBStateEmpty; // free the IDL B buffer again
                
                p = GetFreeBuffer();
            }
            
            _mtx.unlock();
        }
    }
    
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
