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
            void SetMagCycleDuration(int mag, int duration);
            std::array<int,8> GetMagCycleDurations(){ return _magDurations; };
            
        protected:

        private:
            LogLevels _debugLevel;
            std::array<int, 8> _magDurations;
    };
}

#endif // _DEBUG_H_
