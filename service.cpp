/** Service
 */
#include "service.h"

using namespace ttx;
using namespace vbit;

Service::Service()
{
}

Service::Service(Configure *configure, PageList *pageList) :
	_configure(configure),_pageList(pageList)
{
  // @todo Put priority into config and add commands to allow updates.
  uint8_t priority[8]={5,3,3,6,3,3,5,9};	// 1=High priority,9=low. Note: priority[0] is mag 8

  vbit::Mag **magList=_pageList->GetMagazines();
  // Register all the packet sources
  for (uint8_t mag=0;mag<8;mag++)
  {
    vbit::Mag* m=magList[mag];
    std::list<TTXPageStream>* p=m->Get_pageSet();
    _register(new PacketMag(mag,p,_configure,priority[mag]));
  }
  // @todo Add packet sources for subtitles, databroadcast and packet 830
}

Service::~Service()
{
}

void Service::_register(PacketSource *src)
{
  _Sources.push_front(src);
}

int Service::run()
{
  std::cerr << "[Service::worker]This is the worker process" << std::endl;
  uint8_t nmag=1;
  vbit::Mag **mag; // Pointer to magazines array
  vbit::Mag *pMag; // Pointer to the magazine that we are working on
  uint8_t priorityCount[STREAMS]; // Will be handled by the packet sources themselves.

  uint8_t hold[STREAMS]; /// If hold is set then the magazine can not be sent until the next field

  vbit::Packet* pkt=new vbit::Packet(8,25,"                                        ");  // This just allocates storage.

  uint8_t rowCounter=0; // Counts 16 rows to a field

  static vbit::Packet* filler=new vbit::Packet(8,25,"                                        ");  // @todo Again, we should have a pre-prepared quiet packet to avoid eating the heap


    // Initialise the priority counts
    /*
	for (uint8_t i=0;i<STREAMS-1;i++)
	{
		hold[i]=0;
		priorityCount[i]=priority[i];
	}
	*/

	// Get the magazines
	// mag=_pageList->GetMagazines();

  std::cerr << "[Service::worker]Loop starts" << std::endl;
	while(1) // normal
	{
	  // If counters (or other trigger) causes an event then send the events
	  // Iterate through the packet sources
	  //   Check if there is a packet available
	  //   If there is then send it
	  // update the counters.

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
			priorityCount[nmag]=127; // Slow down this empty stream
		}
		else
		{
			if (!hold[nmag]) 		// Not in hold
			{
				pMag->GetPacket(pkt); // YEAH. Don't do this!!!!
				bool isHeader=false;
				if (pkt!=NULL) // How could this be NULL? After the last packet of a page has been sent.
				{
					isHeader=pMag->GetHeaderFlag(); // pkt->IsHeader();

					std::cout.write(pkt->tx(), 42); // Transmit the packet - using cout.write to ensure writing 42 bytes even if it contains a null.

					// Was the last packet a header? If so we need to go into hold
					if (isHeader)
					{
						hold[nmag]=true;
						// std::cerr << "Header wait mode mag=" << (int) nmag << std::endl;
					}
					rowCounter++;
					if (rowCounter>=16)
					{
						rowCounter=0;
						for (uint8_t i=0;i<STREAMS-1;i++) hold[i]=0;	// Any holds are released now
						// Should put packet 8/30 stuff here as in vbit stream.c
					}
				} // not a null packet
				else
				{
					// After a GetPacket returns NULL (after the page runs out of packets)
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
					std::cout << filler->tx();
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
	return 99; // don't really want to return anything
} // worker


