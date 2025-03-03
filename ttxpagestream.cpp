#include "ttxpagestream.h"

TTXPageStream::TTXPageStream() :
    _transitionTime(0),
    _CarouselPage(nullptr),
    _fileStatus(NEW),
    _loadedPacket29(false),
    _loadedCustomHeader(false),
    _isCarousel(false),
    _isSpecial(false),
    _isNormal(false),
    _isUpdated(false),
    _updateCount(0)
{
    //ctor
    _mtx = new std::mutex();
}

TTXPageStream::TTXPageStream(std::string filename) :
    TTXPage(),
    _transitionTime(0),
    _CarouselPage(nullptr),
    _fileStatus(NEW),
    _loadedPacket29(false),
    _loadedCustomHeader(false),
    _isCarousel(false),
    _isSpecial(false),
    _isNormal(false),
    _isUpdated(false),
    _updateCount(0)
{
    _mtx = new std::mutex();
    struct stat attrib;               // create a file attribute structure
    stat(filename.c_str(), &attrib);  // get the attributes of the file
    _modifiedTime=attrib.st_mtime;
    m_LoadTTI(filename);
}

TTXPageStream::~TTXPageStream()
{
    //dtor
    //std::cerr << "TTXPageStream dtor\n";
}

TTXLine* TTXPageStream::GetTxRow(uint8_t row)
{
    // Return a line or nullptr if the row does not exist
    TTXLine* line=nullptr;
    if (IsCarousel())
    {
        line=_CarouselPage->GetRow(row);
    }
    else // single page
    {
        line=GetRow(row); // _lineCounter is implied
    }
    if (line!=nullptr) // Found a line
    {
        return line;
    }
    // No more lines? return NULL.
    return nullptr;
}

void TTXPageStream::StepNextSubpageNoLoop()
{
    if (_CarouselPage==nullptr)
        _CarouselPage=this;
    else
        _CarouselPage=_CarouselPage->Getm_SubPage();
}

void TTXPageStream::StepNextSubpage()
{
    StepNextSubpageNoLoop();
    if (_CarouselPage==nullptr) // Last carousel subpage? Loop to beginning
        _CarouselPage=this;
}

bool TTXPageStream::LoadPage(std::string filename)
{
    _mtx->lock();
    int num = GetPageNumber()>>8;
    bool Loaded = m_LoadTTI(filename);
    if (num != GetPageNumber()>>8)
    {
        // page number has changed so mark this page to get deleted and reloaded
        _fileStatus = MARKED;
    }
    
    _CarouselPage=this;
    
    return Loaded;
}

bool TTXPageStream::GetLock()
{
    if (_mtx->try_lock())
        return true; // ok
    return false; // couldn't get mutex
}

void TTXPageStream::FreeLock()
{
    _mtx->unlock();
}

bool TTXPageStream::operator==(const TTXPageStream& rhs) const
{
    if (this->GetFilename()==rhs.GetFilename())
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
    {
        vbit::MasterClock *mc = mc->Instance();
        _transitionTime = mc->GetMasterClock().seconds + cycleTime;
    }
    else
    {
        _cyclesRemaining=cycleTime;
    }
}

bool TTXPageStream::Expired(bool StepCycles)
{
    // Has carousel timer expired
    if (GetCycleTimeMode() == 'T')
    {
        vbit::MasterClock *mc = mc->Instance();
        return _transitionTime <= mc->GetMasterClock().seconds;
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
    if (Getm_SubPage()!=nullptr) // has subpages
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
