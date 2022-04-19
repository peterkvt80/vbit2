#ifndef _VBIT2_H_
#define _VBIT2_H_

#include <iostream>
#include <thread>
#include "service.h"
#include "configure.h"
#include "pagelist.h"
#include "filemonitor.h"
#include "command.h"

#ifdef WIN32
#include "fcntl.h"
#endif

namespace vbit
{
    class MasterClock {
        public:
            static MasterClock *Instance(){
                if (!instance)
                    instance = new MasterClock;
                return instance;
            }
            
            void SetMasterClock(time_t t){_masterClock = t;}
            time_t GetMasterClock(){return _masterClock;}
            
        private:
            MasterClock(){_masterClock = 0;}; // initialise master clock to unix epoch, it will be set when run() starts generating packets
            static MasterClock *instance;
            time_t _masterClock;
    };
}

#endif
