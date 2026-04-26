#ifndef PACKETDATACAST_H
#define PACKETDATACAST_H

#include <mutex>
#include "packetsource.h"
#include "configure.h"
#include "tables.h"

namespace vbit
{
    class PacketDatacast : public PacketSource
    {
        public:
            PacketDatacast(uint8_t datachannel, Configure* configure);
            virtual ~PacketDatacast();
            
            Packet* GetPacket(Packet* p) override;
            bool IsReady(bool force=false);
            
            int PushRaw(std::vector<uint8_t> *data);
            int PushIDLA(uint8_t flags, uint8_t ial, uint32_t spa, uint8_t ri, uint8_t ci, std::vector<uint8_t> *data);
            int PushIDLBHalf(bool halfFlag, uint8_t an, uint8_t ai, std::array<uint8_t, 245> *data);
            
        protected:
        private:
            uint8_t _datachannel;
            
            std::vector<Packet*> _packetBuffer;
            uint8_t _bufferSize;
            uint8_t _head;
            uint8_t _tail;
            
            Packet* GetFreeBuffer();
            
            std::mutex _mtx;
            
            enum IDLBState {IDLBStateEmpty, IDLBStateHalf, IDLBStateLoaded};
            IDLBState _IDLBState;
            uint8_t _IDLBNextRow;
            uint8_t _ai;
            uint8_t _an;
            std::array<uint8_t, 16*40> _IDLBPacketBlock; // fixed buffer for constructing IDL B packet data
            
            void LoadIDLBRow(uint8_t row, uint8_t *data);
            void CalculateIDLBProtectionBytes();
    };
}

#endif // PACKETDATACAST_H
