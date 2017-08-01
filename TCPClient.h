/**
 ***************************************************************************
 * Description       : Header for TCP command handler
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
 * Handle network control commands.
 * Initially for subtitles but could also be used for other inserter
 * control functions.
**/

#ifndef _TCPCLIENT_H_
#define _TCPCLIENT_H_

#ifdef WIN32
#include <winsock2.h>
#else
#include <sys/socket.h> /* for recv() and send() */
#include <unistd.h>     /* for close() */
#endif

#include <stdint.h>

#include "newfor.h"

namespace vbit
{

class TCPClient
{
  public:
    TCPClient();
   ~TCPClient();
   void Handler(int clntSocket);
  private:
		static const uint8_t MAXCMD=128;
    static const uint8_t RCVBUFSIZE=132;   /* Size of receive buffer */	
	  char _cmd[MAXCMD];		
    void clearCmd(void);
    void addChar(char ch, char* response);
		
}; // TCPClient

} // End of namespace vbit


#endif
