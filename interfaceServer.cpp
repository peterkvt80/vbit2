/** A TCP server for various configuration and teletext input interfaces
    A network interface for the injection of databroadcast packets, inserter configuration, and
    page configuration, for multiple simultaneous clients.
    
    This is a binary message interface where the first byte of any message holds the message
    length, and the second byte contains either a command number (for messages to the server) or an
    error code (for server replies). A client should transmit a command and then read the server's
    response to get the error state and any returned data.
    
    The command numbers and error codes are defined in interfaceServer.h
    
    The client must select which channel subsequent commands relate to using the SETCHAN command.
    Databroadcast packet commands must be sent to any of channels 1-15 (corresponding to the packet
    addresses 1/30 to 7/31), and only a single client may select each of these datacast channels at
    one time.
    Channel 0 is used for non databroadcast commands and may have multiple simultaneous users.
    
    The server currently implements:
    * A databroadcast interface, which can encode IDL format A datacast, or inject raw packet data.
    * A configuration interface, which can set and retrieve certain vbit2 configuration options.
    * A page data API, capable of creating and deleting pages, modifying settings and row data.
*/

#include "interfaceServer.h"

using namespace vbit;

InterfaceServer::InterfaceServer(Configure *configure, Debug *debug, PageList *pageList) :
    _configure(configure),
    _debug(debug),
    _pageList(pageList),
    _portNumber(configure->GetInterfaceServerPort()),
    _maxClients(configure->GetInterfaceServerMaxClients()),
    _isActive(false)
{
    /* initialise sockets */
    _serverSock = -1;
    
    _datachannel[0]=nullptr; // do not create PacketDatacast for reserved data channel 0
    for (int dc=1; dc<16; dc++)
    {
        _datachannel[dc] = new PacketDatacast(dc, configure); // initialise remaining 15 datacast channels
    }
}

InterfaceServer::~InterfaceServer()
{
}

void InterfaceServer::SocketError(std::string errorMessage)
{
    if (_serverSock >= 0)
    {
        #ifdef WIN32
            closesocket(_serverSock);
        #else
            close(_serverSock);
        #endif
    }
    
    for(std::list<ClientState>::iterator it = _clients.begin(); it != _clients.end();)
    {
        ClientState client = *it;
        
        if (client.socket >= 0)
            CloseClient(&client);
        
        it = _clients.erase(it);
    }
    
    _debug->Log(Debug::LogLevels::logERROR,errorMessage);
}

void InterfaceServer::CloseClient(ClientState *client)
{
    // close socket
    #ifdef WIN32
        closesocket(client->socket);
    #else
        close(client->socket);
    #endif
    
    if (client->page)
    {
        client->page->FreeLock(); // unlock page
    }
}

void InterfaceServer::run()
{
    _debug->Log(Debug::LogLevels::logINFO,"[InterfaceServer::run] Datacast server thread started for "+(_maxClients?"max "+std::to_string(_maxClients):"unlimited")+" connections");
    
    int newSock;
    struct sockaddr_in address;
    char readBuffer[256];
    fd_set readfds;
    
#ifdef WIN32
    int addrlen;
    WSADATA wsaData;
    int iResult;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0)
    {
        SocketError("[InterfaceServer::run] WSAStartup failed\n");
        return;
    }
#else
    unsigned int addrlen;
#endif
    
    /* Create socket */
    if ((_serverSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        SocketError("[InterfaceServer::run] socket() failed\n");
        return;
    }
    
    int reuse = true;
    
    /* Allow multiple connnections */
    if(setsockopt(_serverSock, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse)) < 0)
    {
        SocketError("[InterfaceServer::run] setsockopt() SO_REUSEADDR failed\n");
        return;
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(_portNumber);
    
    /* bind socked */
    if (bind(_serverSock, (struct sockaddr *) &address, sizeof(address)) < 0)
    {
        SocketError("[InterfaceServer::run] bind() failed\n");
        return;
    }
    
    /* Listen for incoming connections */
    if (listen(_serverSock, MAXPENDING) < 0)
    {
        SocketError("[InterfaceServer::run] listen() failed\n");
        return;
    }
    
    addrlen = sizeof(address);
    
    while(true)
    {
        FD_ZERO(&readfds);
        FD_SET(_serverSock, &readfds);
        
        for(std::list<ClientState>::iterator it = _clients.begin(); it != _clients.end(); ++it)
        {
            if ((*it).socket > -1)
                FD_SET((*it).socket , &readfds);
        }
        _isActive = !(_clients.empty());
        
        /* wait for activity on any socket */
        if ((select(FD_SETSIZE, &readfds, NULL, NULL, NULL) < 0) && (errno!=EINTR))
            SocketError("[InterfaceServer::run] select() failed");
        
        if (FD_ISSET(_serverSock, &readfds))
        {
            /* incoming connection to server */
            if ((newSock = accept(_serverSock, (struct sockaddr *)&address, &addrlen))<0)
            {
                SocketError("[InterfaceServer::run] accept() failed");
                return;
            }
            
            #ifdef WIN32
                u_long ul = 1;
                if (ioctlsocket(newSock, FIONBIO, &ul) < 0)
                {
                    SocketError("[InterfaceServer::run] ioctlsocket() failed");
                    return;
                }
            #else
                if (fcntl(newSock, F_SETFL, fcntl(newSock, F_GETFL, 0) | O_NONBLOCK) < 0)
                {
                    SocketError("[InterfaceServer::run] fcntl() failed");
                    return;
                }
            #endif
            
            if (_maxClients > 0 && _maxClients == _clients.size())
            {
                /* no more client slots so reject */
                #ifdef WIN32
                    closesocket(newSock);
                #else
                    close(newSock);
                #endif
                _debug->Log(Debug::LogLevels::logWARN,"[InterfaceServer::run] reject new connection from " + std::string(inet_ntoa(address.sin_addr)) + " (too many connections)");
                continue;
            }
            
            ClientState newClient;
            newClient.socket = newSock;
            _clients.push_back(newClient);
            
            _debug->Log(Debug::LogLevels::logINFO,"[InterfaceServer::run] new connection from " + std::string(inet_ntoa(address.sin_addr)) + ":" + std::to_string(ntohs(address.sin_port)) + " as socket " + std::to_string(newSock));
        }
        else
        {
            /* a client socket has activity */
            for(std::list<ClientState>::iterator it = _clients.begin(); it != _clients.end();)
            {
                ClientState* client = &(*it);
                
                if (client->socket >= 0 && FD_ISSET(client->socket , &readfds))
                {
                    /* socket has activity */
                    getpeername(client->socket, (struct sockaddr*)&address, &addrlen);
                    
                    int n = recv(client->socket, readBuffer, 1, MSG_PEEK); // peek at first byte of message
                    if (n == 1)
                    {
                        // byte 0 of message is message length
                        int len = (uint8_t)readBuffer[0];
                        if (len)
                        {
                            n = recv(client->socket, readBuffer, len, 0); // try to read whole message
                            if (n == len)
                            {
                                std::vector<uint8_t> res;
                                res.push_back(CMDOK); // create "OK" response
                                
                                // byte 1 of message is interface server command number
                                switch ((uint8_t)readBuffer[1]){
                                    case SETCHAN: // set interface channel
                                    {
                                        int ch = (uint8_t)readBuffer[2];
                                        if (n == 3 && ch >= 0 && ch <= 15)
                                        {
                                            client->channel = -1; // release current channel
                                            
                                            
                                            if (ch) // allows multiple connections for channel 0
                                            {
                                                // check if another client is using desired datachannel
                                                for(std::list<ClientState>::iterator it = _clients.begin(); it != _clients.end(); ++it)
                                                {
                                                    if ((*it).channel == ch)
                                                    {
                                                        res[0] = CMDBUSY;
                                                        goto SETCHANError; // jump out to return error
                                                    }
                                                }
                                            }
                                            
                                            client->channel = ch; // use requested channel
                                            
                                            std::stringstream ss;
                                            ss << "[InterfaceServer::run] Client " << std::string(inet_ntoa(address.sin_addr)) << ":" << std::to_string(ntohs(address.sin_port)) << ": SETCHAN " << ch;
                                            _debug->Log(Debug::LogLevels::logDEBUG,ss.str());
                                            break;
                                        }
                                        
                                        res[0] = CMDERR;
                                        SETCHANError:
                                        break;
                                    }
                                    
                                    case GETAPIVER:
                                    {
                                        /* get API version number */
                                        if (n == 2)
                                        {
                                            res.push_back(APIVERSION[0]); // major version
                                            res.push_back(APIVERSION[1]); // minor version
                                            res.push_back(APIVERSION[2]); // patch
                                        }
                                        else
                                            res[0] = CMDERR;
                                        break;
                                    }
                                    
                                    case DBCASTAPI:
                                    {
                                        /* databroadcast API */
                                        if (client->channel > 0 && n > 2) // databroadcast commands only on channels 1-15
                                        {
                                            switch((uint8_t)readBuffer[2]) // byte 2 is databroadcast command number
                                            {
                                                case DCRAW: // push raw datacast packet to buffer
                                                {
                                                    if (n == 43) // 40 bytes of packet data
                                                    {
                                                        std::vector<uint8_t> data(readBuffer+3, readBuffer+n);
                                                        
                                                        if(_datachannel[client->channel]->PushRaw(&data))
                                                        {
                                                            res[0] = CMDBUSY; // buffer full
                                                        }
                                                    }
                                                    else
                                                    {
                                                        res[0] = CMDERR;
                                                    }
                                                    break;
                                                }
                                                
                                                case DCFORMATA: // encode and push a format A datacast payload to buffer
                                                {
                                                    /* Does _not_ automate repeats, continuity indicator, etc.
                                                       Format is:
                                                         byte 3: bits 4-7 IAL, bits 1-3 flags RI,CI,DL
                                                         byte 4-6: 24 bit Service Packet Address (little endian)
                                                         byte 7: Repeat indicator
                                                         byte 8: Continuity indicator
                                                         byte 9+: payload data
                                                    */
                                                    if (n > 9)
                                                    {
                                                        uint8_t flags = (uint8_t)readBuffer[3] & 0xe;
                                                        uint8_t ial = (uint8_t)readBuffer[3] >> 4;
                                                        uint32_t spa = (uint8_t)readBuffer[4] | ((uint8_t)readBuffer[5] << 8) | ((uint8_t)readBuffer[6] << 16);
                                                        uint8_t ri = (uint8_t)readBuffer[7];
                                                        uint8_t ci = (uint8_t)readBuffer[8];
                                                        
                                                        std::vector<uint8_t> data(readBuffer+9, readBuffer+n);
                                                        
                                                        int bytes = _datachannel[client->channel]->PushIDLA(flags, ial, spa, ri, ci, &data);
                                                        
                                                        if (bytes == 0) // buffer full
                                                            res[0] = CMDBUSY;
                                                        else if (bytes < n-9) // payload didn't fit
                                                            res[0] = CMDTRUNC; // warn of truncation
                                                        
                                                        res.push_back(bytes); // return number of bytes written
                                                        
                                                        break;
                                                    }
                                                    else
                                                    {
                                                        res[0] = CMDERR;
                                                        res.push_back(0); // no bytes written
                                                    }
                                                    break;
                                                }
                                                
                                                default: // unknown datacast command
                                                    res[0] = CMDERR;
                                            }
                                        }
                                        else
                                        {
                                            res[0] = CMDERR;
                                        }
                                        break;
                                    }
                                    
                                    case CONFIGAPI:
                                    {
                                        /* vbit2 configuration API */
                                        if (client->channel == 0 && n > 2) // allow VBIT2 configuration commands on channel 0 only
                                        {
                                            switch((uint8_t)readBuffer[2]){ // byte 2 is configuration command number
                                                case CONFRAFLAG: /* get/set row adaptive flag */
                                                {
                                                    if (n == 4)
                                                    {
                                                        _configure->SetRowAdaptive((uint8_t)readBuffer[3]&1);
                                                        std::stringstream ss;
                                                        ss << "[InterfaceServer::run] Client " << std::string(inet_ntoa(address.sin_addr)) << ":" << std::to_string(ntohs(address.sin_port)) << ": CONFRAFLAG " << ((readBuffer[3] & 1)?"ON":"OFF");
                                                        _debug->Log(Debug::LogLevels::logDEBUG,ss.str());
                                                    }
                                                    else if (n != 3)
                                                    {
                                                        res[0] = CMDERR;
                                                    }
                                                    res.push_back(_configure->GetRowAdaptive()?1:0);
                                                    
                                                    break;
                                                }
                                                
                                                case CONFRBYTES: /* get/set BSDP reserved bytes */
                                                {
                                                    if (n == 7) // set new bytes
                                                    {
                                                        _configure->SetReservedBytes(std::array<uint8_t, 4>({(uint8_t)readBuffer[3],(uint8_t)readBuffer[4],(uint8_t)readBuffer[5],(uint8_t)readBuffer[6]}));
                                                        std::stringstream ss;
                                                        ss << "[InterfaceServer::run] Client " << std::string(inet_ntoa(address.sin_addr)) << ":" << std::to_string(ntohs(address.sin_port)) << ": CONFRBYTES set";
                                                        _debug->Log(Debug::LogLevels::logDEBUG,ss.str());
                                                    }
                                                    else if (n != 3)
                                                    {
                                                        res[0] = CMDERR;
                                                    }
                                                    
                                                    std::array<uint8_t, 4> bytes = _configure->GetReservedBytes();
                                                    res.push_back(bytes[0]);
                                                    res.push_back(bytes[1]);
                                                    res.push_back(bytes[2]);
                                                    res.push_back(bytes[3]); // read back reserved bytes
                                                    break;
                                                }
                                                
                                                case CONFSTATUS: /* get/set BSDP status message */
                                                {
                                                    if (n == 23) // set new status
                                                    {
                                                        std::ostringstream tmp;
                                                        for (int i = 3; i < 23; i++)
                                                        {
                                                            tmp << (char)(readBuffer[i] & 0x7f);
                                                        }
                                                        
                                                        _configure->SetServiceStatusString(tmp.str());
                                                        std::stringstream ss;
                                                        ss << "[InterfaceServer::run] Client " << std::string(inet_ntoa(address.sin_addr)) << ":" << std::to_string(ntohs(address.sin_port)) << ": CONFSTATUS set";
                                                        _debug->Log(Debug::LogLevels::logDEBUG,ss.str());
                                                    }
                                                    else if (n != 3)
                                                    {
                                                        res[0] = CMDERR;
                                                    }
                                                    
                                                    for(char& c : _configure->GetServiceStatusString()) {
                                                        res.push_back((uint8_t)c & 0x7f);
                                                    }
                                                    
                                                    break;
                                                }
                                                
                                                case CONFHEADER: /* get/set header template */
                                                {
                                                    if (n == 35 || n == 36) // set new header template
                                                    {
                                                        std::array<uint8_t, 40> tmp;
                                                        for (int i = 0; i < 32; i++)
                                                            tmp[8+i] = readBuffer[(n-32)+i];
                                                        std::shared_ptr<TTXLine> line(new TTXLine(tmp));
                                                        
                                                        std::stringstream ss;
                                                        if (n==35)
                                                        {
                                                            _configure->SetHeaderTemplate(line);
                                                            ss << "[InterfaceServer::run] Client " << std::string(inet_ntoa(address.sin_addr)) << ":" << std::to_string(ntohs(address.sin_port)) << ": CONFHEADER set";
                                                        }
                                                        else
                                                        {
                                                            uint8_t mag = (uint8_t)readBuffer[3] & 0x7;
                                                            _pageList->GetMagazines()[mag]->SetCustomHeader(line);
                                                            ss << "[InterfaceServer::run] Client " << std::string(inet_ntoa(address.sin_addr)) << ":" << std::to_string(ntohs(address.sin_port)) << ": CONFHEADER set on magazine " << (int)mag;
                                                        }
                                                        _debug->Log(Debug::LogLevels::logDEBUG,ss.str());
                                                    }
                                                    else if (n == 3)
                                                    {
                                                        for(char& c : _configure->GetHeaderTemplate()) {
                                                            res.push_back((uint8_t)c);
                                                        }
                                                    }
                                                    else if (n == 4)
                                                    {
                                                        uint8_t mag = (uint8_t)readBuffer[3];
                                                        if (mag & 0x80)
                                                        {
                                                            // delete flag
                                                            _pageList->GetMagazines()[mag & 0x7]->DeleteCustomHeader();
                                                        }
                                                        else
                                                        {
                                                            std::string tmp = _pageList->GetMagazines()[mag & 0x7]->GetCustomHeader();
                                                            if (tmp.length() == 32)
                                                            {
                                                                for(char& c : tmp) {
                                                                    res.push_back((uint8_t)c);
                                                                }
                                                            }
                                                            else
                                                            {
                                                                // no custom header set for magazine
                                                                res[0] = CMDNOENT;
                                                            }
                                                        }
                                                    }
                                                    else
                                                    {
                                                        res[0] = CMDERR;
                                                    }
                                                    
                                                    break;
                                                }
                                                
                                                case CONFENHANC:
                                                {
                                                    res[0] = CMDERR; // default to an returning an error unless we have success
                                                    if (n > 3)
                                                    {
                                                        int mag = (uint8_t)readBuffer[3] & 0x1F;
                                                        int deleteFlag = (uint8_t)readBuffer[3]&0x80;
                                                        if (mag < 8)
                                                        {
                                                            if (!deleteFlag)
                                                            {
                                                                if (n == 44)
                                                                {
                                                                    // write row data
                                                                    std::array<uint8_t, 40> tmp;
                                                                    for (int i = 0; i < 40; i++)
                                                                        tmp[i] = (uint8_t)readBuffer[4+i];
                                                                    std::shared_ptr<TTXLine> line(new TTXLine(tmp));
                                                                    
                                                                    _pageList->GetMagazines()[mag]->SetPacket29(line);
                                                                    
                                                                    res[0] = CMDOK;
                                                                }
                                                                else if (n == 5)
                                                                {
                                                                    // read row data
                                                                    std::shared_ptr<TTXLine> line = _pageList->GetMagazines()[mag]->GetPacket29();
                                                                    if (line!=nullptr)
                                                                        line = line->LocateLine((uint8_t)readBuffer[4]&0xF);
                                                                    
                                                                    if (line != nullptr)
                                                                    {
                                                                        std::array<uint8_t, 40> tmp = line->GetLine();
                                                                        res.insert (res.end(), tmp.data(), tmp.data()+tmp.size());
                                                                        res[0] = CMDOK;
                                                                    }
                                                                    else
                                                                        res[0] = CMDNOENT; // row doesn't exist
                                                                }
                                                            }
                                                            else
                                                            {
                                                                if (n==4)
                                                                {
                                                                    // delete all packets
                                                                    _pageList->GetMagazines()[mag]->DeletePacket29();
                                                                }
                                                                else if (n==5)
                                                                {
                                                                    // delete dc
                                                                    _pageList->GetMagazines()[mag]->DeletePacket29((uint8_t)readBuffer[4]&0xF);
                                                                }
                                                                res[0] = CMDOK; // didn't check if line existed, just returns OK
                                                            }
                                                        }
                                                    }
                                                    break;
                                                }
                                                
                                                default: // unknown configuration command
                                                    res[0] = CMDERR;
                                            }
                                        }
                                        else
                                        {
                                            res[0] = CMDERR;
                                        }
                                        break;
                                    }
                                    
                                    case PAGESAPI:
                                    {
                                        /* page data API */
                                        
                                        if (client->channel == 0 && n > 2) // allow page data commands on channel 0 only
                                        {
                                            int cmd = (uint8_t)readBuffer[2]; // byte 2 is page data API command number
                                            
                                            if (cmd <= PAGEDELSUB) // all commands that start with a page/subpage number
                                            {
                                                if (cmd <= PAGEOPEN && client->page)
                                                {
                                                    // implicitly close page when issuing other page delete/open commands
                                                    client->page->FreeLock();
                                                    client->page = nullptr;
                                                    client->subpage = nullptr;
                                                }
                                                
                                                if (n >= 5)
                                                {
                                                    int num = ((uint8_t)readBuffer[3] << 8) | (uint8_t)readBuffer[4];
                                                    if (cmd == PAGEDELETE)
                                                    {
                                                        if (n == 5)
                                                        {
                                                            std::stringstream ss;
                                                            ss << "[InterfaceServer::run] Client " << std::string(inet_ntoa(address.sin_addr)) << ":" << std::to_string(ntohs(address.sin_port)) << ": PAGEDELETE " << std::hex << num;
                                                            _debug->Log(Debug::LogLevels::logDEBUG,ss.str());
                                                            
                                                            std::shared_ptr<TTXPageStream> p = _pageList->Locate(num);
                                                            if (p != nullptr)
                                                            {
                                                                p->SetOneShotFlag(false);
                                                                p->MarkForDeletion();
                                                            }
                                                            else
                                                            {
                                                                res[0] = CMDNOENT;
                                                            }
                                                        }
                                                        else
                                                            res[0] = CMDERR;
                                                    }
                                                    else if (cmd == PAGEOPEN)
                                                    {
                                                        bool OneShot = false;
                                                        if (n > 5)
                                                            OneShot = (readBuffer[5] & 1);
                                                        
                                                        if (n < 7)
                                                        {
                                                            std::stringstream ss;
                                                            ss << "[InterfaceServer::run] Client " << std::string(inet_ntoa(address.sin_addr)) << ":" << std::to_string(ntohs(address.sin_port)) << ": PAGEOPEN " << std::hex << num << (OneShot?" as OneShot":"");
                                                            _debug->Log(Debug::LogLevels::logDEBUG,ss.str());
                                                            if ((uint8_t)readBuffer[3] > 0 && (uint8_t)readBuffer[3] <= 8 && (uint8_t)readBuffer[4] < 0xff)
                                                            {
                                                                std::shared_ptr<TTXPageStream> p = _pageList->Locate(num);
                                                                if (p == nullptr || p->GetIsMarked())
                                                                {
                                                                    p = std::shared_ptr<TTXPageStream>(new TTXPageStream()); // create new page
                                                                    std::stringstream ss;
                                                                    ss << "[InterfaceServer::run] Created new page " << std::hex << num;
                                                                    _debug->Log(Debug::LogLevels::logINFO,ss.str());
                                                                    if (p->GetLock()) // if this fails we have a real problem!
                                                                    {
                                                                        p->SetPageNumber(num);
                                                                        p->SetOneShotFlag(OneShot);
                                                                        _pageList->AddPage(p, true); // put it in the page lists
                                                                        
                                                                        // at this stage it has no subpages!
                                                                        client->page = p;
                                                                    }
                                                                    else
                                                                    {
                                                                        res[0] = CMDBUSY;
                                                                    }
                                                                }
                                                                else
                                                                {
                                                                    res[0] = CMDBUSY; // overwritten if successful
                                                                    if (p->GetOneShotFlag() && p->GetUpdatedFlag())
                                                                    {
                                                                        // previous oneshot hasn't yet sent
                                                                    }
                                                                    else if (p->GetLock()) // try to lock page
                                                                    {
                                                                        if (OneShot || (p->GetOneShotFlag() != OneShot)) // oneshot or oneshot changed
                                                                        {
                                                                            p->SetOneShotFlag(OneShot);
                                                                            _pageList->UpdatePageLists(p);
                                                                        }
                                                                        
                                                                        client->page = p;
                                                                        res[0] = CMDOK;
                                                                    }
                                                                }
                                                            }
                                                        }
                                                        else
                                                            res[0] = CMDERR;
                                                    }
                                                    else if (cmd == PAGESETSUB || cmd == PAGEDELSUB)
                                                    {
                                                        client->subpage = nullptr; // invalidate previous subpage
                                                        if (n == 5 && client->page)
                                                        {
                                                            std::stringstream ss;
                                                            ss << "[InterfaceServer::run] Client " << std::string(inet_ntoa(address.sin_addr)) << ":" << std::to_string(ntohs(address.sin_port)) << ": " << ((cmd==PAGESETSUB)?"PAGESETSUB ":"PAGEDELSUB ") << std::hex << num;
                                                            _debug->Log(Debug::LogLevels::logDEBUG,ss.str());
                                                            
                                                            if ((num & 0xc080) || num >= 0x3f7f) // reject invalid subpage numbers
                                                            {
                                                                res[0] = CMDERR;
                                                            }
                                                            else
                                                            {
                                                                client->subpage = client->page->LocateSubpage(num);
                                                                if (client->subpage == nullptr) // subpage not found
                                                                {
                                                                    if (cmd == PAGESETSUB)
                                                                    {
                                                                        client->subpage = std::shared_ptr<Subpage>(new Subpage()); // create new subpage
                                                                        client->subpage->SetSubCode(num); // set subcode first
                                                                        client->page->InsertSubpage(client->subpage); // add to page
                                                                        _pageList->UpdatePageLists(client->page);
                                                                        client->subpage->SetSubpageStatus(PAGESTATUS_TRANSMITPAGE);
                                                                        
                                                                        if (client->page->GetOneShotFlag()) // page is a oneshot
                                                                            client->page->SetSubpage(num); // put this subpage on air
                                                                    }
                                                                    else // PAGEDELSUB
                                                                    {
                                                                        res[0] = CMDNOENT;
                                                                    }
                                                                }
                                                                else
                                                                {
                                                                    if (cmd == PAGESETSUB)
                                                                    {
                                                                        if (client->page->GetOneShotFlag()) // page is a oneshot
                                                                            client->page->SetSubpage(num); // put this subpage on air
                                                                    }
                                                                    else // PAGEDELSUB
                                                                    {
                                                                        client->page->RemoveSubpage(client->subpage);
                                                                    }
                                                                }
                                                                unsigned int count = client->page->GetSubpageCount();
                                                                res.push_back((count >> 8) & 0xff);
                                                                res.push_back(count & 0xff); // return subpage count (big endian)
                                                            }
                                                        }
                                                        else
                                                            res[0] = CMDERR;
                                                    }
                                                }
                                                else
                                                    res[0] = CMDERR;
                                            }
                                            else if (cmd == PAGECLOSE)
                                            {
                                                std::stringstream ss;
                                                ss << "[InterfaceServer::run] Client " << std::string(inet_ntoa(address.sin_addr)) << ":" << std::to_string(ntohs(address.sin_port)) << ": PAGECLOSE";
                                                _debug->Log(Debug::LogLevels::logDEBUG,ss.str());
                                                if (n==3)
                                                {
                                                    if (client->page)
                                                        client->page->FreeLock();
                                                    else
                                                        res[0] = CMDNOENT;
                                                    client->page = nullptr;
                                                    client->subpage = nullptr;
                                                    
                                                    
                                                }
                                                else
                                                {
                                                    res[0] = CMDERR;
                                                }
                                            }
                                            else if (cmd == PAGEFANDC)
                                            {
                                                if (client->page)
                                                {
                                                    std::stringstream ss;
                                                    ss << "[InterfaceServer::run] Client " << std::string(inet_ntoa(address.sin_addr)) << ":" << std::to_string(ntohs(address.sin_port)) << ": PAGEFANDC";
                                                    _debug->Log(Debug::LogLevels::logDEBUG,ss.str());
                                                    if (n == 5) // write
                                                    {
                                                        client->page->SetPageFunctionInt((uint8_t)readBuffer[3]);
                                                        client->page->SetPageCodingInt((uint8_t)readBuffer[4]);
                                                        _pageList->UpdatePageLists(client->page);
                                                    }
                                                    else if (n != 3)
                                                    {
                                                        res[0] = CMDERR;
                                                    }
                                                    res.push_back(client->page->GetPageFunction());
                                                    res.push_back(client->page->GetPageCoding());
                                                }
                                                else
                                                {
                                                    res[0] = CMDERR;
                                                }
                                            }
                                            else if (cmd == PAGEOPTNS)
                                            {
                                                if (client->subpage)
                                                {
                                                    std::stringstream ss;
                                                    ss << "[InterfaceServer::run] Client " << std::string(inet_ntoa(address.sin_addr)) << ":" << std::to_string(ntohs(address.sin_port)) << ": PAGEOPTNS";
                                                    _debug->Log(Debug::LogLevels::logDEBUG,ss.str());
                                                    
                                                    if (n == 8) // write
                                                    {
                                                        client->subpage->SetSubpageStatus(((uint8_t)readBuffer[3] << 8) | (uint8_t)readBuffer[4]);
                                                        client->subpage->SetRegion((uint8_t)readBuffer[5]);
                                                        client->subpage->SetCycleTime((uint8_t)readBuffer[6]);
                                                        client->subpage->SetTimedMode((uint8_t)readBuffer[7] & 1);
                                                    }
                                                    else if (n != 3)
                                                    {
                                                        res[0] = CMDERR;
                                                    }
                                                    uint16_t status = client->subpage->GetSubpageStatus();
                                                    res.push_back((status >> 8) & 0xff);
                                                    res.push_back(status & 0xff);
                                                    res.push_back(client->subpage->GetRegion());
                                                    res.push_back(client->subpage->GetCycleTime());
                                                    res.push_back(client->subpage->GetTimedMode()?1:0);
                                                }
                                                else
                                                {
                                                    res[0] = CMDERR;
                                                }
                                            }
                                            else if (cmd == PAGEROW)
                                            {
                                                res[0] = CMDERR; // default to an returning an error unless we have success
                                                if (client->subpage)
                                                {
                                                    std::stringstream ss;
                                                    ss << "[InterfaceServer::run] Client " << std::string(inet_ntoa(address.sin_addr)) << ":" << std::to_string(ntohs(address.sin_port)) << ": PAGEROW";
                                                    _debug->Log(Debug::LogLevels::logDEBUG,ss.str());
                                                    
                                                    if (n > 3)
                                                    {
                                                        int num = (uint8_t)readBuffer[3] & 0x1F;
                                                        int deleteFlag = (uint8_t)readBuffer[3]&0x80;
                                                        if (num > 0 && num < 29)
                                                        {
                                                            if (!deleteFlag)
                                                            {
                                                                if (n == 44)
                                                                {
                                                                    // write row data
                                                                    std::array<uint8_t, 40> tmp;
                                                                    for (int i = 0; i < 40; i++)
                                                                        tmp[i] = (uint8_t)readBuffer[4+i];
                                                                    std::shared_ptr<TTXLine> line(new TTXLine(tmp));
                                                                    
                                                                    client->subpage->SetRow(num, line);
                                                                    
                                                                    res[0] = CMDOK;
                                                                }
                                                                else if ((num < 26 && n==4) || (num > 25 && n==5))
                                                                {
                                                                    // read row data
                                                                    std::shared_ptr<TTXLine> line = client->subpage->GetRow(num);
                                                                    if (n==5 && line!=nullptr)
                                                                        line = line->LocateLine((uint8_t)readBuffer[4]&0xF);
                                                                    
                                                                    if (line != nullptr)
                                                                    {
                                                                        std::array<uint8_t, 40> tmp = line->GetLine();
                                                                        res.insert (res.end(), tmp.data(), tmp.data()+tmp.size());
                                                                        res[0] = CMDOK;
                                                                    }
                                                                    else
                                                                        res[0] = CMDNOENT; // row doesn't exist
                                                                }
                                                            }
                                                            else
                                                            {
                                                                if (n==4)
                                                                {
                                                                    // delete row data
                                                                    client->subpage->DeleteRow(num);
                                                                }
                                                                else if (num > 25 && n==5)
                                                                {
                                                                    // delete dc
                                                                    client->subpage->DeleteRow(num, (uint8_t)readBuffer[4]&0xF);
                                                                }
                                                                res[0] = CMDOK; // didn't check if line existed, just returns OK
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                            else if (cmd == PAGELINKS)
                                            {
                                                if (client->subpage)
                                                {
                                                    std::stringstream ss;
                                                    ss << "[InterfaceServer::run] Client " << std::string(inet_ntoa(address.sin_addr)) << ":" << std::to_string(ntohs(address.sin_port)) << ": PAGELINKS";
                                                    _debug->Log(Debug::LogLevels::logDEBUG,ss.str());
                                                    
                                                    std::array<FastextLink, 6> links;
                                                    
                                                    if (n == 15 || n == 27)
                                                    {
                                                        for (int l=0; l<6; l++)
                                                        {
                                                            links[l].page = (((uint8_t)readBuffer[3+(l*2)] & 0x7) << 8) | (uint8_t)readBuffer[4+(l*2)];
                                                            
                                                            if (n == 27)
                                                            {
                                                                links[l].subpage = (((uint8_t)readBuffer[15+(l*2)] << 8) | (uint8_t)readBuffer[16+(l*2)]) & 0x3f7f;
                                                            }
                                                            else
                                                            {
                                                                links[l].subpage = 0x3f7f;
                                                            }
                                                        }
                                                        client->subpage->SetFastext(links);
                                                    }
                                                    else if (n != 3)
                                                    {
                                                        res[0] = CMDERR;
                                                    }
                                                    
                                                    if (res[0] == CMDOK)
                                                    {
                                                        if (client->subpage->GetFastext(&links))
                                                        {
                                                            for (int l = 0; l < 6; l++)
                                                            {
                                                                res.push_back((links[l].page >> 8) & 7);
                                                                res.push_back(links[l].page & 0xff);
                                                            }
                                                            for (int l = 0; l < 6; l++)
                                                            {
                                                                res.push_back((links[l].subpage >> 8) & 0x3f);
                                                                res.push_back(links[l].subpage & 0x7f);
                                                            }
                                                        }
                                                        else
                                                        {
                                                            res[0] = CMDNOENT;
                                                        }
                                                    }
                                                }
                                                else
                                                {
                                                    res[0] = CMDERR;
                                                }
                                            }
                                            else if (cmd > PAGELINKS) // last defined command number
                                            {
                                                std::stringstream ss;
                                                ss << "[InterfaceServer::run] Client " << std::string(inet_ntoa(address.sin_addr)) << ":" << std::to_string(ntohs(address.sin_port)) << ": Unknown PAGESAPI command received " << std::hex << cmd;
                                                _debug->Log(Debug::LogLevels::logDEBUG,ss.str());
                                            }
                                        }
                                        break;
                                    }
                                    
                                    default: // unknown command
                                    {
                                        res[0] = CMDERR;
                                        break;
                                    }
                                }
                                
                                if (res.size() > 254)
                                {
                                    _debug->Log(Debug::LogLevels::logERROR,"[InterfaceServer::run] Response too long");
                                    res.resize(254); // truncate!
                                }
                                
                                res.insert(res.begin(), res.size()+1); // prepend message size
                                
                                unsigned int n = send(client->socket, (char*)res.data(), res.size(), 0); // send response
                                
                                if (n == res.size()) // fail if only partial response can be sent
                                {
                                    ++it;
                                    continue; // next socket in loop
                                }
                            }
                            // else fall through to error handling
                        }
                    }
                    // couldn't read/write all bytes
                    if (n == 0)
                    {
                        /* client disconnected */
                        _debug->Log(Debug::LogLevels::logINFO,"[InterfaceServer::run] closing connection from " + std::string(inet_ntoa(address.sin_addr)) + ":" + std::to_string(ntohs(address.sin_port)) + " on socket " + std::to_string(client->socket));
                    }
                    else
                    {
                        #ifdef WIN32
                            int e = WSAGetLastError();
                        #else
                            int e = errno;
                        #endif
                        
                        _debug->Log(Debug::LogLevels::logWARN,"[InterfaceServer::run] closing connection from " + std::string(inet_ntoa(address.sin_addr)) + ":" + std::to_string(ntohs(address.sin_port)) + " recv error " + std::to_string(e) + " on socket " + std::to_string(client->socket));
                    }
                    
                    /* close the socket when any error occurs */
                    CloseClient(client);
                    it = _clients.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }
    }
}
