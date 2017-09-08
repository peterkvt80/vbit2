/**
 ***************************************************************************
 * Description       : Class for Newfor subtitles
 * Compiler          : C++
 *
 * Copyright (C) 2017, Peter Kwan
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaims all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 ***************************************************************************
 * The command processor accepts commands over TCP.
 * The primary use is to accept Newfor subtitles
 * but other inserter commands could be done this way
 * It arbitrarily uses port 5570 and accepts Newfor commands
**/
#include "command.h"

using namespace vbit;
using namespace ttx;

Command::Command(const uint32_t port=5570, PacketSubtitle* subtitle=nullptr, PageList *pageList=nullptr) :
	_portNumber(port),
	_client(subtitle, pageList)
{
    // Constructor
		// Start a listener thread
}

Command::~Command()
{
    //destructor
}

void Command::DieWithError(std::string errorMessage)
{
    perror(errorMessage.c_str());
    exit(1);
}

void Command::run()
{
  std::cerr << "[Command::run] Newfor subtitle listener started" << std::endl;
  int serverSock;                    /* Socket descriptor for server */
  int clientSock;                    /* Socket descriptor for client */
  struct sockaddr_in echoServAddr; /* Local address */
  struct sockaddr_in echoClntAddr; /* Client address */
  unsigned short echoServPort;     /* Server port */

#ifdef WIN32
  int clntLen;                     /* needs to be signed int for winsock */
	WSADATA wsaData;
  int iResult;

	// Initialize Winsock
  iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
  if (iResult != 0) {
		DieWithError("WSAStartup failed");
  }
#else
	unsigned int clntLen;            /* Length of client address data structure */
#endif

	echoServPort = _portNumber;  /* This is the local port */

	// System initialisations
	/* Construct local address structure */
  std::memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
  echoServAddr.sin_family = AF_INET;                /* Internet address family */
  echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
  echoServAddr.sin_port = htons(echoServPort);      /* Local port */

  /* Create socket for incoming connections */
  if ((serverSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
      DieWithError("socket() failed\n");

  /* Bind to the local address */
  if (bind(serverSock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
      DieWithError("bind() failed");

  /* Mark the socket so it will listen for incoming connections */
  if (listen(serverSock, MAXPENDING) < 0)
    DieWithError("listen() failed");

	/* Set the size of the in-out parameter */
	clntLen = sizeof(echoClntAddr);

	while(1)
	{
    std::cerr << "[Command::run] Ready for a client to connect" << std::endl;

		/* Wait for a client to connect */
		if ((clientSock = accept(serverSock, (struct sockaddr *) &echoClntAddr,
							   &clntLen)) < 0)
			DieWithError("accept() failed");
    std::cerr << "[Command::run] Connected" << std::endl;

		/* clientSock is connected to a client! */

		// printf("Handling client %s\n", inet_ntoa(echoClntAddr.sin_addr));

		_client.Handler(clientSock);

	}
}
