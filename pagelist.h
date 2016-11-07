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
/** @brief A PageList maintains the set of all teletext pages in a teletext service
 *  It can load, save, add, delete, edit pages.
 *  Internally each magazine has its own list of pages.
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

	vbit::Mag **GetMagazines(){vbit::Mag **p=_mag;return p;};

  /** Return the page object that was loaded from <filename>
   * @param filename The filename of the page we are looking for.
   */
  TTXPageStream* Locate(std::string filename);

  /** Add a teletext page to the proper magazine
   * @param page TTXPageStream object that has already been loaded
   */
	void AddPage(TTXPageStream* page);

  /** Clear all the exists flags
   */
	void ClearFlags();
  /** Delete all pages that no longer exist
   */
	void DeleteOldPages();

private:
	Configure* _configure; // The configuration object
	std::list<TTXPageStream> _pageList[8]; /// The list of Pages in this service. One list per magazine
	vbit::Mag* _mag[8];

};

}

#endif
