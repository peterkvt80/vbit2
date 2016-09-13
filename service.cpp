/** Service
 */
#include "service.h"

using namespace ttx;

Service::Service()
{
	std::cout << "[Service::Service] Started" << std::endl;
}

Service::Service(Configure *configure, PageList *pageList) :
	_configure(configure),_pageList(pageList)
{
	std::cout << "[Service::Service] Started (2) " << std::endl;
//	_pageList->loadPageList(_configure->GetPageDirectory());

}

Service::~Service()
{
	std::cout << "[Service] Destructor" << std::endl;
}

bool Service::run()
{
	std::cout << "[Service::run] This should start a thread that starts the teletext stream" << std::endl;
	std::thread t(&Service::worker, this);
    std::cout << "[Service::run]Main thread" << std::endl;
    t.join();
    std::cout << "[Service::run]Main thread ended" << std::endl;
	return false;
}


void Service::worker()
{
    // @todo Put priority into config and add commands to allow updates.
    char priority[STREAMS]={5,3,3,3,3,2,5,9,1};	// 1=High priority,9=low. Note: priority[0] is mag 8, while priority mag[8] is the newfor stream!
    char priorityCount[STREAMS];
    uint8_t nmag;
    vbit::Mag **mag; // Pointer to magazines array
    vbit::Mag *pMag; // Pointer to the magazine that we are working on
    uint8_t hold[STREAMS]; /// If hold is set then the magazine can not be sent until the next field
    std::cout << "[Service::worker]This is the worker process" << std::endl;

    int rowCounter=16; // Counts 16 rows to a field

    // Initialise the priority counts
	for (uint8_t i=0;i<STREAMS;i++)
	{
		hold[i]=0;
		priorityCount[i]=priority[i];
	}

    // Get the magazines
    mag=_pageList->GetMagazines();

    /// Check that we got what we expect
    for (int i=0;i<8;i++)
    {
        std::cout << " Mag [" << i << "] count=" << mag[i]->GetPageCount() << std::endl;
    }

    for (int k=0;k<200;k++) // Debugging! This will be a while(1)
    {
        // Find the next magazine to put out
        for (;priorityCount[nmag]>0;nmag=(nmag+1)%(STREAMS-1)) /// @todo Subtitles need stream 8. But we can't access magazine 9
		{
			priorityCount[nmag]--;
		}
        pMag=mag[nmag];

        if (rowCounter>=16)
        {
            rowCounter=0;
			for (uint8_t i=0;i<STREAMS;i++) hold[i]=0;	// Any holds are released now
			// Should put packet 8/30 stuff here as in vbit stream.c
        }

        // If the magazine has no pages it can be put into hold
        if (pMag->GetPageCount()<1)
            hold[nmag]=true;

		// Does it have any pages and it isn't in hold
		if (!hold[nmag])
        {
            std::cout << "Mag=" << (int)nmag << std::endl;
            vbit::Packet* pkt=pMag->GetPacket();
        }

        //

        // Reset the priority of this magazine
		if (priority[nmag]==0) priority[nmag]=1;	// Can't be 0 or that mag will take all the packets
		priorityCount[nmag]=priority[nmag];	// Reset the priority for the mag that just went out
    }

}


