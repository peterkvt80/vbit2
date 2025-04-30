/* Provide a TCP server for injecting datacast packet streams */

#include "datacastServer.h"

using namespace vbit;

DatacastServer::DatacastServer(Configure *configure, Debug *debug) :
    _configure(configure),
    _debug(debug),
    _portNumber(configure->GetDatacastServerPort()),
    _isActive(false)
{
    /* initialise sockets */
    _serverSock = -1;
    
    for (int i=0; i < MAXCLIENTS; i++)
    {
        _clientSocks[i] = -1;
        _clientChannel[i] = -1; // no channel set
    }
    
    _datachannel[0]=nullptr; // can't use datachannel 0
    for (int dc=1; dc<16; dc++)
    {
        _datachannel[dc] = new PacketDatacast(dc, configure); // create 15 datacast channels
    }
}

DatacastServer::~DatacastServer()
{
}

void DatacastServer::SocketError(std::string errorMessage)
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
        if (_clientSocks[i] >= 0)
        #ifdef WIN32
            closesocket(_clientSocks[i]);
        #else
            close(_clientSocks[i]);
        #endif
        _clientChannel[i] = -1; // no channel set
    }
    
    std::cerr << errorMessage;
}

void DatacastServer::run()
{
    _debug->Log(Debug::LogLevels::logDEBUG,"[DatacastServer::run] Datacast server thread started");
    
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
        SocketError("[DatacastServer::run] WSAStartup failed\n");
        return;
    }
#else
    unsigned int addrlen;
#endif
    
    /* Create socket */
    if ((_serverSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        SocketError("[DatacastServer::run] socket() failed\n");
        return;
    }
    
    int reuse = true;
    
    /* Allow multiple connnections */
    if(setsockopt(_serverSock, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse)) < 0)
    {
        SocketError("[DatacastServer::run] setsockopt() SO_REUSEADDR failed\n");
        return;
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(_portNumber);
    
    /* bind socked */
    if (bind(_serverSock, (struct sockaddr *) &address, sizeof(address)) < 0)
    {
        SocketError("[DatacastServer::run] bind() failed\n");
        return;
    }
    
    /* Listen for incoming connections */
    if (listen(_serverSock, MAXPENDING) < 0)
    {
        SocketError("[DatacastServer::run] listen() failed\n");
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
            sock = _clientSocks[i];
            
            if(sock >= 0){
                FD_SET(sock , &readfds);
                active = true; /* packet server has connections */
            }
        }
        _isActive = active;
        
        /* wait for activity on any socket */
        if ((select(FD_SETSIZE, &readfds, NULL, NULL, NULL) < 0) && (errno!=EINTR))
            SocketError("[DatacastServer::run] select() failed");
        
        if (FD_ISSET(_serverSock, &readfds))
        {
            /* incoming connection to server */
            if ((newSock = accept(_serverSock, (struct sockaddr *)&address, &addrlen))<0)
            {
                SocketError("[DatacastServer::run] accept() failed");
                return;
            }
            
            #ifdef WIN32
                u_long ul = 1;
                if (ioctlsocket(newSock, FIONBIO, &ul) < 0)
                {
                    SocketError("[DatacastServer::run] ioctlsocket() failed");
                    return;
                }
            #else
                if (fcntl(newSock, F_SETFL, fcntl(newSock, F_GETFL, 0) | O_NONBLOCK) < 0)
                {
                    SocketError("[DatacastServer::run] fcntl() failed");
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
                    _debug->Log(Debug::LogLevels::logWARN,"[DatacastServer::run] reject new connection from " + std::string(inet_ntoa(address.sin_addr)) + " (too many connections)");
                    break;
                }
                
                /* find unused slot */
                if( _clientSocks[i] < 0 )
                {
                    /* add to active sockets */
                    _clientSocks[i] = newSock;
                    _clientChannel[i] = -1; // no channel set
                    _debug->Log(Debug::LogLevels::logINFO,"[DatacastServer::run] new connection from " + std::string(inet_ntoa(address.sin_addr)) + ":" + std::to_string(ntohs(address.sin_port)) + " as socket " + std::to_string(newSock));
                    break;
                }
            }
        }
        else
        {
            /* a client socket has activity */
            for (int i = 0; i < MAXCLIENTS; i++)
            {
                sock = _clientSocks[i];
                
                if (sock >= 0 && FD_ISSET(sock , &readfds))
                {
                    /* socket has activity */
                    
                    int n = recv(sock, readBuffer, 1, MSG_PEEK); // peek at first byte of message
                    if (n == 1)
                    {
                        // byte 0 of message is message length
                        int len = (uint8_t)readBuffer[0];
                        n = recv(sock, readBuffer, len, 0); // try to read whole message
                        if (n == len)
                        {
                            std::vector<uint8_t> res = {DCOK};
                            
                            // byte 1 of message is command number
                            switch (readBuffer[1]){
                                case DCSET: // set datacast channel
                                {
                                    int ch = readBuffer[2];
                                    if (n == 3 && ch >= 0 && ch <= 15)
                                    {
                                        _clientChannel[i] = -1; // release a current datachannel
                                        
                                        // check if another client is using desired datachannel
                                        for (int j=0; j<MAXCLIENTS; j++){
                                            if (_clientChannel[j] == ch)
                                                goto DCSETError; // jump out to return error
                                        }
                                        
                                        _clientChannel[i] = ch; // use requested channel
                                        std::cerr << "set datachannel " << std::hex << ch << std::endl;
                                        break;
                                    }
                                    
                                    DCSETError:
                                    res[0] = DCERR;
                                    break;
                                }
                                
                                case DCRAW: // push raw datacast packet to buffer
                                {
                                    if (_clientChannel[i] > 0 && n == 42) // 40 bytes of packet data
                                    {
                                        std::vector<uint8_t> data(readBuffer+2, readBuffer+n);
                                        
                                        if(_datachannel[_clientChannel[i]]->PushRaw(&data))
                                        {
                                            res[0] = DCFULL; // buffer full
                                        }
                                    }
                                    else
                                    {
                                        res[0] = DCERR;
                                    }
                                    break;
                                }
                                
                                case DCFORMATA: // encode and push a format A datacast payload to buffer
                                {
                                    /* Does _not_ automate repeats, continuity indicator, etc.
                                       Format is:
                                         byte 2: bits 4-7 IAL, bits 1-3 flags RI,CI,DL
                                         byte 3-5: 24 bit Service Packet Address (little endian)
                                         byte 6: Repeat indicator
                                         byte 7: Continuity indicator
                                         byte 8+: payload data
                                    */
                                    if (_clientChannel[i] > 0 && n > 8)
                                    {
                                        uint8_t flags = readBuffer[2] & 0xe;
                                        uint8_t ial = readBuffer[2] >> 4;
                                        uint32_t spa = (uint8_t)readBuffer[3] | ((uint8_t)readBuffer[4] << 8) | ((uint8_t)readBuffer[5] << 16);
                                        uint8_t ri = readBuffer[6];
                                        uint8_t ci = readBuffer[7];
                                        
                                        std::vector<uint8_t> data(readBuffer+8, readBuffer+n);
                                        
                                        int bytes = _datachannel[_clientChannel[i]]->PushIDLA(flags, ial, spa, ri, ci, &data);
                                        
                                        if (bytes == 0) // buffer full
                                            res[0] = DCFULL;
                                        else if (bytes < n-8) // payload didn't fit
                                            res[0] = DCTRUNC; // warn of truncation
                                        
                                        res.push_back(bytes); // return number of bytes written
                                        
                                        break;
                                    }
                                    else
                                    {
                                        res[0] = DCERR;
                                        res.push_back(0); // no bytes written
                                    }
                                    break;
                                }
                                
                                case DCCONFIG:
                                {
                                    if (_clientChannel[i] == 0 && n > 2) /* VBIT2 configuration commands on datachannel 0 only */
                                    {
                                        switch(readBuffer[2]){ // byte 2 is configuration command number
                                            case CONFRAFLAG: /* get/set row adaptive flag */
                                            {
                                                if (n == 4)
                                                {
                                                    _configure->SetRowAdaptive((readBuffer[3]&1)?true:false);
                                                }
                                                else if (n != 3)
                                                {
                                                    res[0] = DCERR;
                                                }
                                                res.push_back(_configure->GetRowAdaptive()?1:0);
                                                
                                                break;
                                            }
                                            
                                            case CONFRBYTES: /* get/set BSDP reserved bytes */
                                            {
                                                if (n == 7) // set new bytes
                                                {
                                                    _configure->SetReservedBytes(std::array<uint8_t, 4>({(uint8_t)readBuffer[3],(uint8_t)readBuffer[4],(uint8_t)readBuffer[5],(uint8_t)readBuffer[6]}));
                                                }
                                                else if (n != 3)
                                                {
                                                    res[0] = DCERR;
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
                                                        tmp << (char)readBuffer[i];
                                                    }
                                                    
                                                    _configure->SetServiceStatusString(tmp.str());
                                                }
                                                else if (n != 3)
                                                {
                                                    res[0] = DCERR;
                                                }
                                                
                                                for(char& c : _configure->GetServiceStatusString()) {
                                                    res.push_back((uint8_t)c);
                                                }
                                                
                                                break;
                                            }
                                            
                                            case CONFHEADER: /* get/set header template */
                                            {
                                                if (n == 35) // set new header template
                                                {
                                                    std::ostringstream tmp;
                                                    for (int i = 3; i < 35; i++)
                                                    {
                                                        /* strip to 7-bit values then add back high bit to control codes to match behaviour of templates loaded from config file */
                                                        uint8_t c = readBuffer[i] & 0x7F;
                                                        tmp << (char)((c<0x20)?(c|0x80):c);
                                                    }
                                                    
                                                    _configure->SetHeaderTemplate(tmp.str());
                                                }
                                                else if (n != 3)
                                                {
                                                    res[0] = DCERR;
                                                }
                                                
                                                for(char& c : _configure->GetHeaderTemplate()) {
                                                    res.push_back((uint8_t)c);
                                                }
                                                
                                                break;
                                            }
                                            
                                            default:
                                                res[0] = DCERR;
                                        }
                                    }
                                    else
                                    {
                                        res[0] = DCERR;
                                    }
                                    break;
                                }
                                
                                default: // unknown command
                                {
                                    res[0] = DCERR;
                                    break;
                                }
                            }
                            
                            if (res.size() > 254)
                            {
                                _debug->Log(Debug::LogLevels::logERROR,"[DatacastServer::run] Response too long");
                                res.resize(254); // truncate!
                            }
                            
                            res.insert(res.begin(), res.size()+1); // prepend message size
                            
                            int n = send(sock, (char*)res.data(), res.size(), 0); // send response
                            if (n > 0)
                                continue; // next socket in loop
                        }
                        // else fall through to error handling
                    }
                    // couldn't read/write all bytes
                    getpeername(sock, (struct sockaddr*)&address, &addrlen);
                    if (n == 0)
                    {
                        /* client disconnected */
                        _debug->Log(Debug::LogLevels::logINFO,"[DatacastServer::run] closing connection from " + std::string(inet_ntoa(address.sin_addr)) + ":" + std::to_string(ntohs(address.sin_port)) + " on socket " + std::to_string(sock));
                    }
                    else
                    {
                        #ifdef WIN32
                            int e = WSAGetLastError();
                        #else
                            int e = errno;
                        #endif
                        
                        _debug->Log(Debug::LogLevels::logWARN,"[DatacastServer::run] closing connection from " + std::string(inet_ntoa(address.sin_addr)) + ":" + std::to_string(ntohs(address.sin_port)) + " recv error " + std::to_string(e) + " on socket " + std::to_string(sock));
                    }
                    
                    /* close the socket when any error occurs */
                    _clientSocks[i] = -1; /* free slot */
                    
                    #ifdef WIN32
                        closesocket(sock);
                    #else
                        close(sock);
                    #endif
                }
            }
        }
    }
}
