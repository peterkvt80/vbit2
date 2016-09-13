/** Page
 */
#include "page.h"

///@todo Replace with c++ equivalent
#include <stdio.h>
using namespace ttx;

Page::Page(std::string filename, Configure *configure) :
	_filename(filename), _configure(configure)
{
	std::cout << "[Page::Page] entered " << filename << std::endl;
	if (_configure==NULL)
	{
		std::cerr << "NULL configuration object" << std::endl;
		return;
	}
	// _subPages = new SubPages(configure);
	LoadPage(_filename);
}

Page::~Page()
{
	std::cout << "[Page] ~ Destructor" << std::endl;
}

int Page::LoadPage(std::string filename)
{
    char str[132];
    std::string line;
	std::cout << "[Page::LoadPage] Loading page from " << filename << std::endl;
    std::ifstream pagefile;
    pagefile.open(filename);
    if (pagefile.is_open())
    {
        while (getline(pagefile,line))

    }


    FILE** fp;
    *fp=fopen(filename.c_str(),"r");
    if (!*fp)
    {
        *fp=NULL;
        return 0;
    }
    while (!feof(*fp))	// Parse the carousel
    {
        fgets(str,MAXLINE,*fp);
        std::cout << "String=" << str << std::endl;
    }
	/// @todo Extract the stop level meta data
	/// @todo Create a new sub page and add it to the list
	SubPage *sp = new SubPage(_configure);
	_subPages.push_back(*sp);
	/// @todo Extract the per-frame meta data and send it to SubPage
	/// @todo Extract the page data and send it to SubPage

	std::cout << "[Page::LoadPage] Loading page done " << std::endl;
	return 0;
}
