#ifndef _SUBPAGE_H_
#define _SUBPAGE_H_

#include <iostream>
#include <string>
#include <errno.h>

#include "configure.h"

namespace ttx 
{
/** A SubPage is a single frame of teletext
 *  It also contains some metadata such as subpage number, language and C flags.
 *  
 */
class SubPage
{
public:
	/** @brief Create am empty page list
	 */
	SubPage(Configure *configure=NULL);
	~SubPage();
	

private:
	Configure* _configure;
};

}

#endif
