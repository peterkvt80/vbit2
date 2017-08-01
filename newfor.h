/**
 * @file newfor.h
 */
#ifndef _NEWFOR_H_
#define _NEWFOR_H_

#include "ttxpage.h"

//@todo Update these to the VBIT2 equivalents
//#include "buffer.h"
//#include "mag.h"
//#include "hamm.h"

/** The buffer is big enough to contain a head and seven rows.
 * The buffer is in two parts. A buffer control block and the actual data
 
 How does this work?
 Newfor data is accepted over a network connection.
 A Newfor command is used to set the page. For the UK this is usually 888.
 Subtitle data is then sent as a complete block of rows.
 As each row is received it gets converted into a transmission ready packet.
 Each packet gets put into a buffer.
 When OnAir is received, all these packets get put onto the output stream called bufferpacket
 From there it gets picked up by the stream process and out on air
 
 */
 
 
namespace vbit
{
class Newfor
{
  public:
	  Newfor();
		~Newfor();

		/** 
		 * @brief Put the previously loaded subtitle to the previously select page
		 * @param response String message to send back to the client 
		 */ 
		void SubtitleOnair(char* response);		
		int SoftelPageInit(char* cmd);
		/** InitNewfor
		 * Initialise the subtitle buffer
		 */
		void InitNewfor();
		/** 
		 * Clear down subtitles immediately
		 */ 
		void SubtitleOffair();
		

  private:	
	  // Constants
    static const uint8_t SUBTITLEPACKETCOUNT=8;
    /** These are the two responses possible
     * Only Page Init and Subtitle Data should respond with these ASCII codes. 
     */
    static const uint8_t ACK=0x06;
    static const uint8_t NACK=0x15;
		
		// member variables
		TTXPage ttxpage;

		// member functions
		/**
		 * @detail Expect four characters
		 * 1: Ham 0 (always 0x15)
		 * 2: Page number hundreds hammed. 1..8
		 * 3: Page number tens hammed
		 * Although it says HTU, in keeping with the standard we will work in hex.
		 */ 
		char subs[(40+1)*7]; // One byte for row number plus 40 bytes, seven times.

// instead of this we populate a ttxpage onject
// extern bufferpacket packetCache[1]; // Commands are read into here, and transferred out when OnAir 


/**
 * Start of a Subtitle Data command
 * @return Row count 1..7, or 0 if invalid
 */
int GetRowCount(char* cmd);

/**
 * Creates a packet of of Newfor data.
 */
void saveSubtitleRow(uint8_t mag, uint8_t row,char* cmd);

};

}
#endif
