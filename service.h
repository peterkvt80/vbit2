#ifndef _SERVICE_H_
#define _SERVICE_H_

#include <iostream>
#include <iomanip>
#include <thread>
#include <ctime>
#include <list>

#include "configure.h"
#include "pagelist.h"
#include "packet.h"
#include "mag.h" // @todo THIS WILL BE REDUNDANT
#include "packetsource.h"
#include "packetmag.h"

/// Eight magazines and subtitles (maybe other packets too)
#define STREAMS 9

namespace ttx
{

/** A Service creates a teletext stream from packet sources.
 *  Packet sources are magazines, subtitles, Packet 830 and databroadcast.
 *  Service:
 *    Instances the packet sources
 *    Sends them timing events (cues for field timing etc.)
 *    Polls the packet sources for packets to send
 *    Sends the packets.
 *
 */
class Service
{
public:
	Service();
	/**
	 * @param configure A Configure object with all the settings
	 * @param pageList A pageList object already loaded with pages
	 */
	Service(Configure* configure, PageList* pageList);
	~Service();
	/**
	 * Creates a worker thread and does not terminate (at least for now)
	 * @return Nothing useful yet. Perhaps return an error status if something goes wrong
	 */
	int run();
private:
  // Member variables
	Configure* _configure; /// Member reference to the configuration settings
	PageList* _pageList; /// Member reference to the pages list

	// Member functions
	std::list<vbit::PacketSource*> _Sources;
	void _register(vbit::PacketSource *src);

};

}

#endif
