/** Service
 */
#include "service.h"

using namespace ttx;
using namespace vbit;

Service::Service(Configure *configure, PageList *pageList) :
    _configure(configure),
    _pageList(pageList),
    _fieldCounter(49) // roll over immediately
{
    vbit::PacketMag **magList=_pageList->GetMagazines();
    // Register all the packet sources
    for (uint8_t mag=0;mag<8;mag++)
    {
        vbit::PacketMag* m=magList[mag];
        m->SetPriority(_configure->GetMagazinePriority(mag)); // set the mags to the desired priorities
        _register(m); // use the PacketMags created in pageList rather than duplicating them
    }
    
    // Add packet sources for subtitles and packet 830
    _register(_subtitle=new PacketSubtitle(_configure));
    _register(new Packet830(_configure));
    
    _register(_debug=new PacketDebug(_configure));
    
    _linesPerField = _configure->GetLinesPerField();
    
    _lineCounter = _linesPerField - 1; // roll over immediately
    
    // initialise master clock to unix epoch, it will be set when run() starts generating packets
    configure->SetMasterClock(0); // put master clock in configure so that sources can access it. Horrible spaghetti coding
}

Service::~Service()
{
}

void Service::_register(PacketSource *src)
{
    _Sources.push_front(src);
}

int Service::run()
{
    //std::cerr << "[Service::worker] This is the worker process" << std::endl;
    std::list<vbit::PacketSource*>::const_iterator iterator=_Sources.begin(); // Iterator for packet sources

    std::list<vbit::PacketSource*>::const_iterator first; // Save the first so we know if we have looped.

    vbit::Packet* pkt=new vbit::Packet(8,25,"                                        ");  // This just allocates storage.

    static vbit::Packet* filler=new vbit::Packet(8,25,"                                        ");  // A pre-prepared quiet packet to avoid eating the heap

    std::cerr << "[Service::run] Loop starts" << std::endl;
    std::cerr << "[Service::run] Lines per field: " << (int)_linesPerField << std::endl;
    while(1)
    {
        //std::cerr << "[Service::run]iterates. VBI line=" << (int) _lineCounter << " (int) field=" << (int) _fieldCounter << std::endl;
        // If counters (or other trigger) causes an event then send the events
        
        // Iterate through the packet sources until we get a packet to transmit
        
        vbit::PacketSource* p;
        first=iterator;
        bool force=false;
        uint8_t sourceCount=0;
        uint8_t listSize=_Sources.size();
        
        // Send ONLY one packet per loop
        _updateEvents();
        
        if (_debug->IsReady()) // Special case for debug. Ensures it can have the first line of field
        {
            if (_debug->GetPacket(pkt) != nullptr)
            {
                _packetOutput(pkt);
            }
            else
            {
                _packetOutput(filler);
            }
        }
        else if (_subtitle->IsReady()) // Special case for subtitles. Subtitles always go if there is one waiting
        {
            if (_subtitle->GetPacket(pkt) != nullptr)
            {
                _packetOutput(pkt);
            }
            else
            {
                _packetOutput(filler);
            }
        }
        else
        {
            // scan the rest of the available sources
            do
            {
                // Loop back to the first source
                if (iterator==_Sources.end())
                {
                    iterator=_Sources.begin();
                }

                // If we have tried all sources with and without force, then break out with a filler to prevent a deadlock
                if (sourceCount>listSize*2)
                {
                    p=nullptr;
                    // If we get a lot of this maybe there is a problem?
                    // std::cerr << "[Service::run] No packet available for this line" << std::endl;
                    break;
                }

                // If we have gone around once and got nothing, then force sources to go if possible.
                if (sourceCount>listSize)
                {
                    force=true;
                }

                // Get the packet source
                p=(*iterator);
                ++iterator;

                sourceCount++; // Count how many sources we tried.
            }
            while (!p->IsReady(force));
            
            // Did we find a packet? Then send it otherwise put out a filler
            if (p)
            {
                // GetPacket returns nullptr if the pkt isn't valid - if it's null go round again.
                if (p->GetPacket(pkt) != nullptr)
                {
                    _packetOutput(pkt);
                }
                else
                {
                    _packetOutput(filler);
                }
            }
            else
            {
                _packetOutput(filler);
            }
        }

    } // while forever
    return 99; // can't return but this keeps the compiler happy
} // worker

#define FORWARDSBUFFER 1

void Service::_updateEvents()
{
    time_t masterClock = _configure->GetMasterClock();
    
    // Step the counters
    _lineCounter = (_lineCounter + 1) % _linesPerField;
    
    if (_lineCounter == 0) // new field
    {
        _fieldCounter = (_fieldCounter + 1) % 50;
        
        time_t now;
        time(&now);
        
        if (masterClock > now + FORWARDSBUFFER) // allow vbit2 to run into the future before limiting packet rate
            std::this_thread::sleep_for(std::chrono::milliseconds(20)); // back off for ≈1 field to limit output to (less than) 50 fields per second
        
        if (_fieldCounter == 0)
        {
            masterClock++; // step the master clock before updating debug packet
        }
        
        _debug->TimeAndField(masterClock, _fieldCounter, now); // update the clocks in debugPacket.
        
        if (_fieldCounter == 0)
        {
            // if internal master clock is behind real time, or too far ahead, resynchronise it.
            if (masterClock < now || masterClock > now + FORWARDSBUFFER + 1)
            {
                masterClock = now;
                
                std::cerr << "[Service::_updateEvents] Resynchronising master clock" << std::endl; // emit warning on stderr
            }
            
            _configure->SetMasterClock(masterClock); // update 
            
            if (masterClock%15==0) // TODO: how often do we want to trigger sending special packets?
            {
                for (std::list<vbit::PacketSource*>::const_iterator iterator = _Sources.begin(), end = _Sources.end(); iterator != end; ++iterator)
                {
                    (*iterator)->SetEvent(EVENT_SPECIAL_PAGES);
                    (*iterator)->SetEvent(EVENT_PACKET_29);
                }
            }
        }
        // New field, so set the FIELD event in all the sources.
        for (std::list<vbit::PacketSource*>::const_iterator iterator = _Sources.begin(), end = _Sources.end(); iterator != end; ++iterator)
        {
            (*iterator)->SetEvent(EVENT_FIELD);
        }
        
        if (_fieldCounter%10==0) // Packet 830 happens every 200ms.
        {
            Event ev=EVENT_P830_FORMAT_1;
            switch (_fieldCounter/10)
            {
                case 0:
                {
                    ev=EVENT_P830_FORMAT_1;
                    break;
                }
                case 1:
                {
                    ev=EVENT_P830_FORMAT_2_LABEL_0;
                    break;
                }
                case 2:
                {
                    ev=EVENT_P830_FORMAT_2_LABEL_1;
                    break;
                }
                case 3:
                {
                    ev=EVENT_P830_FORMAT_2_LABEL_2;
                    break;
                }
                case 4:
                {
                    ev=EVENT_P830_FORMAT_2_LABEL_3;
                    break;
                }
            }
            for (std::list<vbit::PacketSource*>::const_iterator iterator = _Sources.begin(), end = _Sources.end(); iterator != end; ++iterator)
            {
                (*iterator)->SetEvent(ev);
            }
        }
        
        
        std::cout << std::flush;
    }
    
    // @todo Databroadcast events. Flag when there is data in the buffer.
}

void Service::_packetOutput(vbit::Packet* pkt)
{
    std::array<uint8_t, PACKETSIZE> *p = pkt->tx(_configure->GetMasterClock());
    
    /* t42 output */
    if (_configure->GetReverseFlag())
    {
        static std::array<uint8_t, PACKETSIZE> tmp;
        for (unsigned int i=0;i<(p->size());i++)
        {
            tmp[i]=ReverseByteTab[p->at(i)];
        }
        p = &tmp;
    }
    
    std::cout.write((char*)p->data()+3, 42); // have to cast the pointer to char for cout.write()

}
