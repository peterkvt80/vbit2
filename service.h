#ifndef _SERVICE_H_
#define _SERVICE_H_

#include <iostream>
#include <iomanip>
#include <thread>
#include <ctime>

#include "configure.h"
#include "pagelist.h"
#include "packet.h"
#include "mag.h"

/// Eight magazines and subtitles (maybe other packets too)
#define STREAMS 9

namespace ttx
{
/** A Service has a name, source folder, a header format.
 *  It loads in pages from the source folder and generates a teletext stream on stdout.
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
	Configure* _configure; /// Member reference to the configuration settings
	PageList* _pageList; /// Member reference to the pages list

};

}

#endif
