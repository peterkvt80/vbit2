#include "Packet.h"

using namespace vbit;

Packet::Packet()
{
    //ctor
}

Packet::~Packet()
{
    //dtor
}

void Packet::Set_packet(char *val)
{
    std::cout << "[Packet::Set_packet] todo. Implement copy" << std::endl;
    _packet[0]=val[0]; // Placeholder. todo
}

// Clear entire packet to 0
void Packet::PacketQuiet()
{
	uint8_t i;
	for (i=0;i<PACKETSIZE;i++)
		_packet[i]=0;
}

// Set CRI and MRAG. Leave the rest of the packet alone
void Packet::SetMRAG(uint8_t mag, uint8_t row)
{
	char *p=_packet; // Remember that the bit order gets reversed later
	*p++=0x55; // clock run in
	*p++=0x55; // clock run in
	*p++=0x27; // framing code
	// add MRAG
	*p++=HamTab[mag%8+((row%2)<<3)]; // mag + bit 3 is the lowest bit of row
	*p++=HamTab[((row>>1)&0x0f)];
} // SetMRAG
