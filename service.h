#ifndef _SERVICE_H_
#define _SERVICE_H_

#include <iostream>
#include <iomanip>
#include <thread>
#include <ctime>
#include <list>

#include "configure.h"
#include "debug.h"
#include "pagelist.h"
#include "packetServer.h"
#include "interfaceServer.h"
#include "packet.h"
#include "packetsource.h"
#include "packetmag.h"
#include "packet830.h"
#include "packetDebug.h"
#include "masterClock.h"

namespace vbit
{
    /** A Service creates a teletext stream from packet sources.
     *  Packet sources are magazines, Packet 8/30, databroadcast, and debugging.
     *  Service:
     *    Instances the packet sources
     *    Sends them timing events (cues for field timing etc.)
     *    Polls the packet sources for packets to send
     *    Sends the packets.
     */
    class Service
    {
        public:
            /**
             * @param configure A Configure object with all the settings
             * @param pageList A pageList object already loaded with pages
             */
            Service(Configure* configure, Debug* debug, PageList* pageList, PacketServer* packetServer, InterfaceServer *interfaceServer);
            
            ~Service();
            
            /**
             * Creates a worker thread and does not terminate (at least for now)
             * @return Nothing useful yet. Perhaps return an error status if something goes wrong
             */
            int run();

        private:
            // Member variables that define the service
            Configure* _configure; /// Member reference to the configuration settings
            Debug* _debug;
            PageList* _pageList; /// Member reference to the pages list
            PacketMag** _magList;
            PacketServer* _packetServer;
            InterfaceServer* _interfaceServer;

            // Member variables for event management
            uint16_t _linesPerField;
            uint16_t _datacastLines;
            uint16_t _lineCounter; // Which VBI line are we on? Used to signal a new field.
            uint8_t _fieldCounter; // Which field? Used to time packet 8/30
            
            uint64_t _PTS; // presentation timestamp counter
            bool _PTSFlag; // generate PCR and PTS
            
            std::list<PacketSource*> _magazineSources; // A list of packet sources for magazine data
            std::list<PacketSource*> _datacastSources; // A list of sources for independent data line packets

            Packet830* _packet830; // BSDP packet source
            PacketDebug* _packetDebug; // Debug packet source

            // Member functions
            void _register(std::list<PacketSource*> *list, PacketSource *src); // Register packet sources

            /**
             * @brief Check if anything changed, and if so signal the event to the packet sources.
             * Must be called once per transmitted row so that it can maintain a field count
             */
            void _updateEvents();
            
            /* output a packet in the desired format */
            void _packetOutput(Packet* pkt);
            
            /* queue up packets for outputting as a Packetised Elementary Stream */
            std::vector<std::vector<uint8_t>> _PESBuffer;
            
            Configure::OutputFormat _OutputFormat;
            uint16_t _PID;
            uint8_t _tscontinuity;
            
            /* queue up a frame of packets for the packet server */
            std::vector<std::vector<uint8_t>> _FrameBuffer;
    };
}

#endif
