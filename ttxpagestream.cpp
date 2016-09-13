#include "ttxpagestream.h"

TTXPageStream::TTXPageStream()
{
    //ctor
}

TTXPageStream::TTXPageStream(std::string filename) :
     TTXPage(filename),
     _lineCounter(0),
     _isCarousel(false),
     _CurrentPage(this)
 {


 }

TTXPageStream::~TTXPageStream()
{
    //dtor
}

TTXLine* TTXPageStream::GetCurrentRow()
{
    // @todo Check everything in sight
    TTXLine* line=GetRow(_lineCounter);
    std::cerr << "[TTXPageStream::GetRow]This line is " << line->GetLine() << " linecounter=" << _lineCounter << std::endl;
    return line;
}

TTXLine* TTXPageStream::GetNextRow()
{
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
