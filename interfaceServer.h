#ifndef _INTERFACESERVER_H_
#define _INTERFACESERVER_H_

#include "configure.h"
#include "debug.h"
#include "pagelist.h"
#include "ttxpagestream.h"
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

/* interface server command numbers */
#define DCSET       0x00    /* set datachannel for subsequent commands */
#define DCRAW       0x01    /* push raw packet data to datacast buffer */
#define DCFORMATA   0x02    /* push format A packet to datacast buffer */
#define DCFORMATB   0x03    /* placeholder - may never implement */
#define CONFIGAPI   0x04    /* vbit2 configuration API command */
#define PAGESAPI    0x05    /* page data API */

/* interface server response codes */
#define DCOK        0x00    /* command successful */
#define DCTRUNC     0xfd    /* command completed but data was truncated */
#define DCFULL      0xfe    /* command failed as buffer is full */
#define DCERR       0xff    /* command failed */

/* command numbers for vbit2 configuration API */
#define CONFRAFLAG  0x00    /* get/set row adaptive flag */
#define CONFRBYTES  0x01    /* get/set BSDP reserved bytes */
#define CONFSTATUS  0x02    /* get/set 20 byte BSDP status message */
#define CONFHEADER  0x03    /* get/set 32 byte header template */

/* command numbers for page data API */
#define PAGEDELETE  0x00    /* remove a page from the service */
#define PAGEOPEN    0x01    /* open a page for updating */
#define PAGESETSUB  0x02    /* select subpage */
#define PAGEDELSUB  0x03    /* delete subpage */
#define PAGECLOSE   0x04    /* close updated page */
#define PAGEFANDC   0x05
#define PAGEOPTNS   0x06
#define PAGEROW     0x07
#define PAGELINKS   0x08

namespace vbit

{
    class ClientState
    {
        public:
            int socket = -1;
            int channel = -1;
            std::shared_ptr<TTXPageStream> page = nullptr;
            std::shared_ptr<Subpage> subpage = nullptr;
    };
    class InterfaceServer
    {
        public:
            InterfaceServer(Configure *configure, Debug *debug, PageList *pageList);
            ~InterfaceServer();
            
            void run();
            bool GetIsActive(){return _isActive;}; /* is the packet server running? */
            
            
            PacketDatacast** GetDatachannels() { PacketDatacast **channels=_datachannel; return channels; };
            
        private:
            Configure* _configure;
            Debug* _debug;
            PageList* _pageList;
            PacketDatacast* _datachannel[16]; /* array of datacast sources */
            
            static const uint16_t MAXPENDING=5;
            static const uint16_t MAXCLIENTS=5;
            
            int _portNumber;
            int _serverSock;
            
            ClientState _clientState[MAXCLIENTS];
            
            bool _isActive;
            
            void SocketError(std::string errorMessage); // handle fatal socket errors
            void CloseClient(ClientState *client); // clean up after a connected client
    };
}

#endif
