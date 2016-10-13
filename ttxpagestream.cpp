#include "ttxpagestream.h"

TTXPageStream::TTXPageStream() :
     _lineCounter(0),
     _isCarousel(false),
     _CurrentPage(this),
     _transitionTime(0)
{
    //ctor
}

TTXPageStream::TTXPageStream(std::string filename) :
     TTXPage(filename),
     _lineCounter(0),
     _isCarousel(false),
     _CurrentPage(this),
     _transitionTime(0)
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
    // @todo: To complicate matters
    // Increment the line
    _lineCounter++;
    // If we did the last line then return NULL.
    if (_lineCounter>=MAXROW)
    {
        _lineCounter=0;
        return NULL;
    }
    return GetCurrentRow();
}
