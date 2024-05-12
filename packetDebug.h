/** Packet source for debugging output
 */
#ifndef _PACKETDEBUG_H_
#define _PACKETDEBUG_H_

#include <list>
#include "packetsource.h"
#include "configure.h"
#include "debug.h"

#define VBIT2_DEBUG_VERSION 0x01   // Debug packet version

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
            
            void TimeAndField(MasterClock::timeStruct masterClock, time_t systemClock, bool resync);
            
            bool IsReady(bool force=false);

        protected:

        private:
            ttx::Configure* _configure;
            Debug* _debug;
            
            uint8_t _datachannel;
            uint32_t _servicePacketAddress;
            uint8_t _debugPacketCI; // continuity indicator for databroadcast stream
            
            const time_t _startupTime;
            time_t _masterClockSeconds;
            uint8_t _masterClockFields;
            time_t _systemClock;
            
            std::array<uint8_t, 8> _magDurations;
            std::array<uint8_t, 8> _magSizes;
            
            enum DebugTypes {NONE, FORMAT1, FORMAT2};
            DebugTypes _debugType;
            
            enum ClockFlags {CFLAGRESYNC = 1};
            uint8_t _clockFlags;
            
            enum MagFlags {MFLAGDURATION=1, MFLAGPAGES=2};
            uint8_t _magFlags;
    };
}

#endif // _PACKETDEBUG_H_
