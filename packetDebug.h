/** Packet source for debugging output
 */
#ifndef _PACKETDEBUG_H_
#define _PACKETDEBUG_H_

#include <list>
#include "packetsource.h"
#include "configure.h"
#include "debug.h"

#define VBIT2_DEBUG_VERSION 0x01   // Debug API version

namespace vbit
{
    class PacketDebug : public PacketSource
    {
        public:
            /** Default constructor */
            PacketDebug(ttx::Configure* configure, Debug* debug);
            /** Default destructor */
            virtual ~PacketDebug();

            // overrides
            Packet* GetPacket(Packet* p) override;
            
            void TimeAndField(MasterClock::timeStruct masterClock, time_t systemClock);
            
            bool IsReady(bool force=false);

        protected:

        private:
            ttx::Configure* _configure;
            Debug* _debug;
            
            uint8_t _datachannel;
            uint32_t _servicePacketAddress;
            uint8_t _debugPacketCI; // continuity indicator for databroadcast stream
            
            struct debugData
            {
                char header[4] = {'V','B','I','T'};
                uint8_t ver = VBIT2_DEBUG_VERSION;
                time_t masterClockSeconds = 0;
                uint8_t masterClockFields = 0;
                time_t systemClock = 0;
            };
            
            struct debugData _debugData;
    };
}

#endif // _PACKETDEBUG_H_
