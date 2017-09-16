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

  /** Find a page given the page number
   * @param filename The page number of the page (in MPPSS hex)
	 * \return A Page object if it exists, otherwise null
   */
  TTXPageStream* FindPage(char* pageNumber);

  // Probably want a nextPage function to scan using a wildcard

  /**
   * \brief Match - Find and mark all pages that match the page identity
   * \param page - A page identity string
   * \return - The number of pages that matched this identity
   */
  int Match(char* page);

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

  /** \brief Iterate through all pages
   *  \return Returns the next page or nullptr if we are at the end
   */
	TTXPageStream* NextPage();

  /** \brief Iterate through selected pages (using the P command)
   *  \return Returns the next page or nullptr if we are at the end
   */
  TTXPageStream* NextSelectedPage();

  /** \brief Reset the page iterator to the beginning
   *  Afterwards repeatedly call NextPage() until it returns nullptr
   *  \return the initial page or nullptr if there is no page
   */
  TTXPageStream* ResetIter();

private:
	Configure* _configure; // The configuration object
	std::list<TTXPageStream> _pageList[16]; /// The list of Pages in this service. Two lists per magazine (normal pages in lists 0-7, enhancement and magazine inventory and organisation table pages in 8-15)
	vbit::Mag* _mag[8];

	// iterators through selected pages. (use the same iterator for D command and MD, L etc.)
	uint8_t _iterMag;  /// Magazine number for the iterator
	std::list<TTXPageStream>::iterator _iter;  /// pages in a magazine

};

}

#endif
