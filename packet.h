#ifndef _PACKET_H_
#define _PACKET_H_

#include <cstdint>
#include <iostream>
#include <vector>
#include <iomanip>
#include <cstring>
#include <ctime>
#include "tables.h"
#include "hamm-tables.h"
#include <cassert>
#include "ttxpage.h"

/**
 * Teletext packet.
 * Deals with teletext packets including header, text rows, enhanced packets, service packets and parity
 */

#define PACKETSIZE 45
namespace vbit
{

class Packet
{
    public:
        /** Default constructor */
        Packet();
        /** char constructor */
        Packet(char *val);
        /** string constructor */
        Packet(std::string val);

        /** row constructor */
        Packet(int mag, int row, std::string val);

        /** Default destructor */
        virtual ~Packet();

        inline char* Get_packet() { return _packet; }
        /** Set _packet
         * \param *val New value to set
         */
        void Set_packet(char *val);
        
        // this is a raw copy of 40 bytes for pre-formatted packets. use with caution!
        void SetPacketRaw(char *val);

        /** SetPacketText
         * Copy the supplied text into the text part of the packet (last 40 bytes)
         * \param val New value to set
         */
        void SetPacketText(std::string val);

        /** PacketQuiet
         * Make this packet all 0
         */
        void PacketQuiet();

        /** tx
         * @return Packet in transmission format.
         * There are one or two things that can affect the format.
         * We create transmission ready packets of 45 bytes.
         * raspi-teletext does not use the clock run and framing code in so we skip the first three bytes.
         */
        char* tx(time_t t);

        /** SetMRAG
         * Sets the first five bytes of the packet
         * Namely Two clock run in, one framing code, and two for magazine/row address group
         * CRI|CRI|FC|MRAG
         */
        void SetMRAG(uint8_t mag, uint8_t row);

        /** Header
         * Sets everything except the caption
         * @param mag 0..7 (where 0 is mag 8)
         * @param page number 00..ff
         * @param subcode (16 bit hex code as in tti file)
         * @param control C bits
         */
        void Header(unsigned char mag, unsigned char page, unsigned int subcode, unsigned int control);

        /** HeaderText
         * Sets last 32 bytes. This is the caption part
         * @param val String of exactly 32 characters.
         */
        void HeaderText(std::string val);

        /** Parity
         * Sets the parity of the bytes starting from offset
         * @param offset 5 (default) for normal text rows, 13 for headers
         */
        void Parity(uint8_t offset=5);

        /** IsHeader
         * Transmission rule: After a header packet, wait at least one field before transmitting rows.
         * @return true if the last packet out was a header packet.
         */
				bool IsHeader(){return _isHeader;};

        /** Create a Fastext packet
         * Requires a list of six links
				 * @param links Array of six link values (0x100 to 0x8FF)
				 * @param mag - Magazine number
         *
         */
				void Fastext(int* links, int mag);

        /*  Independent data line format A 
            returns the number of payload bytes which were transmitted
        */
        enum IDLAFormatFlags : uint8_t
        {
            IDLA_NONE   = 0x0,
            IDLA_RI     = 0x2,
            IDLA_CI     = 0x4,
            IDLA_DL     = 0x8
        };
        
        int IDLA(uint8_t datachannel, uint8_t flags, uint8_t ial, uint32_t spa, uint8_t ri, uint8_t ci, std::vector<uint8_t> data);
        
				/**
				 * @return The current row number
				 */
				int GetRow(){return _row;};

				/**
				 * @return The current page number
				 */
				int GetPage(){return _page;};

				/**
				 * @brief Same as the row constructor, except it doesn't construct
				 * @param mag - Magazine number 0..7 where 0 is magazine 8
				 * @param row - Row 0..31
				 * @param val - The contents of the row text (40 characters)
				 * @param coding -
				 */
				void SetRow(int mag, int row, std::string val, PageCoding coding);

				void Dump();

    protected:
    
    private:
        char _packet[46]; // 45 byte packet with room for a null terminator
        bool _isHeader; //<! True if the packet is a header
        uint8_t _mag;//<! The magazine number this packet belongs to 0..7 where 0 is maazine 8
        uint32_t _page;//<! The page number this packet belongs to 00 to ff
        uint8_t _row; //<! Row number 0 to 31
        PageCoding _coding; // packet coding

        void IDLcrc(uint16_t *crc, uint8_t data); // calculate a CRC checksum for one byte
        void ReverseCRC(uint16_t *crc, uint8_t byte);

        bool get_offset_time(time_t t, char* str);
        bool get_net(char* str);
        bool get_time(time_t t, char* str);
        /** Hamming 24/18
         * The incoming triplet should be packed 18 bits of an int 32 representing D1..D18
         * The int is repacked with parity bits
         */
        void SetTriplet(int ix, int triplet);

        /**
         * @ingroup Error
         * @param p A Hamming 24/18 protected 24 bit word will be stored here,
         *   last significant byte first, lsb first transmitted.
         * @param c Integer between 0 ... 1 << 18 - 1.
         *
         * Encodes an 18 bit word with Hamming 24/18 protection
         * as specified in ETS 300 706, Section 8.3.
         *
         * @since 0.2.27
         */
        void vbi_ham24p(uint8_t *p, unsigned int c);
        
        void stringToBytes(char *ptr, char *cstr, unsigned int len);
        
        #ifdef RASPBIAN
        bool get_temp(char* str);
        #endif
};

}

#endif // _PACKET_H_
