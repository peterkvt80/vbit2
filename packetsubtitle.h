/** Delivers subtitle packets
 */
#ifndef _PACKETSUBTITLE_H_
#define _PACKETSUBTITLE_H_

#include <thread>
#include <mutex>

#include "packetsource.h"
#include "ttxpage.h"

namespace vbit{

/** Subtitles state machine
 */
enum SubtitleEvent
{
	SUBTITLE_EVENT_IDLE,
  SUBTITLE_EVENT_HEADER,
  SUBTITLE_EVENT_TEXT_ROW,
  SUBTITLE_EVENT_NUMBER_ITEMS
};

class PacketSubtitle : public PacketSource
{
  public:
    /** Default constructor */
    PacketSubtitle();
    /** Default destructor */
    virtual ~PacketSubtitle();

    /** @todo Routines to link Newfor subtitles to here
     */

    Packet* GetPacket(Packet* p) override;

    /**
     * Packet 830 must always wait for the correct field.
     * @param force is ignored
     */
    bool IsReady(bool force=false);

    /**
     * @brief Accept a page from another thread
     * @param page - Pointer to another page object
     */
    void SendSubtitle(TTXPage* page);

  protected:

  private:
		std::mutex _mtx;		// Mutex to interlock Mewfor thread
		TTXPage* _page[2];
		uint8_t _swap;
		SubtitleEvent _event;	// Subtitle state machine
};

}

#endif // _PACKETSUBTITLE_H_
