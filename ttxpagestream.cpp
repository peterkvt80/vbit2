#include "ttxpagestream.h"

using namespace vbit;

TTXPageStream::TTXPageStream() :
    _transitionTime(0),
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
    if (std::shared_ptr<Subpage> s = GetSubpage())
    {
        if (s->GetTimedMode())
        {
            MasterClock *mc = mc->Instance();
            _transitionTime = mc->GetMasterClock().seconds + cycleTime;
        }
        else
        {
            _cyclesRemaining=cycleTime;
        }
    }
}

bool TTXPageStream::Expired(bool StepCycles)
{
    // Has carousel timer expired
    if (std::shared_ptr<Subpage> s = GetSubpage())
    {
        if (s->GetTimedMode())
        {
            MasterClock *mc = mc->Instance();
            if (_transitionTime == 0)
                return false; // catch race condition where we can check carousel before its timeout has been set
            return _transitionTime <= mc->GetMasterClock().seconds;
        }
    }
    
    if (StepCycles)
    {
        _cyclesRemaining--;
        _cyclesRemaining = (_cyclesRemaining<0)?0:_cyclesRemaining;
    }
    return _cyclesRemaining == 0;
}
