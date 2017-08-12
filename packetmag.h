#ifndef PACKETMAG_H
#define PACKETMAG_H
#include <list>
#include <PacketSource.h>
#include "ttxpagestream.h"
#include "carousel.h"
#include "configure.h"

namespace vbit
{


class PacketMag : public PacketSource
{
  public:
    /** Default constructor */
    PacketMag(uint8_t mag, std::list<TTXPageStream>* pageSet, ttx::Configure *configure, uint8_t priority);
    /** Default destructor */
    virtual ~PacketMag();

    /** Get the next packet
     *  @return The next packet OR if IsReady() would return false then a filler packet
     */
    Packet* GetPacket() override;

    bool IsReady();


  protected:

  private:
        std::list<TTXPageStream>*  _pageSet; //!< Member variable "_pageSet"
        ttx::Configure* _configure;
        TTXPageStream* _page; //!< The current page being output
        int _magNumber; //!< The number of this magazine. (where 0 is mag 8)
        uint8_t _priority; //!< Priority of transmission where 1 is highest

        std::list<TTXPageStream>::iterator _it;
        Carousel* _carousel;
        uint8_t _priorityCount; // Controls transmission priority
};

}

#endif // PACKETMAG_H
