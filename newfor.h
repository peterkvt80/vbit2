/**
 * @file newfor.h
 */
#ifndef _NEWFOR_H_
#define _NEWFOR_H_
#include "ttxpage.h"
#include "packetsubtitle.h"

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
	  Newfor(PacketSubtitle* subtitle=nullptr);
		~Newfor();

		/**
		 * @brief Put the previously loaded subtitle to the previously select page
		 * @param response String message to send back to the client
		 */
		void SubtitleOnair(char* response);
		/**
		 * @detail Expect four characters
		 * 1: Ham 0 (always 0x15)
		 * 2: Page number hundreds hammed. 1..8
		 * 3: Page number tens hammed
		 * Although it says HTU, in keeping with the standard we will work in hex.
		 */
		int SoftelPageInit(char* cmd);

		/** InitNewfor
		 * Initialise the subtitle buffer
		 */
		void InitNewfor();

		/**
		 * Clear down subtitles immediately
		 */
		void SubtitleOffair();

    /**
	  * Start of a Subtitle Data command
	  * @return Row count 1..7, or 0 if invalid
	  */
	  int GetRowCount(char* cmd);

		/**
		 * Creates a packet of Newfor data.
		 */
		void saveSubtitleRow(uint8_t mag, uint8_t row, char* cmd);

  private:
		// member variables
		TTXPage ttxpage_;
    uint8_t rowcount_; /// Number of rows in this subtitle
    PacketSubtitle* subtitle_;
    
		// member functions
};

}
#endif
