/** Service
 */
#include "service.h"
#include "vbit2.h"

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
    vbit::MasterClock *mc = mc->Instance();
    time_t masterClock = mc->GetMasterClock();
    
    // Step the counters
    _lineCounter = (_lineCounter + 1) % _linesPerField;
    
    if (_lineCounter == 0) // new field
    {
        _fieldCounter = (_fieldCounter + 1) % 50;
        
        time_t now;
        time(&now);
        
        if (masterClock > now + FORWARDSBUFFER) // allow vbit2 to run into the future before limiting packet rate
            std::this_thread::sleep_for(std::chrono::milliseconds(40)); // back off for ≈2 fields to limit output to (less than) 50 fields per second
        
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
            
            mc->SetMasterClock(masterClock); // update the master clock singleton
            
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
    std::array<uint8_t, PACKETSIZE> *p = pkt->tx();
    
    Configure::OutputFormat OutputFormat = _configure->GetOutputFormat();
    
    switch (OutputFormat)
    {
        case Configure::OutputFormat::T42:
        {
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
            
            break;
        }
        
        case Configure::OutputFormat::Raw:
        {
            /* full 45 byte teletext packets */
            std::cout.write((char*)p->data(), 45); // have to cast the pointer to char for cout.write()
            
            break;
        }
        
        case Configure::OutputFormat::PES:
        {
            /* Packetized Elementary Stream for insertion into MPEG-2 transport stream */
            
            if (_lineCounter == 0)
            {
                // a new field has started - transmit data for previous field if there is any
                if (!(_PESBuffer.empty()))
                {
                    std::array<uint8_t, 46> padding;
                    padding.fill(0xff);
                    
                    std::vector<uint8_t> header = {0x00, 0x00, 0x01, 0xBD};
                    
                    int numBlocks = _PESBuffer.size() + 1; // header and N lines
                    int numTSPackets = ((numBlocks * 46) + 183) / 184; // round up
                    int packetLength = (numTSPackets * 184) - 6;
                    
                    header.push_back(packetLength >> 8);
                    header.push_back(packetLength & 0xff);
                    
                    /* bits | 7 | 6 |  5   | 4   |     3    |     2     |     1     |     0    |
                            | 1 | 0 | Scrambling | Priority | Alignment | Copyright | Original | */
                    header.push_back(0x85); // Align, Original
                    
                    /* bits |  7 | 6  |   5  |    4    |     3     |     2     |    1    |       0       |
                            | PTS DTS | ESCR | ES rate | DSM trick | copy info | PES CRC | PES extension |*/
                    header.push_back(0x00); // No PTS
                    
                    header.push_back(0x24); // PES header data length
                    
                    /* 
                    uint64_t PTS = 0; // ???
                    
                    // append PTS
                    header.push_back(0x21 | (PTS >> 30));
                    header.push_back((PTS & 0x3FC00000) >> 22);
                    header.push_back(0x01 | ((PTS & 0x3F8000) >> 14));
                    header.push_back((PTS & 0x7F80) >> 7);
                    header.push_back(0x01 | ((PTS & 0x7F) << 1));
                    */
                    
                    header.resize(0x2D, 0xff); // make PES header up to 45 bytes long with stuffing bytes.
                    
                    header.push_back(0x10); // append PES data identifier (EBU data)
                    
                    std::cout.write((char*)header.data(), header.size()); // output PES header and data_identifier
                    
                    for (unsigned int i = 0; i < _PESBuffer.size(); i++)
                    {
                        std::cout.write((char*)_PESBuffer[i].data(), 46);
                    }
                    
                    for (int i = numBlocks; i < numTSPackets * 4; i++)
                    {
                        std::cout.write((char*)padding.data(), 46); // pad out remainder of PES packet
                    }
                    
                    _PESBuffer.clear(); // empty buffer ready for next frame's packets
                }
            }
            
            std::vector<uint8_t> data = {0x02, 0x2c}; // data_unit_id and data_unit_length (EBU teletext non-subtitle, 44 bytes)
            
            if (_lineCounter > 15)
            {
                data.push_back(((_fieldCounter&1)^1) << 5); //field parity, line number undefined
            }
            else
            {
                data.push_back((((_fieldCounter&1)^1) << 5) | (_lineCounter + 7)); // field parity and line number
            }
            
            for (int i = 2; i < 45; i++)
            {
                data.push_back(ReverseByteTab[p->at(i)]); // bits are reversed in PES stream
            }
            
            _PESBuffer.push_back(data);
            
            break;
        }
    }

}
