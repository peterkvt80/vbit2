/* Provide a TCP packet server which sends a whole frame of t42 data at a time to all connected clients */

#include "packetServer.h"

using namespace vbit;

PacketServer::PacketServer(Configure *configure, Debug *debug) :
    _debug(debug),
    _portNumber(configure->GetPacketServerPort()),
    _isActive(false)
{
    /* initialise sockets */
    _serverSock = -1;
    
    for (int i = 0; i < MAXCLIENTS; i++)
        _clientSocks[i] = -1;
}

PacketServer::~PacketServer()
{
}

void PacketServer::DieWithError(std::string errorMessage)
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
    }
    
    perror(errorMessage.c_str());
    exit(1);
}

void PacketServer::SendField(std::vector<std::vector<uint8_t>> FrameBuffer)
{
    std::array<uint8_t, 42*32> RawFrameBuffer;
    RawFrameBuffer.fill(0x00); // clear two frames
    unsigned int i, j;
    
    for (i = 0; i < FrameBuffer.size(); i++)
    {
        int field = FrameBuffer[i][0]&1;
        int line = (FrameBuffer[i][1]<<8) + FrameBuffer[i][2];
        if (line < 16) // full field lines not supported in this output
        {
            for (j = 0; j < 42; j++)
            {
                RawFrameBuffer[(field*42*16)+(line*42)+j] = FrameBuffer[i][j+3];
            }
        }
    }
    
    int sock;
    int ret;
    for (i = 0; i < MAXCLIENTS; i++)
    {
        if (_mtx[i].try_lock()) // skip this socket if unable to lock mutex as it's in the process of being closed
        {
            sock = _clientSocks[i];
            if (sock >= 0)
            {
                ret = send(sock, (char*)RawFrameBuffer.data(), RawFrameBuffer.size(), 0);
                if (ret != RawFrameBuffer.size())
                {
                    /*
                        We were unable to send a whole frame to the client. We either sent a partial frame or the send failed entirely.
                        This probably means that either there are network issues, or the client is not consuming data fast enough.
                        Either way trying to handle this adds a lot of complexity and risks getting the client desynchronised or blocking the thread trying to sort it out, so the best thing to do is probably just boot the client off and let it reconnect.
                    */
                    
                    #ifdef WIN32
                        int e = WSAGetLastError();
                    #else
                        int e = errno;
                    #endif
                    
                    _debug->Log(Debug::LogLevels::logWARN,"[PacketServer::SendField] send() failed. Closing socket " + std::to_string(sock) + " send error " + std::to_string(e));
                    
                    _clientSocks[i] = -1; /* free slot */
                    
                    #ifdef WIN32
                        closesocket(sock);
                    #else
                        close(sock);
                    #endif
                }
            }
            _mtx[i].unlock();
        }
    }
}

void PacketServer::run()
{
    _debug->Log(Debug::LogLevels::logDEBUG,"[PacketServer::run] TCP packet server thread started");
    
    int newSock;
    int sock;
    struct sockaddr_in address;
    unsigned short servPort;
    
    char readBuffer[BUFFLEN];
    
    fd_set readfds;
    
#ifdef WIN32
    int addrlen;
    WSADATA wsaData;
    int iResult;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0)
    {
        DieWithError("[PacketServer::run] WSAStartup failed");
    }
#else
    unsigned int addrlen;
#endif
    
    servPort = _portNumber;
    
    /* Create socket */
    if ((_serverSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("[PacketServer::run] socket() failed\n");
    
    int reuse = true;
    
    /* Allow multiple connnections */
    if(setsockopt(_serverSock, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse)) < 0)
        DieWithError("[PacketServer::run] setsockopt() SO_REUSEADDR failed");
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(servPort);
    
    /* bind socked */
    if (bind(_serverSock, (struct sockaddr *) &address, sizeof(address)) < 0)
        DieWithError("[PacketServer::run] bind() failed");
    
    /* Listen for incoming connections */
    if (listen(_serverSock, MAXPENDING) < 0)
        DieWithError("[PacketServer::run] listen() failed");
    
    addrlen = sizeof(address);
    
    int iopt = 42*32*25; /* 25 frames worth of t42 */
    
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
            DieWithError("[PacketServer::run] select() failed");
        
        if (FD_ISSET(_serverSock, &readfds))
        {
            /* incoming connection to server */
            if ((newSock = accept(_serverSock, (struct sockaddr *)&address, &addrlen))<0)
                DieWithError("[PacketServer::run] accept() failed");
            
            #ifdef WIN32
                u_long ul = 1;
                if (ioctlsocket(newSock, FIONBIO, &ul) < 0)
                    DieWithError("[PacketServer::run] ioctlsocket() failed");
            #else
                if (fcntl(newSock, F_SETFL, fcntl(newSock, F_GETFL, 0) | O_NONBLOCK) < 0)
                    DieWithError("[PacketServer::run] fcntl() failed");
            #endif
            
            if (setsockopt(newSock, SOL_SOCKET, SO_SNDBUF, (char *) &iopt, sizeof(iopt)) < 0 )
                DieWithError("[PacketServer::run] setsockopt() failed");
            
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
                    _debug->Log(Debug::LogLevels::logWARN,"[PacketServer::run] reject new connection from " + std::string(inet_ntoa(address.sin_addr)) + " (too many connections)");
                    break;
                }
                
                /* find unused slot */
                if( _clientSocks[i] < 0 )
                {
                    /* add to active sockets */
                    _clientSocks[i] = newSock;
                    _debug->Log(Debug::LogLevels::logINFO,"[PacketServer::run] new connection from " + std::string(inet_ntoa(address.sin_addr)) + ":" + std::to_string(ntohs(address.sin_port)) + " as socket " + std::to_string(newSock));
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
                    
                    int n = recv(sock, readBuffer, BUFFLEN, 0);
                    if (n == 0)
                    {
                        /* client disconnected */
                        getpeername(sock, (struct sockaddr*)&address, &addrlen);
                        
                        _debug->Log(Debug::LogLevels::logINFO,"[PacketServer::run] closing connection from " + std::string(inet_ntoa(address.sin_addr)) + ":" + std::to_string(ntohs(address.sin_port)) + " on socket " + std::to_string(sock));
                        
                        _mtx[i].lock();
                        _clientSocks[i] = -1; /* free slot */
                        
                        #ifdef WIN32
                            closesocket(sock);
                        #else
                            close(sock);
                        #endif
                        _mtx[i].unlock();
                    }
                    else if (n > 0)
                    {
                        // don't care what client sent right now
                    }
                    else /* n < 0 */
                    {
                        #ifdef WIN32
                            int e = WSAGetLastError();
                        #else
                            int e = errno;
                        #endif
                        
                        getpeername(sock, (struct sockaddr*)&address, &addrlen);
                        
                        _debug->Log(Debug::LogLevels::logWARN,"[PacketServer::run] closing connection from " + std::string(inet_ntoa(address.sin_addr)) + ":" + std::to_string(ntohs(address.sin_port)) + " recv error " + std::to_string(e) + " on socket " + std::to_string(sock));
                        
                        /* close the socket when any error occurs */
                        _mtx[i].lock();
                        _clientSocks[i] = -1; /* free slot */
                        
                        #ifdef WIN32
                            closesocket(sock);
                        #else
                            close(sock);
                        #endif
                        _mtx[i].unlock();
                    }
                }
            }
        }
    }
}
