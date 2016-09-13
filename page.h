#ifndef _PAGE_H_
#define _PAGE_H_

#include <iostream>
#include <fstream>
#include <string>
#include <errno.h>
#include <list>

#include "configure.h"
#include "subpage.h"

#define MAXLINE 132

namespace ttx
{
/** A Page is a set of subpages plus the top level meta data such as magazine and page number.
 *
 */
class Page
{
public:
	/** @brief Create am empty page list
	 */
	Page(std::string filename, Configure *configure=NULL);
	~Page();

	/**
	 * @param filename of page
	 * @param Return 0 if OK or errno
   */
	int LoadPage(std::string filename);


	inline int GetPageNumber() const {return _pageNumber;}

	inline bool operator==(const Page& other) const {return (GetPageNumber() == other.GetPageNumber()); }
	//inline bool operator!=(const Page& lhs, const Page& rhs) const {return !operator==(lhs,rhs);}
	inline bool operator< (const Page& other) const {return (GetPageNumber() <  other.GetPageNumber()); }
	//inline bool Page::operator> (const Page& lhs, const Page& rhs)const{return  operator< (rhs,lhs);}
	//inline bool Page::operator<=(const Page& lhs, const Page& rhs)const{return !operator> (lhs,rhs);}
	//inline bool Page::operator>=(const Page& lhs, const Page& rhs)const{return !operator< (lhs,rhs);}

private:
	std::string _filename; /// The filename of this page
	Configure* _configure; /// settings singleton
	uint32_t	_pageNumber;  /// The mpp. Range 0x100 to 0x8fe
	std::list<SubPage> _subPages; /// The frames in this carousel set. Non carousel page will have exactly one subpage.

public:
	/**
	 * @return The filename of this page
	 */
	inline std::string GetFilename() const {return _filename;}
};


}

#endif
