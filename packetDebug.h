/** Packet source for debugging output
 */
#ifndef _PACKETDEBUG_H_
#define _PACKETDEBUG_H_

#include <list>
#include "packetsource.h"
#include "configure.h"

#define VBIT2_DEBUG_VERSION 0x00   // Debug API version

#define MAXMESSAGEBUFFER 30 // maximum number of debug messages to queue before discarding
#define BUFFERRECOVER 20 // size buffer must shrink to before allowing new messages to be queued after an overrun

namespace vbit
{
    class PacketDebug : public PacketSource
    {
        public:
            /** Default constructor */
            PacketDebug(ttx::Configure* configure);
            /** Default destructor */
            virtual ~PacketDebug();

            // overrides
            Packet* GetPacket(Packet* p) override;
            
            void TimeAndField(time_t masterClock, uint8_t fieldCount, time_t systemClock);
            
            bool IsReady(bool force=false);

        protected:

        private:
            ttx::Configure* _configure;
            
            uint8_t _datachannel;
            uint32_t _servicePacketAddress;
            uint8_t _debugPacketCI; // continuity indicator for databroadcast stream
            
            struct debugData
            {
                char header[4] = {'V','B','I','T'};
                uint8_t ver = VBIT2_DEBUG_VERSION;
                time_t masterClock = 0;
                uint8_t fieldCount = 0;
                time_t systemClock = 0;
            };
            
            struct debugData _debugData;
    };
}

#endif // _PACKETDEBUG_H_