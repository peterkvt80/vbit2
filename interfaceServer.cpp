/** A TCP server for various configuration and teletext input interfaces
    A network interface for the injection of databroadcast packets, which is extended to provide
    configuration and page data APIs for multiple simultaneous clients.
    
    This is a binary message interface where the first byte of any message holds the message
    length, and the second byte contains either a command number (for messages to the server) or an
    error code (for server replies). A client should transmit a command and then read the server's
    response to get the error state and any returned data.
    
    The command numbers and error codes are defined in interfaceServer.h
    
    The client must select which channel subsequent commands relate to using the DCSET command.
    Databroadcast packet commands must be sent to any of channels 1-15, and only a single client
    may select each of these datacast channels at one time.
    Channel 0 is used for non databroadcast commands. It currently implements a configuration
    interface, which can set and retrieve certain vbit2 configuration options.
*/

#include "interfaceServer.h"

using namespace vbit;

InterfaceServer::InterfaceServer(Configure *configure, Debug *debug, PageList *pageList) :
    _configure(configure),
    _debug(debug),
    _pageList(pageList),
    _portNumber(configure->GetInterfaceServerPort()),
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
    
    for (int i = 0; i < MAXCLIENTS; i++)
    {
        if (_clientState[i].socket >= 0)
            CloseClient(&_clientState[i]);
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
        client->page = nullptr;
    }
    
    client->channel = -1;
    client->subpage = nullptr;
    client->socket = -1; // mark this slot free for new connections
}

void InterfaceServer::run()
{
    _debug->Log(Debug::LogLevels::logDEBUG,"[InterfaceServer::run] Datacast server thread started");
    
    int newSock;
    int sock;
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
        
        bool active = false;
        for (int i = 0; i < MAXCLIENTS; i++)
        {
            sock = _clientState[i].socket;
            
            if(sock >= 0){
                FD_SET(sock , &readfds);
                active = true; /* packet server has connections */
            }
        }
        _isActive = active;
        
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
            
            for (int i = 0; i <= MAXCLIENTS; i++)
            {
                if (i == MAXCLIENTS)
                {
                    /* no more client slots so reject */
                    #ifdef WIN32
                        closesocket(newSock);
                    #else
                        close(newSock);
                    #endif
                    _debug->Log(Debug::LogLevels::logWARN,"[InterfaceServer::run] reject new connection from " + std::string(inet_ntoa(address.sin_addr)) + " (too many connections)");
                    break;
                }
                
                /* find unused slot */
                if( _clientState[i].socket < 0 )
                {
                    /* add to active sockets */
                    _clientState[i].socket = newSock;
                    _clientState[i].channel = -1; // no channel set
                    _debug->Log(Debug::LogLevels::logINFO,"[InterfaceServer::run] new connection from " + std::string(inet_ntoa(address.sin_addr)) + ":" + std::to_string(ntohs(address.sin_port)) + " as socket " + std::to_string(newSock));
                    break;
                }
            }
        }
        else
        {
            /* a client socket has activity */
            for (int i = 0; i < MAXCLIENTS; i++)
            {
                sock = _clientState[i].socket;
                
                if (sock >= 0 && FD_ISSET(sock , &readfds))
                {
                    /* socket has activity */
                    
                    int n = recv(sock, readBuffer, 1, MSG_PEEK); // peek at first byte of message
                    if (n == 1)
                    {
                        // byte 0 of message is message length
                        int len = (uint8_t)readBuffer[0];
                        if (len)
                        {
                            n = recv(sock, readBuffer, len, 0); // try to read whole message
                            if (n == len)
                            {
                                std::vector<uint8_t> res = {CMDOK}; // create "OK" response
                                
                                // byte 1 of message is interface server command number
                                switch ((uint8_t)readBuffer[1]){
                                    case SETCHAN: // set interface channel
                                    {
                                        int ch = (uint8_t)readBuffer[2];
                                        if (n == 3 && ch >= 0 && ch <= 15)
                                        {
                                            _clientState[i].channel = -1; // release current channel
                                            
                                            
                                            if (ch) // allows multiple connections for channel 0
                                            {
                                                // check if another client is using desired datachannel
                                                for (int j=0; j<MAXCLIENTS; j++){
                                                    if (_clientState[j].channel == ch)
                                                    {
                                                        res[0] = CMDBUSY;
                                                        goto SETCHANError; // jump out to return error
                                                    }
                                                }
                                            }
                                            
                                            _clientState[i].channel = ch; // use requested channel
                                            
                                            std::stringstream ss;
                                            ss << "[InterfaceServer::run] Client " << i << ": SETCHAN " << ch;
                                            _debug->Log(Debug::LogLevels::logDEBUG,ss.str());
                                            break;
                                        }
                                        
                                        res[0] = CMDERR;
                                        SETCHANError:
                                        break;
                                    }
                                    
                                    case DBCASTAPI:
                                    {
                                        /* databroadcast API */
                                        if (_clientState[i].channel > 0 && n > 2) // databroadcast commands only on channels 1-15
                                        {
                                            switch((uint8_t)readBuffer[2]) // byte 2 is databroadcast command number
                                            {
                                                case DCRAW: // push raw datacast packet to buffer
                                                {
                                                    if (n == 43) // 40 bytes of packet data
                                                    {
                                                        std::vector<uint8_t> data(readBuffer+3, readBuffer+n);
                                                        
                                                        if(_datachannel[_clientState[i].channel]->PushRaw(&data))
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
                                                        
                                                        int bytes = _datachannel[_clientState[i].channel]->PushIDLA(flags, ial, spa, ri, ci, &data);
                                                        
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
                                        if (_clientState[i].channel == 0 && n > 2) // allow VBIT2 configuration commands on channel 0 only
                                        {
                                            switch((uint8_t)readBuffer[2]){ // byte 2 is configuration command number
                                                case CONFRAFLAG: /* get/set row adaptive flag */
                                                {
                                                    if (n == 4)
                                                    {
                                                        _configure->SetRowAdaptive(((uint8_t)readBuffer[3]&1)?true:false);
                                                        std::stringstream ss;
                                                        ss << "[InterfaceServer::run] Client " << i << ": CONFRAFLAG " << ((readBuffer[3] & 1)?"ON":"OFF");
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
                                                        ss << "[InterfaceServer::run] Client " << i << ": CONFRBYTES set";
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
                                                        ss << "[InterfaceServer::run] Client " << i << ": CONFSTATUS set";
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
                                                    if (n == 35) // set new header template
                                                    {
                                                        std::array<uint8_t, 40> tmp;
                                                        for (int i = 0; i < 32; i++)
                                                            tmp[8+i] = readBuffer[3+i];
                                                        std::shared_ptr<TTXLine> line(new TTXLine(tmp));
                                                        _configure->SetHeaderTemplate(line);
                                                        std::stringstream ss;
                                                        ss << "[InterfaceServer::run] Client " << i << ": CONFHEADER set";
                                                        _debug->Log(Debug::LogLevels::logDEBUG,ss.str());
                                                    }
                                                    else if (n != 3)
                                                    {
                                                        res[0] = CMDERR;
                                                    }
                                                    
                                                    for(char& c : _configure->GetHeaderTemplate()) {
                                                        res.push_back((uint8_t)c);
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
                                        
                                        if (_clientState[i].channel == 0 && n > 2) // allow page data commands on channel 0 only
                                        {
                                            int cmd = (uint8_t)readBuffer[2]; // byte 2 is page data API command number
                                            
                                            if (cmd <= PAGEDELSUB && n >= 5) // all commands that start with a page/subpage number
                                            {
                                                int num = ((uint8_t)readBuffer[3] << 8) | (uint8_t)readBuffer[4];
                                                if (cmd <= PAGEOPEN && _clientState[i].page)
                                                {
                                                    // implicitly close page when issuing other page delete/open commands
                                                    _clientState[i].page->FreeLock();
                                                    _clientState[i].page = nullptr;
                                                    _clientState[i].subpage = nullptr;
                                                }
                                                
                                                if (cmd == PAGEDELETE)
                                                {
                                                    if (n == 5)
                                                    {
                                                        std::stringstream ss;
                                                        ss << "[InterfaceServer::run] Client " << i << ": PAGEDELETE " << std::hex << num;
                                                        _debug->Log(Debug::LogLevels::logDEBUG,ss.str());
                                                        
                                                        std::shared_ptr<TTXPageStream> p = _pageList->Locate(num);
                                                        if (p != nullptr)
                                                        {
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
                                                        OneShot = (readBuffer[5] == 1);
                                                    
                                                    if (n < 7)
                                                    {
                                                        std::stringstream ss;
                                                        ss << "[InterfaceServer::run] Client " << i << ": PAGEOPEN " << std::hex << num << (OneShot?" as OneShot":"");
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
                                                                    _clientState[i].page = p;
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
                                                                        if (!OneShot)
                                                                        {
                                                                            // ensure page gets re-added to lists
                                                                            p->SetNormalFlag(false);
                                                                            p->SetSpecialFlag(false);
                                                                            p->SetCarouselFlag(false);
                                                                        }
                                                                        p->SetUpdatedFlag(false);
                                                                        _pageList->UpdatePageLists(p);
                                                                    }
                                                                    
                                                                    _clientState[i].page = p;
                                                                    res[0] = CMDOK;
                                                                }
                                                            }
                                                        }
                                                    }
                                                    else
                                                        res[0] = CMDERR;
                                                }
                                                else if ((cmd == PAGESETSUB || cmd == PAGEDELSUB) && _clientState[i].page)
                                                {
                                                    _clientState[i].subpage = nullptr; // invalidate previous subpage
                                                    if (n == 5)
                                                    {
                                                        std::stringstream ss;
                                                        ss << "[InterfaceServer::run] Client " << i << ": " << ((cmd==PAGESETSUB)?"PAGESETSUB ":"PAGEDELSUB ") << std::hex << num;
                                                        _debug->Log(Debug::LogLevels::logDEBUG,ss.str());
                                                        
                                                        if ((num & 0xc080) || num >= 0x3f7f) // reject invalid subpage numbers
                                                        {
                                                            res[0] = CMDERR;
                                                        }
                                                        else
                                                        {
                                                            _clientState[i].subpage = _clientState[i].page->LocateSubpage(num);
                                                            if (_clientState[i].subpage == nullptr) // subpage not found
                                                            {
                                                                if (cmd == PAGESETSUB)
                                                                {
                                                                    _clientState[i].subpage = std::shared_ptr<Subpage>(new Subpage()); // create new subpage
                                                                    _clientState[i].subpage->SetSubCode(num); // set subcode first
                                                                    _clientState[i].page->InsertSubpage(_clientState[i].subpage); // add to page
                                                                    _pageList->UpdatePageLists(_clientState[i].page);
                                                                    _clientState[i].subpage->SetSubpageStatus(PAGESTATUS_TRANSMITPAGE);
                                                                    
                                                                    // ------------------- Debug: add a test row -----------------
                                                                    std::stringstream ss;
                                                                    ss << "TEST " << std::hex << std::setw(4) << std::setfill('0') << num;
                                                                    std::shared_ptr<TTXLine> tmp(new TTXLine(ss.str()));
                                                                    _clientState[i].subpage->SetRow(1,tmp);
                                                                    // -----------------------------------------------------------
                                                                    
                                                                    if (_clientState[i].page->GetOneShotFlag()) // page is a oneshot
                                                                        _clientState[i].page->SetSubpage(num); // put this subpage on air
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
                                                                    if (_clientState[i].page->GetOneShotFlag()) // page is a oneshot
                                                                        _clientState[i].page->SetSubpage(num); // put this subpage on air
                                                                }
                                                                else // PAGEDELSUB
                                                                {
                                                                    _clientState[i].page->RemoveSubpage(_clientState[i].subpage);
                                                                }
                                                            }
                                                            unsigned int count = _clientState[i].page->GetSubpageCount();
                                                            res.push_back((count >> 8) & 0xff);
                                                            res.push_back(count & 0xff); // return subpage count (big endian)
                                                        }
                                                    }
                                                    else
                                                        res[0] = CMDERR;
                                                }
                                            }
                                            else if (cmd == PAGECLOSE)
                                            {
                                                std::stringstream ss;
                                                ss << "[InterfaceServer::run] Client " << i << ": PAGECLOSE";
                                                _debug->Log(Debug::LogLevels::logDEBUG,ss.str());
                                                if (_clientState[i].page && n==3)
                                                {
                                                    _clientState[i].page->FreeLock();
                                                    _clientState[i].page = nullptr;
                                                    _clientState[i].subpage = nullptr;
                                                }
                                                else
                                                {
                                                    res[0] = CMDERR;
                                                }
                                            }
                                            else if (cmd == PAGEFANDC)
                                            {
                                                if (_clientState[i].page)
                                                {
                                                    std::stringstream ss;
                                                    ss << "[InterfaceServer::run] Client " << i << ": PAGEFANDC";
                                                    _debug->Log(Debug::LogLevels::logDEBUG,ss.str());
                                                    if (n == 5) // write
                                                    {
                                                        _clientState[i].page->SetPageFunctionInt((uint8_t)readBuffer[3]);
                                                        _clientState[i].page->SetPageCodingInt((uint8_t)readBuffer[4]);
                                                        _pageList->UpdatePageLists(_clientState[i].page);
                                                    }
                                                    else if (n != 3)
                                                    {
                                                        res[0] = CMDERR;
                                                    }
                                                    res.push_back(_clientState[i].page->GetPageFunction());
                                                    res.push_back(_clientState[i].page->GetPageCoding());
                                                }
                                                else
                                                {
                                                    res[0] = CMDERR;
                                                }
                                            }
                                            else if (cmd == PAGEOPTNS)
                                            {
                                                if (_clientState[i].subpage)
                                                {
                                                    std::stringstream ss;
                                                    ss << "[InterfaceServer::run] Client " << i << ": PAGEOPTNS";
                                                    _debug->Log(Debug::LogLevels::logDEBUG,ss.str());
                                                    
                                                    if (n == 8) // write
                                                    {
                                                        _clientState[i].subpage->SetSubpageStatus(((uint8_t)readBuffer[3] << 8) | (uint8_t)readBuffer[4]);
                                                        _clientState[i].subpage->SetRegion((uint8_t)readBuffer[5]);
                                                        _clientState[i].subpage->SetCycleTime((uint8_t)readBuffer[6]);
                                                        _clientState[i].subpage->SetTimedMode((uint8_t)readBuffer[7] == 1);
                                                    }
                                                    else if (n != 3)
                                                    {
                                                        res[0] = CMDERR;
                                                    }
                                                    uint16_t status = _clientState[i].subpage->GetSubpageStatus();
                                                    res.push_back((status >> 8) & 0xff);
                                                    res.push_back(status & 0xff);
                                                    res.push_back(_clientState[i].subpage->GetRegion());
                                                    res.push_back(_clientState[i].subpage->GetCycleTime());
                                                    res.push_back(_clientState[i].subpage->GetTimedMode()?1:0);
                                                }
                                                else
                                                {
                                                    res[0] = CMDERR;
                                                }
                                            }
                                            else if (cmd == PAGEROW)
                                            {
                                                std::stringstream ss;
                                                ss << "[InterfaceServer::run] Client " << i << ": PAGEROW";
                                                _debug->Log(Debug::LogLevels::logDEBUG,ss.str());
                                            }
                                            else if (cmd == PAGELINKS)
                                            {
                                                std::stringstream ss;
                                                ss << "[InterfaceServer::run] Client " << i << ": PAGELINKS";
                                                _debug->Log(Debug::LogLevels::logDEBUG,ss.str());
                                            }
                                            else if (cmd > PAGELINKS) // last defined command number
                                            {
                                                std::stringstream ss;
                                                ss << "[InterfaceServer::run] Client " << i << ": Unknown PAGESAPI command received " << std::hex << cmd;
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
                                
                                int n = send(sock, (char*)res.data(), res.size(), 0); // send response
                                if (n > 0)
                                    continue; // next socket in loop
                            }
                            // else fall through to error handling
                        }
                    }
                    // couldn't read/write all bytes
                    getpeername(sock, (struct sockaddr*)&address, &addrlen);
                    if (n == 0)
                    {
                        /* client disconnected */
                        _debug->Log(Debug::LogLevels::logINFO,"[InterfaceServer::run] closing connection from " + std::string(inet_ntoa(address.sin_addr)) + ":" + std::to_string(ntohs(address.sin_port)) + " on socket " + std::to_string(sock));
                    }
                    else
                    {
                        #ifdef WIN32
                            int e = WSAGetLastError();
                        #else
                            int e = errno;
                        #endif
                        
                        _debug->Log(Debug::LogLevels::logWARN,"[InterfaceServer::run] closing connection from " + std::string(inet_ntoa(address.sin_addr)) + ":" + std::to_string(ntohs(address.sin_port)) + " recv error " + std::to_string(e) + " on socket " + std::to_string(sock));
                    }
                    
                    /* close the socket when any error occurs */
                    CloseClient(&_clientState[i]);
                }
            }
        }
    }
}
