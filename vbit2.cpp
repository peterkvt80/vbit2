#include "vbit2.h"

using namespace vbit;

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
    InterfaceServer *interfaceServer=new InterfaceServer(configure, debug);

    Service* svc=new Service(configure, debug, pageList, packetServer, interfaceServer); // Need to copy the subtitle packet source for Newfor

    std::thread monitorThread(&FileMonitor::run, FileMonitor(configure, debug, pageList));
    std::thread serviceThread(&Service::run, svc);

    if (configure->GetPacketServerEnabled())
    {
        // only start packet server thread if required
        std::thread packetServerThread(&PacketServer::run, packetServer );
        packetServerThread.detach();
    }

    if (configure->GetInterfaceServerEnabled())
    {
        // only start interface server thread if required
        std::thread interfaceServerThread(&InterfaceServer::run, interfaceServer );
        interfaceServerThread.detach();
    }

    // The threads should never stop, but just in case...
    monitorThread.join();
    serviceThread.join();

    return 0;
}

