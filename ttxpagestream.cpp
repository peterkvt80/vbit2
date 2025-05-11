#include "ttxpagestream.h"

using namespace vbit;

TTXPageStream::TTXPageStream() :
    _transitionTime(0),
    _CarouselPage(nullptr),
    _loadedPacket29(false),
    _loadedCustomHeader(false),
    _isCarousel(false),
    _isSpecial(false),
    _isNormal(false),
    _isUpdated(false),
    _updateCount(0),
    _deleteFlag(false)
{
    //ctor
    _mtx.reset(new std::mutex());
}

TTXPageStream::~TTXPageStream()
{
    //dtor
    //std::cerr << "TTXPageStream dtor\n";
}

std::shared_ptr<TTXLine> TTXPageStream::GetTxRow(uint8_t row)
{
    // Return a line or nullptr if the row does not exist
    std::shared_ptr<TTXLine> line=nullptr;
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
        _CarouselPage=this->getptr();
    else
        _CarouselPage=_CarouselPage->Getm_SubPage();
}

void TTXPageStream::StepNextSubpage()
{
    StepNextSubpageNoLoop();
    if (_CarouselPage==nullptr) // Last carousel subpage? Loop to beginning
        _CarouselPage=this->getptr();
}

bool TTXPageStream::LoadPage(std::string filename)
{
    bool Loaded = m_LoadTTI(filename);
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

void TTXPageStream::IncrementUpdateCount()
{
    _updateCount = (_updateCount + 1) % 8;
}

void TTXPageStream::SetTransitionTime(int cycleTime)
{
    if (GetCycleTimeMode() == 'T')
    {
        MasterClock *mc = mc->Instance();
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
        MasterClock *mc = mc->Instance();
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
