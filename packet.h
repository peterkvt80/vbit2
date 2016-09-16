#ifndef _PACKET_H_
#define _PACKET_H_

#include <stdint.h>
#include "iostream"
#include "iomanip"
#include <string.h>
#include "tables.h"

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
        /** Access _packet[45]
         * \return The current value of _packet[45]
         */
        inline char* Get_packet() { return _packet; }
        /** Set _packet
         * \param *val New value to set
         */
        void Set_packet(char *val);

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
         * The endian depends on the hardware attached.
         * Sometimes we need to reverse the bit order
         *
         */
        std::string tx(bool debugMode);

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

    protected:
    private:
        char _packet[45]; //!< Member variable "_packet[45]"
};

}

#endif // _PACKET_H_
