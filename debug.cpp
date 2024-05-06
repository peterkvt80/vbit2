#include <debug.h>

using namespace vbit;

Debug::Debug() :
    _debugLevel(logNONE)
{
    //ctor
    _magDurations.fill(-1);
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
        Log(logDEBUG, "Magazine " + std::to_string(mag) + " cycle duration: " + std::to_string(duration / 50) + " seconds, " + std::to_string(duration % 50) + " fields");
    }
}
