/** Service
 */
#include "service.h"

#include <ctime>

using namespace ttx;

Service::Service()
{
	// std::cerr << "[Service::Service] Started" << std::endl;
}

Service::Service(Configure *configure, PageList *pageList) :
	_configure(configure),_pageList(pageList)
{
	// std::cerr << "[Service::Service] Started (2) " << std::endl;
//	_pageList->loadPageList(_configure->GetPageDirectory());

}

Service::~Service()
{
	// std::cerr << "[Service] Destructor" << std::endl;
}

bool Service::run()
{
	std::cerr << "[Service::run] This should start a thread that starts the teletext stream" << std::endl;
	std::thread t(&Service::worker, this);
    std::cerr << "[Service::run]Main thread" << std::endl;
    t.join();
    std::cerr << "[Service::run]Main thread ended" << std::endl;
	return false;
}


void Service::worker()
{
    // @todo Put priority into config and add commands to allow updates.
    char priority[STREAMS]={5,3,3,3,3,4,5,9,1};	// 1=High priority,9=low. Note: priority[0] is mag 8, while priority mag[8] is the newfor stream!
    char priorityCount[STREAMS];
    uint8_t nmag=1;
    vbit::Mag **mag; // Pointer to magazines array
    vbit::Mag *pMag; // Pointer to the magazine that we are working on
    uint8_t hold[STREAMS]; /// If hold is set then the magazine can not be sent until the next field
    std::cerr << "[Service::worker]This is the worker process" << std::endl;

    uint8_t rowCounter=0; // Counts 16 rows to a field

    // Initialise the priority counts
	for (uint8_t i=0;i<STREAMS-1;i++)
	{
		hold[i]=0;
		priorityCount[i]=priority[i];
	}

	// Get the magazines
	mag=_pageList->GetMagazines();

	/// Check that we got what we expect
	for (int i=0;i<8;i++)
	{
		std::cerr << "[Service::worker] Mag [" << i << "] count=" << mag[i]->GetPageCount() << std::endl;
	}

	int debugMode=3; // 0=normal, 1=debug, 3=magazine debug 4=counter 5=iterate limit

	int debugCounter=0;

	while(1) // normal
	{
        if (debugMode==4)
        {
            if (debugCounter>1000)
            {
                std::cout << "S";
                debugCounter=0;
            }
            debugCounter++;
        }

        // Hold after n iterations, then exit.
        if (debugMode==5)
        {
            struct timespec rec;
            int ms=20;
            rec.tv_sec = ms / 1000;
            rec.tv_nsec=(ms % 1000) *1000000;

            nanosleep(&rec,NULL);

            std::cerr << "*";
            debugCounter++;
            if (debugCounter>1000000)
            {
                std::cerr << "Press Key+Return to quit" << std::endl;
                char c;
                std::cin >> c;
                exit(3);
            }
        }

		// Find the next magazine to put out.
		// We decrement the priority count for each magazine until one reaches 0.
		for (;priorityCount[nmag]>0;nmag=(nmag+1)%(STREAMS-1)) /// @todo Subtitles need stream 8. But we can't access magazine 9
		{
			priorityCount[nmag]--;
		}
		pMag=mag[nmag];

		// If the magazine has no pages it can be put into hold.
		// @todo Subtitles are an exception. They should not have a page but they could be skipped.
		if (pMag->GetPageCount()<1)
		{
			hold[nmag]=true;
		}
		else
		{
			if (!hold[nmag]) 		// Not in hold
			{
				if (debugMode==4) std::cout << (int)nmag;
				vbit::Packet* pkt=pMag->GetPacket();
				if (pkt!=NULL) // How could this be NULL? After the last packet of a page has been sent.
				{
					std::string s=pkt->tx();
					if (s.length()<42)
					{
						// This happens during fastext. Might need to check this
						//std::cout << "Length=" << s.length() << std::endl;
						//exit(3);
					}
					// Set this to 0=normal 1=debug mode 3=wtf
					switch (debugMode)
					{
					case 0:
						std::cout << s.substr(0,42); // Transmit the packet
						break;
					case 1:
						pkt->tx(true);
						std::cout << std::endl;
						break;
					case 3:
						std::cout << std::setfill('0') << std::hex;
						if (nmag==1) // Filter for mag 1 only
						{
							if (pkt->LastPacketWasHeader())
								std::cout << (int)nmag << "*" << std::setw(2) << pkt->GetPage() << " " << std::dec; // Page number
							else
								std::cout << (int)nmag << "-" << std::dec << std::setw(2) << pkt->GetRow()  << " "; // Row number
						}
						else
							std::cout << "    ";
						break;
					case 4:
							std::cout << "P";
							break;
					case 5:
							break;
					default:
						exit(3);
					}
					// Was the last packet a header? If so we need to go into hold
					if (pkt->LastPacketWasHeader())
					{
						hold[nmag]=true;
						// std::cerr << "Header wait mode mag=" << nmag << std::endl;
					}
					rowCounter++;
					if (rowCounter>=16)
					{
						rowCounter=0;
						for (uint8_t i=0;i<STREAMS-1;i++) hold[i]=0;	// Any holds are released now
						// Should put packet 8/30 stuff here as in vbit stream.c
						if (debugMode==3 || debugMode==4)
							std::cout << std::endl;
					}
				} // not a null packet
				else
        {					
						// This should only happen for a text row that returns empty
            // std::cerr << "We got a NULL on mag " << (int)nmag << " and we don't like it";
						// exit(3);
				}
			} // not in hold
			else
      {
				// To avoid a deadlock, we check if everything is in hold
				bool blocked=true;
				for (uint8_t i=0;i<STREAMS-1;i++) // Hold does not include stream 9 (subtitles)
				{
						if (hold[i]==false)
						{
								blocked=false;
								break;
						}
				}
				if (blocked)
				{
					vbit::Packet* p=new vbit::Packet();  // @todo Again, we should have a pre-prepared quiet packet to avoid eating the heap
					p->PacketQuiet();
					std::cout << p->tx();
					// Step the row counter
					rowCounter++;
					if (rowCounter>=16)
					{
						rowCounter=0;
						for (uint8_t i=0;i<STREAMS;i++) hold[i]=0;	// Any holds are released now
					}
				} // blocked
			}
		} // page count is positive

		// Reset the priority of this magazine
		if (priority[nmag]==0)
			priority[nmag]=1;	// Can't be 0 or that mag will take all the packets
		priorityCount[nmag]=priority[nmag];	// Reset the priority for the mag that just went out
	} // while

} // worker


