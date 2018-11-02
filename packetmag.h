#ifndef PACKETMAG_H
#define PACKETMAG_H
#include <list>
#include <packetsource.h>
#include "ttxpagestream.h"
#include "carousel.h"
#include "specialpages.h"
#include "normalpages.h"
#include "configure.h"

#define MAXPACKET29TYPES 3

namespace vbit
{

enum PacketState {PACKETSTATE_HEADER, PACKETSTATE_FASTEXT, PACKETSTATE_PACKET26, PACKETSTATE_PACKET27, PACKETSTATE_PACKET28, PACKETSTATE_TEXTROW};

class PacketMag : public PacketSource
{
  public:
    /** Default constructor */
    PacketMag(uint8_t mag, std::list<TTXPageStream>* pageSet, ttx::Configure *configure, uint8_t priority);
    /** Default destructor */
    virtual ~PacketMag();

    /** Access _pageSet
    * \return The current value of _pageSet
    */
    std::list<TTXPageStream>*  Get_pageSet() { return _pageSet; }
    
    SpecialPages* GetSpecialPages() { return _specialPages; }
    NormalPages* GetNormalPages() { return _normalPages; }
    Carousel* GetCarousel() { return _carousel; }
    
    /** Get the next packet
     *  @return The next packet OR if IsReady() would return false then a filler packet
     */
    Packet* GetPacket(Packet* p) override;
    
    void SetPriority(uint8_t priority) { _priority = priority; }

    bool IsReady(bool force=false);
	
	void SetPacket29(TTXLine *lines[MAXPACKET29TYPES]);

  protected:

  private:
      std::list<TTXPageStream>*  _pageSet; //!< Member variable "_pageSet"
      ttx::Configure* _configure;
      TTXPageStream* _page; //!< The current page being output
      int _magNumber; //!< The number of this magazine. (where 0 is mag 8)
      uint8_t _priority; //!< Priority of transmission where 1 is highest

      std::list<TTXPageStream>::iterator _it;
      Carousel* _carousel;
      SpecialPages* _specialPages;
      NormalPages* _normalPages;
      uint8_t _priorityCount; /// Controls transmission priority
      PacketState _state; /// State machine to sequence packet types
      uint8_t _thisRow; // The current line that we are outputting
      TTXLine* _lastTxt; // The text of the last row that we fetched. Used for enhanced packets

      TTXLine* _packet29[MAXPACKET29TYPES]; // space to store magazine related enhancement packets
      TTXLine* _nextPacket29;
      int _nextPacket29DC;
      int _magRegion;
      int _status;
      int _region;
      bool _hasX28Region;
      bool _specialPagesFlipFlop; // toggle to alternate between special pages and normal pages
      int _waitingForField;
};

}

#endif // PACKETMAG_H
