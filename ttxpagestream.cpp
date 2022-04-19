#include "ttxpagestream.h"

TTXPageStream::TTXPageStream() :
    _transitionTime(0),
    _CarouselPage(NULL),
    _fileStatus(NEW),
    _loadedPacket29(false),
    _isCarousel(false),
    _isSpecial(false),
    _isNormal(false),
    _isUpdated(false),
    _updateCount(0)
{
    //ctor
}

TTXPageStream::TTXPageStream(std::string filename) :
    TTXPage(filename),
    _transitionTime(0),
    _CarouselPage(NULL),
    _fileStatus(NEW),
    _loadedPacket29(false),
    _isCarousel(false),
    _isSpecial(false),
    _isNormal(false),
    _isUpdated(false),
    _updateCount(0)
{
    struct stat attrib;               // create a file attribute structure
    stat(filename.c_str(), &attrib);  // get the attributes of the file
    _modifiedTime=attrib.st_mtime;
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
        return line;
    }
    // No more lines? return NULL.
    return NULL;
}

void TTXPageStream::StepNextSubpageNoLoop()
{
    if (_CarouselPage==NULL)
        _CarouselPage=this;
    else
        _CarouselPage=_CarouselPage->Getm_SubPage();
}

void TTXPageStream::StepNextSubpage()
{
    StepNextSubpageNoLoop();
    if (_CarouselPage==NULL) // Last carousel subpage? Loop to beginning
        _CarouselPage=this;
}

bool TTXPageStream::LoadPage(std::string filename)
{
    bool Loaded=false;
    //m_Init(); // Careful! We should move inits to the initialisation list and call the default constructor
    m_PageNumber=FIRSTPAGE; // Force to replace the root page rather than add to the carousel
    if (m_LoadTTI(filename))
        Loaded=true;
    return Loaded;
}

bool TTXPageStream::operator==(const TTXPageStream& rhs) const
{
    if (this->GetSourcePage()==rhs.GetSourcePage())
        return true;
    return false;
}

void TTXPageStream::IncrementUpdateCount()
{
    _updateCount = (_updateCount + 1) % 8;
}

void TTXPageStream::SetTransitionTime(int cycleTime)
{
    if (GetCycleTimeMode() == 'T')
        _transitionTime=time(nullptr) + cycleTime; // TODO: Uh-oh, this should be using the master clock
    else
        _cyclesRemaining=cycleTime;
}

bool TTXPageStream::Expired(bool StepCycles)
{
    // Has carousel timer expired
    if (GetCycleTimeMode() == 'T')
    {
        return _transitionTime<=time(nullptr); // TODO: Uh-oh, this should be using the master clock
    }
    else
    {
        if (StepCycles)
        {
            _cyclesRemaining--;
            _cyclesRemaining = (_cyclesRemaining<0)?0:_cyclesRemaining;
        }
        return _cyclesRemaining == 0;
    }
}

bool TTXPageStream::IsCarousel()
{
    if (Getm_SubPage()!=NULL) // has subpages
    {
        return true;
    }
    
    if (GetCycleTimeMode() == 'T' && GetPageStatus() & PAGESTATUS_C9_INTERRUPTED)
    {
        // no subpages, but interrupted sequence flag is set, and page is in timed mode, so treat as a 1 page carousel
        return true;
    }

    return false;
}
