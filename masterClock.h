#ifndef _MASTERCLOCK_H_
#define _MASTERCLOCK_H_

#include <cstdint>
#include <ctime>

namespace vbit
{
    class MasterClock {
        public:
            struct timeStruct {
                time_t seconds;
                uint8_t fields;
            };
            
            static MasterClock *Instance(){
                if (!instance)
                    instance = new MasterClock;
                return instance;
            }
            
            void SetMasterClock(timeStruct t){ _masterClock = t; }
            timeStruct GetMasterClock(){ return _masterClock; }
            
        private:
            MasterClock(){ _masterClock = {time(NULL)-1, 0}; }; // initialise master clock to system time - 1
            static MasterClock *instance;
            timeStruct _masterClock;
    };
}

#endif // _MASTERCLOCK_H_
