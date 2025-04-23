/** Top level teletext application
 *  @brief Load the configuration, initialise the service and run it.
 * @detail This can be run without parameters in which case it will use the defaults in configure.
 * If a parameter is given then it replaces the defaults.
 * @todo Define options for overriding the config file settings, and the config file itself,
 * This will include path to pages, header packet, priority etc.
 * Example: vbit2 --config teletext/vbit.conf
 * Compiler: c++11
 */

/** ***************************************************************************
 * Description       : Top level teletext stream generator
 * Compiler          : C++
 *
 * Copyright (C) 2016, Peter Kwan
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
 *************************************************************************** **/

#include "vbit2.h"

using namespace vbit;
using namespace ttx;

MasterClock *MasterClock::instance = 0; // initialise MasterClock singleton

/* Options
 * --dir <path to pages>
 * Sets the pages directory and the location of vbit.conf.
 */

int main(int argc, char** argv)
{
    #ifdef WIN32
    _setmode(_fileno(stdout), _O_BINARY); // set stdout to binary mode stdout to avoid pesky line ending conversion
    #endif
    
    Debug *debug=new Debug();
    
    /// @todo option of adding a non standard config path
    Configure *configure=new Configure(debug, argc, argv);
    
    // attempt to use system locale for strftime
    if (std::setlocale(LC_TIME, "") == nullptr)
    {
        debug->Log(Debug::LogLevels::logERROR,"[main] Unable to set locale");
    }
    
    PageList *pageList=new PageList(configure, debug);
    PacketServer *packetServer=new PacketServer(configure, debug);
    DatacastServer *datacastServer=new DatacastServer(configure, debug);

    Service* svc=new Service(configure, debug, pageList, packetServer, datacastServer); // Need to copy the subtitle packet source for Newfor

    std::thread monitorThread(&FileMonitor::run, FileMonitor(configure, debug, pageList));
    std::thread serviceThread(&Service::run, svc);

    if (configure->GetCommandPortEnabled())
    {
        // only start command thread if required
        std::thread commandThread(&Command::run, Command(configure, debug, svc->GetSubtitle(), pageList) );
        commandThread.detach();
    }

    if (configure->GetPacketServerEnabled())
    {
        // only start packet server thread if required
        std::thread packetServerThread(&PacketServer::run, packetServer );
        packetServerThread.detach();
    }

    if (configure->GetDatacastServerEnabled())
    {
        // only start datacast server thread if required
        std::thread datacastServerThread(&DatacastServer::run, datacastServer );
        datacastServerThread.detach();
    }

    // The threads should never stop, but just in case...
    monitorThread.join();
    serviceThread.join();

    return 0;
}

