/**
 ***************************************************************************
 * @brief      Header for Newfor subtitles
 * @file       command.h
 * Compiler          : C++
 * @author     Peter Kwan
 * @date       July, 2017
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
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 ***************************************************************************
 * Newfor subtitles
 * @description Softel Newfor subtitles for VBIT.
 * A TCP port 5570 on VBIT2 accepts newfor subtitling commands.
 * It can be used to:
 * Play out pre-recorded subtitles from a file or a video server
 * Play bridged subtitles from an inserter that supports Newfor like the DTP600
 * or accept any industry standard Newfor source.
**/
#ifndef _COMMAND_H_
#define _COMMAND_H_

#include <iostream>
#include <cstring>
#include <stdint.h>

#ifdef WIN32
#include <winsock2.h>
#else
#include <sys/ioctl.h>

#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#endif

#include "pagelist.h"
#include "TCPClient.h"

namespace vbit
{
    class Command
    {
        public:
            /**
             * @brief Constructor
             * @description Listens on port 5570 and accepts connections.
             * When connected it can be sent Newfor commands.
             */
            Command(ttx::Configure *configure, vbit::PacketSubtitle* subtitle, ttx::PageList* pageList);

            /**
             * @brief Destructor
             */
            ~Command();

            /**
             * @brief Run the listener thread.
             */
            void run();

        private:
            int _portNumber; /// The port number is configurable. Default is 5570 for no oarticular reason.
            TCPClient _client; /// Did I call this a client? It is where clients connect and get their commands executed.

            /* Page init and subtitle data can respond with these standard codes */
            static const uint8_t ASCII_ACK=0x06;
            static const uint8_t ASCII_NACK=0x15;
            
            static const uint8_t MAXPENDING=5;    /* Maximum outstanding connection requests */

            void DieWithError(std::string errorMessage);  /* Error handling function */
    };
}

#endif
