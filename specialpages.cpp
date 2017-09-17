
#include "specialpages.h"

using namespace vbit;

SpecialPages::SpecialPages()
{

}

SpecialPages::~SpecialPages()
{

}

void SpecialPages::addPage(TTXPageStream* p)
{
	_specialPagesList.push_front(p);
}

void SpecialPages::deletePage(TTXPageStream* p)
{
	_specialPagesList.remove(p);
}

TTXPageStream* SpecialPages::NextPage()
{

}

TTXPageStream* SpecialPages::ResetIter()
{

}
