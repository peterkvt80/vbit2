/** Newfor.cpp */

/** @file newfor.c
 * @brief Softel Newfor subtitles for VBIT2.
 * A TCP port 5570 on VBIT accepts newfor subtitling commands.
 * The main purpose is to play out pre-recorded subtititles.
 * You can also play out subtitles from a video server or
 * bridged from another subtitles source such as through a DTP600.
 * or accept any industry standard Newfor source.
 */

#include "newfor.h"

// @todo These will be converted to member variables
// bufferpacket packetCache[1]; // Incoming rows are cached here, and transferred to subtitleBuffer when OnAir

// Packet Subtitles are stored here

// uint8_t subtitleCache [SUBTITLEPACKETCOUNT][PACKETSIZE];	/// Storage for 8 packets, 45 bytes per packet. (for packetCache)

static uint8_t _page;	/// Page number (in hex). This is set by Page Init
static uint8_t _rowcount; /// Number of rows in this subtitle
// static char packet[PACKETSIZE];

using namespace vbit;

// This is not so pretty. @todo Factor this out of TCPClient
static int
vbi_unham8			(unsigned int		c)
{
	return _vbi_hamm8_inv[(uint8_t) c];
}


Newfor::Newfor(PacketSubtitle* subtitle) :
  _subtitle(subtitle)
{
		ttxpage.SetSubCode(0);
}

Newfor::~Newfor()
{
}

/** initNu4
 * @detail Initialise the buffers used by Newfor
 */
//void InitNu4()
//{
//	bufferInit(packetCache   ,(char*)subtitleCache ,SUBTITLEPACKETCOUNT);
//}


/**
 * @detail Expect four characters
 * 1: Ham 0 (always 0x15)
 * 2: Page number hundreds hammed. 1..8
 * 3: Page number tens hammed
 * Although it says HTU, in keeping with the standard we will work in hex.
 * @return The decoded page number
 */
int Newfor::SoftelPageInit(char* cmd)
{
  int page=0;
  int n;


  // Useless leading 0
  n=vbi_unham8(cmd[1]);
  if (n<0) return 0x900; // @todo This is an error.
  // Hundreds
  n=vbi_unham8(cmd[2]);
  if (n<1 || n>8) return 0x901;
  page=n*0x100;
  // tens (hex allowed!)
  n=vbi_unham8(cmd[3]);
  if (n<0 || n>0x0f) return 0x902;
  page+=n*0x10;
  // units
  n=vbi_unham8(cmd[4]);
  if (n<0 || n>0x0f) return 0x903;
  page+=n;
  _page=page;
	ttxpage.SetPageNumber(page*0x100);
  return page;
}

void Newfor::SubtitleOnair(char* response)
{
  // std::cerr << "[Newfor::SubtitleOnair] page=" << std::hex << ttxpage.GetPageNumber() << std::dec << std::endl;

	// OLD CODE
	// What page is the subtitle on? THIS IS IN THE WRONG PLACE! Ideally we will already have a header and placed it in the cache
	//uint8_t mag;
	//uint8_t page;
	//mag=(_page/0xff) & 0x07;
	//page=_page & 0xff;
	// First packet needs to be the header. Could well want suppress header too.
	// sprintf(response,"[SubtitleOnair]mag=%1x page=%02x",mag,page);
	// PacketHeader(packet, mag, page, 0, 0x0002, "test");
	//PacketHeader(packet, mag, page, 0, 0x4002); // Dummy
	// dumpPacket(packet);
	//bufferPut(&magBuffer[8],packet);
	// fprintf(stderr,response);

	//while (!bufferIsEmpty(packetCache))
	//{
//		bufferMove(&magBuffer[8],packetCache);
//	}
	//  \OLD CODE

	strcpy(response,"Response not implemented, sorry\n");

	// At this point ttxpage is ready to go.
	// Somehow we need to get it to packetsubtitle in the service thread.
	// 1. Which object is managing the subtitle?

	// 2. Send the page to this object
  _subtitle->SendSubtitle(&ttxpage);

	// After we have displayed the subs we could save them for retransmission
	// but instead we will clear out everything.
	for (int i=0;i<24;i++)
	{
			ttxpage.SetRow(i,"");
	}

}

void Newfor::SubtitleOffair()
{
	std::cerr << "[Newfor;:SubtitleOffair]" << std::endl;
// Construct a header for _page with the erase flag set.
//
/* OLD CODE
	uint8_t mag;
	uint8_t page;
	mag=(_page/0xff) & 0x07;
	page=_page & 0xff;
	// Control bits are erase+subtitle
	PacketHeader(packet, mag, page, 0, 0x4002);
	Parity(packet,13);
	// May want to clear subtitle buffer at the same time
	bufferPut(&magBuffer[8],packet);


	\OLD CODE  */

	// Set the page status
	ttxpage.SetPageStatus(0x4002);
	// clear all the lines
	// Somehow send the page to be transmitted (in adaptive mode where blank lines are elided)

}

/**
 * @return Row count 1..7, or 0 if invalid
 * @param cmd - Need some splainin
 */
int Newfor::GetRowCount(char* cmd)
{
  int n=vbi_unham8(cmd[1]);
  if (n>7)
	n=0;
  _rowcount=n;
  return n;
}

/**
 * @brief Decode the rowaddress and row packet part of a Type 2 message.
 * @param mag - This is irrelevant as mag is set up in a Type 1 message
 * @param row - The row number on which to display the subtitle
 * @param cmd - The row text
 * @brief Save a row of data when a Newfor command is completed
 */
void Newfor::saveSubtitleRow(uint8_t mag, uint8_t row, char* cmd)
{
	/* OLD CODE
	char packet[PACKETSIZE]; // A packet for us to fill
	PacketPrefix((uint8_t*)packet, mag, row);
	// copy from cmd to the packet
	strncpy(packet+5,cmd,40);
	// fix the parity
	Parity(packet,5);
	// stuff it in the buffer
	bufferPut(packetCache,packet); // buffer packets locally
	\OLD CODE */

	// What @todo about the mag?
	// std::cerr << "[Newfor::saveSubtitleRow] cmd=" << cmd << std::endl;
	ttxpage.SetRow(row, cmd);
}
