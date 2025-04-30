#ifndef PACKETMAG_H
#define PACKETMAG_H
#include <list>
#include <mutex>
#include <packetsource.h>
#include "ttxpagestream.h"
#include "carousel.h"
#include "specialpages.h"
#include "normalpages.h"
#include "updatedpages.h"
#include "configure.h"
#include "pagelist.h"
#include "debug.h"
#include "masterClock.h"

#define MAXPACKET29TYPES 3

namespace vbit
{
    class PacketMag : public PacketSource
    {
        public:
            /** Default constructor */
            PacketMag(uint8_t mag, PageList *pageList, Configure *configure, Debug *debug, uint8_t priority);
            /** Default destructor */
            virtual ~PacketMag();

            Carousel* GetCarousel() { return _carousel; }
            SpecialPages* GetSpecialPages() { return _specialPages; }
            NormalPages* GetNormalPages() { return _normalPages; }
            UpdatedPages* GetUpdatedPages() { return _updatedPages; }

            /** Get the next packet
             *  @return The next packet OR if IsReady() would return false then a filler packet
             */
            Packet* GetPacket(Packet* p) override;

            void SetPriority(uint8_t priority) { _priority = priority; }

            bool IsReady(bool force=false);

            void SetPacket29(int i, TTXLine *line);
            bool GetPacket29Flag() { return _hasPacket29; };
            void DeletePacket29();
            
            void SetCustomHeader(std::string row) {_customHeaderTemplate = row; _hasCustomHeader = true;}
            bool GetCustomHeaderFlag() { return _hasCustomHeader; };
            void DeleteCustomHeader();
            
            void InvalidateCycleTimestamp() { _lastCycleTimestamp = {0,0}; }; // reset cycle duration calculation
            int GetCycleDuration() { return _cycleDuration; };

        protected:

        private:
            enum PacketState {PACKETSTATE_HEADER, PACKETSTATE_FASTEXT, PACKETSTATE_PACKET26, PACKETSTATE_PACKET27, PACKETSTATE_PACKET28, PACKETSTATE_TEXTROW};
            
            PageList* _pageList;
            Configure* _configure;
            Debug* _debug;
            TTXPageStream* _page; //!< The current page being output
            TTXPage* _subpage; // pointer to the actual subpage
            int _magNumber; //!< The number of this magazine. (where 0 is mag 8)
            uint8_t _priority; //!< Priority of transmission where 1 is highest

            std::list<TTXPageStream>::iterator _it;
            Carousel* _carousel;
            SpecialPages* _specialPages;
            NormalPages* _normalPages;
            UpdatedPages* _updatedPages;
            uint8_t _priorityCount; /// Controls transmission priority
            PacketState _state; /// State machine to sequence packet types
            uint8_t _thisRow; // The current line that we are outputting
            TTXLine* _lastTxt; // The text of the last row that we fetched. Used for enhanced packets

            int _nextPacket29DC;
            TTXLine* _packet29[MAXPACKET29TYPES]; // space to store magazine related enhancement packets
            TTXLine* _nextPacket29;
            bool _hasPacket29;
            std::mutex _mtx; // Mutex to interlock packet 29 from filemonitor
            
            std::string _customHeaderTemplate;
            bool _hasCustomHeader;

            int _magRegion;
            int _status;
            int _region;
            bool _hasX28Region;
            bool _specialPagesFlipFlop; // toggle to alternate between special pages and normal pages
            bool _waitingForField;
            
            MasterClock::timeStruct _lastCycleTimestamp;
            int _cycleDuration; // magazine cycle time in fields
    };
}

#endif // PACKETMAG_H
