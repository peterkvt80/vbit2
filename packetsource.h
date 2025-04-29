/**
 * Anything that generates teletext packets is derived from this interface.
 * Functions defined by this interface are
 * Constructor - sets up the data required by a particular packet source.
 * GetPacket - Gets the next packet in the stream.
 * Waiting - The stream is waiting for an event to restart it
 * SetEvent - Registers an even with the packet source
 *
 * Teletext packets come in four broad categories
 * 1) Magazines - Rows consisting of headers and body rows.
 * 2) Subtitles - Same as magazines but sent at highest priority.
 * 3) Packets on magazine 8, row 30 - Timing and control that happen on specific fields.
 * 4) Databroadcast - Packets that get sent at low priority and when possible.
 *
 * Events
 * 1) Field
 * 2) 10 fields
 * 3) Subtitle data in buffer
 * 4) All other streams are waiting (so we can now send databroadcast)
**/
#ifndef _PACKETSOURCE_H_
#define _PACKETSOURCE_H_

#include <packet.h>

namespace vbit
{

/** @brief Events are used to trigger packet sources so that they may proceed
 *  @description Different packet sources use different timing schemes.
 *  Packet sources may put themselves into a waiting state.
 *  Events are used to start them up again.
 *  Magazines stop after a header and wait for the next field.
 *  Subtitles wait until there is data in the subtitle buffer.
 *  Packet 830 waits for a multiple of 10 fields.
 *  Databroadcast varies according to the importance of the signal.
 *  Lowest priority may only jump in only when a filler packet would be sent.
 *  Highest priority might steal all the packets except for 830.
 */
enum Event
{
  EVENT_FIELD,
  EVENT_P830_FORMAT_1,
  EVENT_P830_FORMAT_2_LABEL_0,
  EVENT_P830_FORMAT_2_LABEL_1,
  EVENT_P830_FORMAT_2_LABEL_2,
  EVENT_P830_FORMAT_2_LABEL_3,
  EVENT_SUBTITLE,
  EVENT_DATABROADCAST,
  EVENT_SPECIAL_PAGES,
  EVENT_PACKET_29,
  EVENT_NUMBER_ITEMS
}  ;


class PacketSource
{
  public:
    /** Default constructor */
    PacketSource();
    /** Default destructor */
    virtual ~PacketSource();

    /** Get the next packet
     *  @return The next packet OR if IsReady() would return false then a filler packet
     */
    virtual Packet* GetPacket(Packet* p)=0;

    /** Is there a packet ready to go?
     *  @param force - If true and the next packet's priority is holding it, then allow the packet to go anyway. Default false.
     *  @return true if there is a packet ready to go.
     */
    virtual bool IsReady(bool force=false)=0; // {return _readyFlag;};

    /** Report that an event happened */
    void SetEvent(Event event); // All packet sources can use the same code
    void ClearEvent(Event event){_eventList[event]=false;}; // All packet sources can use the same code
    bool GetEvent(Event event){return _eventList[event];};

  protected:
     bool _readyFlag;

  private:
     bool _eventList[EVENT_NUMBER_ITEMS];
};

} // vbit namespace

#endif // _PACKETSOURCE_H_
