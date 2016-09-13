#ifndef _PAGELIST_H_
#define _PAGELIST_H_

#include <iostream>
#include <string>
#include <dirent.h>
#include <errno.h>
#include <vector>
#include <list>

#include "configure.h"
#include "ttxpagestream.h"
#include "mag.h"

namespace ttx
{
/** A PageList maintains the set of all teletext pages in a teletext service
 *  It can load, save, add, delete, edit pages.
 *
 */
class PageList
{
public:
	/** @brief Create am empty page list
	 */
	PageList(Configure *configure=NULL);
	~PageList();

	/**
	 * @param filepath Path to pages directory
	 * @param Return 0 if OK or errno
   */
	int LoadPageList(std::string filepath);

	inline vbit::Mag **GetMagazines(){vbit::Mag **p=_mag;return p;};

private:
	Configure* _configure; // The configuration object
	std::list<TTXPageStream> _pageList[8]; /// The list of Pages in this service. One list per magazine
	vbit::Mag* _mag[8];

};

}

#endif
