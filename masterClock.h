#ifndef _MASTERCLOCK_H_
#define _MASTERCLOCK_H_

#include <cstdint>

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
            MasterClock(){ _masterClock = {0, 0}; }; // initialise master clock to unix epoch, it will be set when run() starts generating packets
            static MasterClock *instance;
            timeStruct _masterClock;
    };
}

#endif // _MASTERCLOCK_H_
