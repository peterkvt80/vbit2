#ifndef _PACKET_H_
#define _PACKET_H_

#include <stdint.h>
#include "iostream"

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

        /** PacketQuiet
         * Make this packet all 0
         */
        void PacketQuiet();

        /** SetMRAG
         * Sets the first five bytes of the packet
         * Namely Two clock run in, one framing code, and two for magazine/row address group
         * CRI|CRI|FC|MRAG
         */
        void SetMRAG(uint8_t mag, uint8_t row);

    protected:
    private:
        char _packet[45]; //!< Member variable "_packet[45]"
};

}

#endif // _PACKET_H_
