#ifndef _DATACASTSERVER_H_
#define _DATACASTSERVER_H_

#include "configure.h"
#include "debug.h"
#include "packet.h"
#include "packetDatacast.h"

#ifdef WIN32
#include <winsock2.h>
#else
#include <fcntl.h>
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <sys/select.h> /* for fd_set() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <unistd.h>     /* for close() */
#endif

#define DCSET       0x00
#define DCRAW       0x01
#define DCFORMATA   0x02

#define DCOK    0x00    /* command successful */
#define DCTRUNC 0xfd    /* command completed but data was truncated */
#define DCFULL  0xfe    /* command failed as buffer is full */
#define DCERR   0xff    /* command failed */

namespace ttx

{
    class DatacastServer
    {
        public:
            DatacastServer(ttx::Configure *configure, vbit::Debug *debug);
            ~DatacastServer();
            
            void run();
            bool GetIsActive(){return _isActive;}; /* is the packet server running? */
            
            
            vbit::PacketDatacast** GetDatachannels() { vbit::PacketDatacast **channels=_datachannel; return channels; };
            
        private:
            vbit::Debug* _debug;
            vbit::PacketDatacast* _datachannel[16]; /* array of datacast sources */
            
            static const uint16_t MAXPENDING=5;
            static const uint16_t MAXCLIENTS=5;
            
            int _portNumber;
            int _serverSock;
            
            int _clientSocks[MAXCLIENTS];
            int _clientChannel[MAXCLIENTS];
            
            bool _isActive;
            
            void SocketError(std::string errorMessage); // handle fatal socket errors
    };
}

#endif
