#ifndef PACKETDATACAST_H
#define PACKETDATACAST_H

#include "packetsource.h"
#include "configure.h"

namespace vbit
{
    class PacketDatacast : public PacketSource
    {
        public:
            PacketDatacast(uint8_t datachannel, ttx::Configure* configure);
            virtual ~PacketDatacast();
            
            Packet* GetPacket(Packet* p) override;
            bool IsReady(bool force=false);
            
            int PushRaw(std::vector<uint8_t> *data);
            int PushIDLA(uint8_t flags, uint8_t ial, uint32_t spa, uint8_t ri, uint8_t ci, std::vector<uint8_t> *data);
            
        protected:
        private:
            uint8_t _datachannel;
            
            std::vector<Packet*> _packetBuffer;
            uint8_t _bufferSize;
            uint8_t _head;
            uint8_t _tail;
            
            Packet* GetFreeBuffer();
    };
}

#endif // PACKETDATACAST_H
