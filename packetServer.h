#ifndef _PACKETSERVER_H_
#define _PACKETSERVER_H_

#include "configure.h"
#include "debug.h"
#include <mutex>

#ifdef WIN32
#include <winsock2.h>
#else
#include <fcntl.h>
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <sys/select.h> /* for fd_set() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <unistd.h>     /* for close() */
#endif

namespace ttx

{
    class PacketServer
    {
        public:
            PacketServer(ttx::Configure *configure, vbit::Debug *debug);
            ~PacketServer();
            
            void run();
            bool GetIsActive(){return _isActive;}; /* is the packet server running? */
            void SendField(std::vector<std::vector<uint8_t>> FrameBuffer);
            
        private:
            vbit::Debug* _debug;
            static const uint16_t MAXPENDING=5;
            static const uint16_t MAXCLIENTS=5;
            static const uint16_t BUFFLEN=256;
            
            int _portNumber;
            int _serverSock;
            
            int _clientSocks[MAXCLIENTS];
            std::mutex _mtx[MAXCLIENTS];
            
            bool _isActive;
            
            void DieWithError(std::string errorMessage); // handle fatal socket errors
    };
}

#endif
