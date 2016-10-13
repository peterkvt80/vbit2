#include "ttxpagestream.h"

TTXPageStream::TTXPageStream() :
     _lineCounter(0),
     _isCarousel(false),
     _transitionTime(0),
		 _CarouselPage(this)
{
    //ctor
}

TTXPageStream::TTXPageStream(std::string filename) :
     TTXPage(filename),
     _lineCounter(0),
     _isCarousel(false),
     _transitionTime(0),
		 _CarouselPage(this)
 {


 }

TTXPageStream::~TTXPageStream()
{
    //dtor
}

TTXLine* TTXPageStream::GetCurrentRow()
{
    // @todo Check everything in sight.
		// Not quite. the first packets out should be magazines
    TTXLine* line=GetRow(_lineCounter);
    // std::cerr << "[TTXPageStream::GetRow]This line is " << line->GetLine() << " linecounter=" << _lineCounter << std::endl;
    return line;
}

TTXLine* TTXPageStream::GetNextRow()
{
    // To complicate matters, a non-carousel page just uses GetCurrentRow
		// but a carousel needs to know what page to look at.
		// To even further complicate things, this is where the enhanced packets should go.
    // Increment the line
    _lineCounter++;
		
    // If we did the last line then return NULL.
    if (_lineCounter>=MAXROW)
    {
        _lineCounter=0;			
        return NULL;
    }
		if (IsCarousel())
		{
			//return GetCurrentRow();
			return _CarouselPage->GetRow(_lineCounter);
		}
		else // single page
		{
			return GetCurrentRow();
		}
}

void TTXPageStream::StepNextSubpage()
{
	if (_CarouselPage==NULL) _CarouselPage=this;
	_CarouselPage=_CarouselPage->Getm_SubPage();
	if (_CarouselPage==NULL) // Last carousel subpage? Loop to beginning
		_CarouselPage=this;
}

