/** debugging class
 */
#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <iostream>
#include <array>

namespace vbit
{
    class Debug
    {
        public:
            enum LogLevels
            {
                logNONE,
                logERROR,
                logWARN,
                logINFO,
                logDEBUG
            };
            
            /** Default constructor */
            Debug();
            /** Default destructor */
            virtual ~Debug();
            
            void Log(LogLevels level, std::string str);
            void SetDebugLevel(LogLevels level){ _debugLevel = level; };
            LogLevels GetDebugLevel(){ return _debugLevel; };
            void SetMagCycleDuration(int mag, int duration);
            std::array<int,8> GetMagCycleDurations(){ return _magDurations; };
            void SetMagazineSize(int mag, int size);
            std::array<int,8> GetMagSizes(){ return _magSizes; };
            
        protected:

        private:
            LogLevels _debugLevel;
            std::array<int, 8> _magDurations;
            std::array<int, 8> _magSizes;
    };
}

#endif // _DEBUG_H_
