/** Service
 */
#include "service.h"

using namespace ttx;
using namespace vbit;

Service::Service(Configure *configure, vbit::Debug *debug, PageList *pageList, PacketServer *packetServer) :
    _configure(configure),
    _debug(debug),
    _pageList(pageList),
    _packetServer(packetServer),
    _fieldCounter(49) // roll over immediately
{
    _magList=_pageList->GetMagazines();
    // Register all the packet sources
    for (uint8_t mag=0;mag<8;mag++)
    {
        vbit::PacketMag* m=_magList[mag];
        m->SetPriority(_configure->GetMagazinePriority(mag)); // set the mags to the desired priorities
        _register(m); // use the PacketMags created in pageList rather than duplicating them
    }
    
    // Add packet sources for subtitles and packet 830
    _register(_subtitle=new PacketSubtitle(_configure, _debug));
    _register(new Packet830(_configure));
    
    _register(_packetDebug=new PacketDebug(_configure, _debug));
    
    _linesPerField = _configure->GetLinesPerField();
    
    _lineCounter = _linesPerField - 1; // roll over immediately
    
    _OutputFormat = _configure->GetOutputFormat();
    _PTS = 0;
    _PTSFlag = true;
    _PID = _configure->GetTSPID();
    _tscontinuity = 0;
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
    _debug->Log(Debug::LogLevels::logDEBUG,"[Service::run] This is the worker process");
    std::list<vbit::PacketSource*>::const_iterator iterator=_Sources.begin(); // Iterator for packet sources

    std::list<vbit::PacketSource*>::const_iterator first; // Save the first so we know if we have looped.

    vbit::Packet* pkt=new vbit::Packet(8,25,"                                        ");  // This just allocates storage.

    static vbit::Packet* filler=new vbit::Packet(8,25,"                                        ");  // A pre-prepared quiet packet to avoid eating the heap


    _debug->Log(Debug::LogLevels::logINFO,"[Service::run] Lines per field: " + std::to_string((int)_linesPerField));
    while(1)
    {
        // If counters (or other trigger) causes an event then send the events
        
        // Iterate through the packet sources until we get a packet to transmit
        
        vbit::PacketSource* p;
        first=iterator;
        bool force=false;
        uint8_t sourceCount=0;
        uint8_t listSize=_Sources.size();
        
        // Send ONLY one packet per loop
        _updateEvents();
        
        // Special case for debug. Ensures it can have the first line of field
        if (_packetDebug->IsReady(_debug->GetDebugLevel() >= Debug::LogLevels::logDEBUG)) // force if log level DEBUG
        {
            _packetDebug->GetPacket(pkt);
            _packetOutput(pkt);
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

#define FORWARDSBUFFER 1 // how far into the future vbit2 should run before rate limiting in seconds

void Service::_updateEvents()
{
    vbit::MasterClock *mc = mc->Instance();
    vbit::MasterClock::timeStruct masterClock = mc->GetMasterClock();
    
    // Step the counters
    _lineCounter = (_lineCounter + 1) % _linesPerField;
    
    if (_lineCounter == 0) // new field
    {
        _fieldCounter = (_fieldCounter + 1) % 50;
        
        time_t now;
        time(&now);
        
        if (masterClock.seconds > now + FORWARDSBUFFER) // allow vbit2 to run into the future before limiting packet rate
            std::this_thread::sleep_for(std::chrono::milliseconds(40)); // back off for ≈2 fields to limit output to (less than) 50 fields per second
        
        if (_fieldCounter == 0)
        {
            masterClock.seconds++; // step the master clock before updating debug packet
        }
        
        masterClock.fields = _fieldCounter;
        
        _packetDebug->TimeAndField(masterClock, now, false); // update the clocks in debugPacket.
        
        if (_fieldCounter == 0)
        {
            // if internal master clock is behind real time, or too far ahead, resynchronise it.
            if (masterClock.seconds < now || masterClock.seconds > now + FORWARDSBUFFER + 1)
            {
                masterClock.seconds = now;
                
                _debug->Log(Debug::LogLevels::logWARN,"[Service::_updateEvents] Resynchronising master clock");
                
                for (int i=0;i<8;i++)
                    _magList[i]->InvalidateCycleTimestamp(); // reset magazine cycle duration calculations
                
                _packetDebug->TimeAndField(masterClock, now, true); // update the clocks in debugPacket.
            }
            
            if (masterClock.seconds%15==0) // TODO: how often do we want to trigger sending special packets?
            {
                for (std::list<vbit::PacketSource*>::const_iterator iterator = _Sources.begin(), end = _Sources.end(); iterator != end; ++iterator)
                {
                    (*iterator)->SetEvent(EVENT_SPECIAL_PAGES);
                    (*iterator)->SetEvent(EVENT_PACKET_29);
                }
            }
        }
        
        
        
        mc->SetMasterClock(masterClock); // update the master clock singleton
        
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
    
    switch (_OutputFormat)
    {
        case Configure::OutputFormat::None:
        {
            /* disable stdout */
            break;
        }
        
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
        
        case Configure::OutputFormat::TSNPTS:
            _PTSFlag = false; // Don't generate PCR and PTS in output stream
            /* fallthrough */
            [[gnu::fallthrough]];
        case Configure::OutputFormat::TS:
        {
            /* MPEG-2 transport stream holding a DVB-TXT Packetized Elementary Stream */
            
            if (_lineCounter == 0 && !(_fieldCounter&1))
            {
                // a new frame has started - transmit data for previous frame if there is any
                if (!(_PESBuffer.empty()))
                {
                    std::array<uint8_t, 46> padding;
                    padding.fill(0xff);
                    
                    std::array<uint8_t, 188> ts;
                    ts.fill(0xff);
                    
                    ts[0] = 0x47;
                    ts[1] = (uint8_t)(_PID >> 8);
                    ts[2] = (uint8_t)(_PID & 0xFF);
                    
                    if (_PTSFlag){
                        ts[3] = 0x20 | _tscontinuity; // adaption field no payload
                        ts[4] = 0x07; // 7 bytes in adaption field
                        ts[5] = 0x10; // PCR flag
                        // make PCR from our PTS
                        ts[6] = _PTS >> 25;
                        ts[7] = (_PTS >> 17) & 0xFF;
                        ts[8] = (_PTS >> 9) & 0xFF;
                        ts[9] = (_PTS >> 1) & 0xFF;
                        ts[10] = (_PTS & 1) << 7;
                        ts[11] = 0x00;
                        std::cout.write((char*)ts.data(), 188); // write out transport stream packet
                    }
                    
                    std::vector<uint8_t> header = {0x00, 0x00, 0x01, 0xBD}; // PES start code
                    
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
                    header.push_back(_PTSFlag?0x80:0x00); // if _PTSFlag, PTS no DTS follows
                    
                    header.push_back(0x24); // PES header data length
                    
                    if (_PTSFlag)
                    {
                        // append PTS
                        header.push_back(0x21 | ((_PTS & 0x1C0000000) >> 29));
                        header.push_back((_PTS & 0x3FC00000) >> 22);
                        header.push_back(0x01 | ((_PTS & 0x3F8000) >> 14));
                        header.push_back((_PTS & 0x7F80) >> 7);
                        header.push_back(0x01 | ((_PTS & 0x7F) << 1));
                        
                        _PTS += 3600;
                        if (_PTS >= 0x200000000)
                            _PTS = 0;
                    }
                    
                    header.resize(0x2D, 0xff); // make PES header up to 45 bytes long with stuffing bytes.
                    
                    header.push_back(0x10); // append PES data identifier (EBU data)
                    
                    ts[1] = (uint8_t)((_PID >> 8) | 0x40);
                    
                    _tscontinuity = (_tscontinuity+1)&0xf;
                    ts[3] = 0x10 | _tscontinuity; // no adaption field payload only
                    
                    std::cout.write((char*)ts.data(), 4); // transport stream header
                    
                    std::cout.write((char*)header.data(), header.size()); // output PES header and data_identifier
                    
                    for (unsigned int i = 0; i < _PESBuffer.size(); i++)
                    {
                        if (((i % 184) % 4) == 3) // new ts packet
                        {
                            ts[1] = (uint8_t)(_PID >> 8);
                            _tscontinuity = (_tscontinuity+1)&0xf;
                            ts[3] = 0x10 | _tscontinuity;
                            std::cout.write((char*)ts.data(), 4); // transport stream header
                        }
                        std::cout.write((char*)_PESBuffer[i].data(), 46);
                    }
                    
                    for (int i = numBlocks; i < numTSPackets * 4; i++)
                    {
                        std::cout.write((char*)padding.data(), 46); // pad out remainder of PES packet
                    }
                    
                    _PESBuffer.clear(); // empty buffer ready for next field's packets
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
    
    if (_packetServer->GetIsActive())
    {
        // packet server needs feeding
        
        if (_lineCounter == 0 && !(_fieldCounter&1))
        {
            // a new field has started 
            
            _packetServer->SendField(_FrameBuffer);
            
            _FrameBuffer.clear(); // empty buffer ready for next frame's packets
        }
        
        /* internal format only: 45 bytes in the form field count, line high byte, line low byte, 42 payload bytes */
        std::vector<uint8_t> data = {_fieldCounter, (uint8_t)(_lineCounter >> 8), (uint8_t)(_lineCounter & 0xFF)};
        
        for (int i = 3; i < 45; i++)
        {
            data.push_back(p->at(i));
        }
        
        _FrameBuffer.push_back(data);
    }
}
