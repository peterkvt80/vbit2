#include "ttxpagestream.h"

TTXPageStream::TTXPageStream() :
     _isCarousel(false),
     _transitionTime(0),
		 _CarouselPage(this)
{
    //ctor
}

TTXPageStream::TTXPageStream(std::string filename) :
     TTXPage(filename),
     _isCarousel(false),
     _transitionTime(0),
		 _CarouselPage(this)
 {


 }

TTXPageStream::~TTXPageStream()
{
    //dtor
}


TTXLine* TTXPageStream::GetTxRow(uint8_t row)
{
  // Return a line OR NULL if the row does not exist
  TTXLine* line=NULL;
  if (IsCarousel())
  {
    line=_CarouselPage->GetRow(row);
  }
  else // single page
  {
    line=GetRow(row); // _lineCounter is implied
  }
  if (line!=NULL) // Found a line
  {
    // std::cerr << " (R" << (int)row << ") ";
    return line;
  }
  // No more lines? return NULL.
  // std::cerr << " GetNextRow " << _lineCounter << std::endl;
  return NULL;
}

void TTXPageStream::StepNextSubpage()
{
	if (_CarouselPage==NULL) _CarouselPage=this;
	_CarouselPage=_CarouselPage->Getm_SubPage();
	if (_CarouselPage==NULL) // Last carousel subpage? Loop to beginning
		_CarouselPage=this;
}

void TTXPageStream::printList()
{
      std::cerr << "DUMP TODO" << std::endl;
}

