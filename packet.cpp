#include "Packet.h"


using namespace vbit;

Packet::Packet()
{
    //ctor
}

Packet::Packet(char *val)
{
    //ctor
    strncpy(_packet,val,45+1);
}

Packet::Packet(std::string val)
{
    //ctor
    strncpy(_packet,val.c_str(),45+1);
}

Packet::Packet(int mag, int row, std::string val)
{
    SetMRAG(mag, row);
    SetPacketText(val);
}

Packet::~Packet()
{
    //dtor
    strcpy(_packet,"This packet constructed with default txt");
}

void Packet::Set_packet(char *val)
{
    std::cout << "[Packet::Set_packet] todo. Implement copy" << std::endl;
    strncpy(&_packet[5],val,45+1);
}

void Packet::SetPacketText(std::string val)
{
    std::cout << "[Packet::SetPacketText] todo. Implement copy" << std::endl;
    strncpy(&_packet[5],val.c_str(),45+1);
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

std::string Packet::tx(bool debugMode=false)
{
    // @TODO: parity
    if (!debugMode)
    {
        // @todo drop off the first three characters for raspi-teletext
        return _packet;
    }
    else
    {
        std::cerr << std::setfill('0') <<  std::hex ;
        for (int i=0;i<45;i++)
            std::cerr << std::setw(2)  << (int)(_packet[i] & 0x7f) << " ";
        std::cerr << std::dec << std::endl;
        for (int i=0;i<45;i++)
        {
            char ch=_packet[i] & 0x7f;
            if (ch<' ') ch='.';
            std::cerr << " " << ch << " ";
        }
        std::cerr << std::endl;
        return &_packet[3];
    }

}

/** A header has mag, row=0, page, flags, caption and time
 */
void Packet::Header(unsigned char mag, unsigned char page, unsigned int subcode, unsigned int control)
{
	uint8_t cbit;
	SetMRAG(mag,0);
	_packet[5]=HamTab[page%0x10];
	_packet[6]=HamTab[page/0x10];
	_packet[7]=HamTab[(subcode&0x0f)]; // S1
	subcode>>=4;
	// Map the page settings control bits from MiniTED to actual teletext packet.
	// To find the MiniTED settings look at the tti format document.
	// To find the target bit position these are in reverse order to tx and not hammed.
	// So for each bit in ETSI document, just divide the bit number by 2 to find the target location.
	// Where ETSI says bit 8,6,4,2 this maps to 4,3,2,1 (where the bits are numbered 1 to 8)
	cbit=0;
	if (control & 0x4000) cbit=0x08;	// C4 Erase page
	_packet[8]=HamTab[(subcode&0x07) | cbit]; // S2 add C4
	subcode>>=3;
	_packet[9]=HamTab[(subcode&0x0f)]; // S3
	subcode>>=4;
	cbit=0;
	// Not sure if these bits are reversed. C5 and C6 are indistinguishable
	if (control & 0x0002) cbit=0x08;	// C6 Subtitle
	if (control & 0x0001) cbit|=0x04;	// C5 Newsflash

	// cbit|=0x08; // TEMPORARY!
 	_packet[10]=HamTab[(subcode&0x03) | cbit]; // S4 C6, C5
	cbit=0;
	if (control & 0x0004)  cbit=0x01;	// C7 Suppress Header TODO: Check if these should be reverse order
	if (control & 0x0008) cbit|=0x02;	// C8 Update
	if (control & 0x0010) cbit|=0x04;	// C9 Interrupted sequence
	if (control & 0x0020) cbit|=0x08;	// C10 Inhibit display

	_packet[11]=HamTab[cbit]; // C7 to C10
	cbit=(control & 0x0380) >> 6;	// Shift the language bits C12,C13,C14. TODO: Check if C12/C14 need swapping. CHECKED OK.
	if (control & 0x0040) cbit|=0x01;	// C11 serial/parallel
	_packet[12]=HamTab[cbit]; // C11 to C14 (C11=0 is parallel, C12,C13,C14 language)
} // Header

