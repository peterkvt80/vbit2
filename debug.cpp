#include <debug.h>

using namespace vbit;

Debug::Debug() :
    _debugLevel(logNONE)
{
    //ctor
    _magDurations.fill(-1);
    _magSizes.fill(0);
}

Debug::~Debug()
{
    //dtor
}

void Debug::Log(LogLevels level, std::string str)
{
    if (_debugLevel >= level){
        std::cerr << str + "\n"; // keep sending messages to stderr for now
    }
}

void Debug::SetMagCycleDuration(int mag, int duration)
{
    if (mag >= 0 && mag < 8)
    {
        _magDurations[mag] = duration;
        Log(logDEBUG, "[Debug::SetMagCycleDuration] Magazine " + std::to_string(mag) + " cycle duration: " + std::to_string(duration / 50) + " seconds, " + std::to_string(duration % 50) + " fields");
    }
}

void Debug::SetMagazineSize(int mag, int size)
{
    if (mag >= 0 && mag < 8)
    {
        _magSizes[mag] = (size<255)?size:255; // clamp erroneous sizes to 255
        Log(logDEBUG, "[Debug::SetMagazineSize] Magazine " + std::to_string(mag) + " size: " + std::to_string(size) + " pages");
    }
}
