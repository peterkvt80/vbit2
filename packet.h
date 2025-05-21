#ifndef _PACKET_H_
#define _PACKET_H_

#include <cstdint>
#include <algorithm>
#include <iostream>
#include <vector>
#include <array>
#include <iomanip>
#include <cstring>
#include <ctime>
#include "tables.h"
#include <cassert>
#include "page.h"
#include "masterClock.h"

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
            /** row constructor */
            Packet(int mag, int row, std::string val);

            /** Default destructor */
            virtual ~Packet();

            inline std::array<uint8_t, PACKETSIZE> Get_packet() { return _packet; }
            
            /** SetPacketRaw
             * Copy the supplied raw binary data into the last 40 bytes of packet
             * \param data New 40 byte binary packet data
             */
            void SetPacketRaw(std::vector<uint8_t> data);

            /** SetPacketText
             * Copy the supplied text into the text part of the packet (last 40 bytes)
             * \param val New 40 character text string
             */
            void SetPacketText(std::string val);
            
            /** tx
             * @return pointer to packet data vector
             * We create transmission ready packets of 45 bytes.
             */
            std::array<uint8_t, PACKETSIZE>* tx();

            /** SetMRAG
             * Sets the first five bytes of the packet
             * Namely Two clock run in, one framing code, and two for magazine/row address group
             * CRI|CRI|FC|MRAG
             */
            void SetMRAG(uint8_t mag, uint8_t row);

            /** Header
             * @param mag 0..7 (where 0 is mag 8)
             * @param page number 00..ff
             * @param subcode (16 bit hex code as in tti file)
             * @param control C bits
             * @param text header template
             */
            void Header(uint8_t mag, uint8_t page, uint16_t subcode, uint16_t control, std::string text);

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
            void Fastext(std::array<FastextLink, 6>links, int mag);
            
            enum IDLAFormatFlags : uint8_t
            {
                IDLA_NONE   = 0x0,
                IDLA_RI     = 0x2,
                IDLA_CI     = 0x4,
                IDLA_DL     = 0x8
            };
            
            /**
             * Independent data line format A, returns the number of payload bytes which were transmitted.
             * @param datachannel - channel from 0-15 per EN 300 708 6.4.2
             * @param flags - controls which options are in use
             * @param ial - Interpretation and Address Length
             * @param spa - Service Packet Address (0 to 24 bits)
             * @param ri - Repeat Indicator
             * @param ci - Continuity Indicator
             * @param data - payload
             */
            int IDLA(uint8_t datachannel, uint8_t flags, uint8_t ial, uint32_t spa, uint8_t ri, uint8_t ci, std::vector<uint8_t> data);
            
            /**
             * @brief Same as the row constructor, except it doesn't construct
             * @param mag - Magazine number 0..7 where 0 is magazine 8
             * @param row - Row 0..31
             * @param val - The contents of the row text (40 characters)
             * @param coding -
             */
            void SetRow(int mag, int row, std::string val, PageCoding coding);
            
            /** PacketCRC
             * Set the 16 byte CRC in X/27/0 packets
             * @param crc intial crc value
             */
            void SetX27CRC(uint16_t crc);
            
            /** PacketCRC
             * @return result of applying teletext page CRC to packet
             * @param crc intial crc value
             */
            uint16_t PacketCRC(uint16_t crc);

        protected:
        
        private:
            std::array<uint8_t, PACKETSIZE> _packet; // 45 byte packet
            bool _isHeader; //<! True if the packet is a header
            uint8_t _mag;//<! The magazine number this packet belongs to 0..7 where 0 is maazine 8
            uint8_t _row; //<! Row number 0 to 31
            PageCoding _coding; // packet coding
            
            int GetOffsetOfSubstition(std::string string);
            
            void IDLcrc(uint16_t *crc, uint8_t data); // calculate a CRC checksum for one byte
            void ReverseCRC(uint16_t *crc, uint8_t byte);

            bool get_offset_time(time_t t, uint8_t* str);
            bool get_net(char* str);
            
            /** Hamming 24/18
             * Hamming 24/18 encode a triplet and place at appropriate index in packet
             * The incoming triplet should be packed 18 bits of an uint32_t representing D1..D18
             * The triplet is repacked with parity bits
             */
            void Hamming24EncodeTriplet(uint8_t index, uint32_t triplet);
            
            void PageCRC(uint16_t *crc, uint8_t byte); // calculate a CRC checksum for one byte
            
            #ifdef RASPBIAN
            bool get_temp(char* str);
            #endif
    };
}

#endif // _PACKET_H_
