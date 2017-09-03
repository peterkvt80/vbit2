/** Delivers subtitle packets
 */
#ifndef _PACKETSUBTITLE_H_
#define _PACKETSUBTITLE_H_

#include <thread>
#include <mutex>

#include "packetsource.h"
#include "ttxpage.h"
#include "configure.h"

namespace vbit{

/** Subtitles state machine
 */
enum SubtitleState
{
	SUBTITLE_STATE_IDLE,
  SUBTITLE_STATE_HEADER,
  SUBTITLE_STATE_TEXT_ROW,
  SUBTITLE_STATE_NUMBER_ITEMS
};

class PacketSubtitle : public PacketSource
{
  public:
    /** Default constructor */
    PacketSubtitle(ttx::Configure *configure);
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
		TTXPage _page[2];
		uint8_t _swap;
		SubtitleState _state;	// Subtitle state machine
		uint8_t _rowCount;  // Used to iterate through the rows of the subtitle page
    ttx::Configure* _configure; /// Configuration object
		uint8_t _repeatCount; /// Counts repeat transmissions
		bool _C8Flag;         /// C8 Update flag. Set when new sub comes in, cleared when first header goes out.
};

}

#endif // _PACKETSUBTITLE_H_
