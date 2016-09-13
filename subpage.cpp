/** SubPage
 */
#include "subpage.h"
 
using namespace ttx;

SubPage::SubPage(Configure *configure) :
	_configure(configure)
{
	std::cout << "[SubPage::SubPage] ctor" << std::endl;
	if (_configure==NULL)
	{
		std::cerr << "NULL configuration object" << std::endl;
		return;
	}
	std::cout << "[SubPage::SubPage] ctor exits" << std::endl;}

SubPage::~SubPage()
{
	std::cout << "[SubPage] Destructor" << std::endl;
}

