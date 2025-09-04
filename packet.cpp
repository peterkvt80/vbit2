#include "packet.h"
#include "version.h"

using namespace vbit;

Packet::Packet(int mag, int row) : _isHeader(false), _coding(CODING_7BIT_TEXT)
{
    //ctor
    _packet.fill(0x20); // fill with spaces
    SetMRAG(mag, row); // overwrite front 5 bytes
    assert(_row!=0); // Use Header for row 0
}

Packet::~Packet()
{
    //dtor
}

void Packet::SetRow(int mag, int row, std::array<uint8_t, 40> val, PageCoding coding)
{
    SetMRAG(mag, row);
    
    _isHeader=false; // Because it can't be a header
    std::copy(val.begin(), val.end(), _packet.begin() + 5);
    
    _coding = coding;
    
    switch(coding)
    {
        case CODING_PER_PACKET:
        {
            _coding = Page::ReturnPageCoding(_packet[5] & 0xF); // set packet coding based on first byte of packet
            /* fallthrough */
            [[gnu::fallthrough]];
        }
        case CODING_13_TRIPLETS:
        case CODING_HAMMING_8_4:
        case CODING_HAMMING_7BIT_GROUPS:
        {
            _packet[5] = Hamming8EncodeTable[_packet[5] & 0x0F]; // first byte is hamming 8/4 coded
            break;
        }
        case CODING_7BIT_TEXT:
        {
            _packet[5] = OddParityTable[_packet[5] & 0x7f]; // set parity on first byte
        }
        default:
        {
            break;
        }
    }
    
    switch(_coding)
    {
        default: // treat an invalid coding as 7-bit text
        case CODING_7BIT_TEXT:
        {
            // Perform substitution of version number string
            // %%%%%V version number eg. v2.0.0
            int off = Packet::GetOffsetOfSubstition("%%%%%V");
            if (off > -1)
            {
                // allow this substitution to overflow the template
                int len = strlen(VBIT2_VERSION);
                if (off + len > 45)
                    len = 45 - off; // but clamp to the end of the packet
                std::copy_n(VBIT2_VERSION,len,_packet.begin() + off);
                if (len < 6)
                {
                    for (int i = len; i < 6; i++)
                        _packet[off+i] = 0x20; // insert spaces if version string is too short
                }
            }
            
            // first byte parity already set by first switch statement
            Parity(6);
            break;
        }
        case CODING_13_TRIPLETS:
        {
            // Special handler to allow stuffing enhancement packets in as OL rows
            // Each 18 bits of data for a triplet is coded in the input line as
            // three bytes least significant first where each byte contains 6 data
            // bits in b0-b5.
            // designation code is 8/4 hamming coded by first switch statement
            /* 0x0a and 0x00 in the hammed output is causing a problem so disable this until they are fixed (output will be gibberish) */
            int triplet;
            for (int i = 1; i<=13; i++)
            {
                triplet = _packet[i*3+3] & 0x3F;
                triplet |= (_packet[i*3+4] & 0x3F) << 6;
                triplet |= (_packet[i*3+5] & 0x3F) << 12;
                Hamming24EncodeTriplet(i, triplet);
            }
            break;
        }
        case CODING_HAMMING_8_4:
        {
            // first byte already hamming 8/4 coded by first switch statement
            for (int i = 1; i<40; i++)
            {
                _packet[5+i] = Hamming8EncodeTable[_packet[5+i] & 0x0F];
            }
            break;
        }
        case CODING_HAMMING_7BIT_GROUPS:
        {
            // first byte already hamming 8/4 coded by first switch statement
            for (int i = 1; i<8; i++)
            {
                _packet[5+i] = Hamming8EncodeTable[_packet[5+i] & 0x0F];
            }
            for (int i = 8; i<20; i++)
            {
                _packet[5+i] = OddParityTable[(uint8_t)(_packet[5+i]&0x7f)];
            }
            for (int i = 20; i<28; i++)
            {
                _packet[5+i] = Hamming8EncodeTable[_packet[5+i] & 0x0F];
            }
            for (int i = 28; i<40; i++)
            {
                _packet[5+i] = OddParityTable[(uint8_t)(_packet[5+i]&0x7f)];
            }
            break;
        }
        case CODING_8BIT_DATA:
        {
            // do nothing to 8-bit data
            break;
        }
    }
}

void Packet::SetX27CRC(uint16_t crc)
{
    if (Hamming8DecodeTable[_packet[5]] == 0) // only set CRC bytes for packet X/27/0
    {
        _packet[43]=crc >> 8;
        _packet[44]=crc & 0xFF;
    }
}

void Packet::SetPacketRaw(std::vector<uint8_t> data)
{
    data.resize(40, 0x00); // ensure correct length
    std::copy(data.begin(), data.end(), _packet.begin() + 5);
    _coding = CODING_8BIT_DATA; // don't allow this to be re-processed with parity etc
}

// Set CRI and MRAG. Leave the rest of the packet alone
void Packet::SetMRAG(uint8_t mag, uint8_t row)
{
    _packet[0]=0x55; // clock run in
    _packet[1]=0x55; // clock run in
    _packet[2]=0x27; // framing code
    
    _packet[3]=Hamming8EncodeTable[mag%8+((row%2)<<3)]; // mag + bit 3 is the lowest bit of row
    _packet[4]=Hamming8EncodeTable[((row>>1)&0x0f)];
    _isHeader=row==0;
    _row=row;
    _mag=mag;
}

/** get_offset_time.
 * Given a parameter of say %t+02
 * where str[2] is + or -
 * str[4:3] is a two digit of half hour offsets from UTC
 * @return local time at offset from UTC
 */
bool Packet::get_offset_time(time_t t, uint8_t* str)
{
    char strTime[6];
    time_t rawtime = t;
    struct tm *tmGMT;

    // What is our offset in seconds?
    int offset=((str[3]-'0')*10+str[4]-'0')*30*60; // @todo We really ought to validate this

    // Is it negative (west of us?)
    if (str[2]=='-')
        offset=-offset;
    else
        if (str[2]!='+') return false; // Must be + or -

    // Add the offset to the time value
    rawtime+=offset;

    tmGMT = gmtime(&rawtime);

    strftime(strTime, 21, "%H:%M", tmGMT);
    std::copy_n(strTime,5,str);
    return true;
}

int Packet::GetOffsetOfSubstition(std::string string)
{
    auto it = std::search(_packet.begin()+1, _packet.end(), string.begin(), string.end());
    if (it != _packet.end())
        return std::distance(_packet.begin(), it);
    else
        return -1;
}

/* Perform translations on packet.
 * return pointer to 45 byte packet data vector
 *
 * *** Any substitutions applied by this function will break the checksum that has already been calculated and broadcast ***
 */
std::array<uint8_t, PACKETSIZE>* Packet::tx()
{
    // get master clock singleton
    MasterClock *mc = mc->Instance();
    time_t t = mc->GetMasterClock().seconds;
    
    // Get local time
    struct tm * timeinfo;
    timeinfo=localtime(&t);
    
    char tmpstr[21];
    int off;
    
    if (_isHeader)
    {
        // substitutions already done in HeaderText
    }
    else if (_row < 26 && _coding == CODING_7BIT_TEXT) // Other text rows
    {
        for (int i=5;i<45;i++) _packet[i] &= 0x7f; // strip parity bits off
        // ======= TEMPERATURE ========
        off = Packet::GetOffsetOfSubstition("%%%T");
        
        if (off > -1)
        {
            #ifdef RASPBIAN
            get_temp(tmpstr);
            std::copy_n(tmpstr,4,_packet.begin() + off);
            #else
            std::copy_n("err ",4,_packet.begin() + off);
            #endif
        }

        // ======= WORLD TIME ========
        // Special case for world time. Put %t<+|-><hh> to get local time HH:MM offset by +/- half hours
        for (;;)
        {
            off = Packet::GetOffsetOfSubstition("%t+");
            if (off == -1)
            {
                off = Packet::GetOffsetOfSubstition("%t-");
            }
            if (off > -1)
            {
                //std::cout << "[test 1]" << _packet << std::endl;
                get_offset_time(t, _packet.data() + off); // TODO: something with return value
                //exit(4);
            }
            else
                break;
        }
        // ======= NETWORK ========
        // Special case for network address. Put %%%%%%%%%%%%%%n to get network address in form xxx.yyy.zzz.aaa with trailing spaces (15 characters total)
        off = Packet::GetOffsetOfSubstition("%%%%%%%%%%%%%%n");
        if (off > -1)
        {
            #ifndef WIN32
            get_net(tmpstr);
            std::copy_n(tmpstr,15,_packet.begin() + off);
            #else
            std::copy_n("not implemented",15,_packet.begin() + off);
            #endif
        }
        // ======= TIME AND DATE ========
        // Special case for system time. Put %%%%%%%%%%%%timedate to get time and date
        off = Packet::GetOffsetOfSubstition("%%%%%%%%%%%%timedate");
        if (off > -1)
        {
            int num = strftime(tmpstr, 21, "\x02%a %d %b\x03%H:%M/%S", timeinfo);
            std::copy_n(tmpstr,num,_packet.begin() + off);
        }
        
        Parity(5); // redo the parity because substitutions will need processing
    }
    
    return &_packet;
}

/** A header has mag, row=0, page, flags, caption and time
 */
void Packet::Header(uint8_t mag, uint8_t page, uint16_t subcode, uint16_t control, std::string text)
{
    uint8_t cbit;
    SetMRAG(mag,0);
    _packet[5]=Hamming8EncodeTable[page%0x10];
    _packet[6]=Hamming8EncodeTable[page/0x10];
    _packet[7]=Hamming8EncodeTable[(subcode&0x0f)];         // S1 four bits
    subcode>>=4;
    // Map the page settings control bits from MiniTED to actual teletext packet.
    // To find the MiniTED settings look at the tti format document.
    // To find the target bit position these are in reverse order to tx and not hammed.
    // So for each bit in ETSI document, just divide the bit number by 2 to find the target location.
    // Where ETSI says bit 8,6,4,2 this maps to 4,3,2,1 (where the bits are numbered 1 to 8)
    cbit=0;
    if (control & 0x4000) cbit=0x08;                        // C4 Erase page
    _packet[8]=Hamming8EncodeTable[(subcode&0x07) | cbit];  // S2 (3 bits) add C4
    subcode>>=4;
    _packet[9]=Hamming8EncodeTable[(subcode&0x0f)];         // S3 four bits
    subcode>>=4;
    
    cbit=0;
    if (control & 0x0001) cbit=0x04;                        // C5 Newsflash
    if (control & 0x0002) cbit|=0x08;                       // C6 Subtitle
    _packet[10]=Hamming8EncodeTable[(subcode&0x03) | cbit]; // S4 C6, C5
    
    cbit=0;
    if (control & 0x0004)  cbit=0x01;                       // C7 Suppress Header
    if (control & 0x0008) cbit|=0x02;                       // C8 Update
    if (control & 0x0010) cbit|=0x04;                       // C9 Interrupted sequence
    if (control & 0x0020) cbit|=0x08;                       // C10 Inhibit display
    _packet[11]=Hamming8EncodeTable[cbit];                  // C7 to C10
    
    cbit=(control & 0x0380) >> 6;                           // Shift the language bits C12,C13,C14.
    
    // if (control & 0x0040) cbit|=0x01;                    // C11 serial/parallel *** We only work in parallel mode, Serial would mean a different packet ordering.
    _packet[12]=Hamming8EncodeTable[cbit];                  // C11 to C14 (C11=0 is parallel, C12,C13,C14 language)

    _isHeader=true; // Because it must be a header
    text.resize(32);
    std::copy_n(text.begin(),32,_packet.begin() + 13);
    
    // perform the header template substitutions for page number, date, etc.
    
    // get master clock singleton
    MasterClock *mc = mc->Instance();
    time_t t = mc->GetMasterClock().seconds;
    
    // Get local time
    struct tm * timeinfo;
    timeinfo=localtime(&t);
    
    char tmpstr[4];
    int off;
    
    // mpp page number - %%#
    off = Packet::GetOffsetOfSubstition("%%#");
    if (off > -1)
    {
        if (_mag==0)
            _packet[off]='8';
        else
            _packet[off]=_mag+'0';
        _packet[off+1]=page/0x10+'0';
        if (_packet[off+1]>'9')
            _packet[off+1]=_packet[off+1]-'0'-10+'A'; // Particularly poor hex conversion algorithm

        _packet[off+2]=page%0x10+'0';
        if (_packet[off+2]>'9')
            _packet[off+2]=_packet[off+2]-'0'-10+'A'; // Particularly poor hex conversion algorithm
    }
    
    // day name - %%a
    off = Packet::GetOffsetOfSubstition("%%a");
    if (off > -1)
    {
        int num = strftime(tmpstr,4,"%a",timeinfo);
        if (num){
            _packet[off]=tmpstr[0];
            _packet[off+1]=(num > 1)?tmpstr[1]:' ';
            _packet[off+2]=(num > 2)?tmpstr[2]:' ';
        }
    }

    // month name - %%b
    off = Packet::GetOffsetOfSubstition("%%b");
    if (off > -1)
    {
        int num = strftime(tmpstr,4,"%b",timeinfo);
        if (num){
            _packet[off]=tmpstr[0];
            _packet[off+1]=(num > 1)?tmpstr[1]:' ';
            _packet[off+2]=(num > 2)?tmpstr[2]:' ';
        }
    }
    
    // day of month with leading zero - %d
    off = Packet::GetOffsetOfSubstition("%d");
    if (off > -1)
    {
        strftime(tmpstr,3,"%d",timeinfo);
        _packet[off]=tmpstr[0];
        _packet[off+1]=tmpstr[1];
    }
    
    // day of month with no leading zero - %e
    off = Packet::GetOffsetOfSubstition("%e");
    if (off > -1)
    {
        // windows doesn't support %e so just use %d and blank leading zero
        strftime(tmpstr,3,"%d",timeinfo);
        if (tmpstr[0] == '0')
            _packet[off]=' ';
        else
            _packet[off]=tmpstr[0];
        _packet[off+1]=tmpstr[1];
    }
    
    // month number with leading 0 - %m
    off = Packet::GetOffsetOfSubstition("%m");
    if (off > -1)
    {
        strftime(tmpstr,10,"%m",timeinfo);
        _packet[off]=tmpstr[0];
        _packet[off+1]=tmpstr[1];
    }
    
    // 2 digit year - %y
    off = Packet::GetOffsetOfSubstition("%y");
    if (off > -1)
    {
        strftime(tmpstr,10,"%y",timeinfo);
        _packet[off]=tmpstr[0];
        _packet[off+1]=tmpstr[1];
    }
    
    // hours - %H
    off = Packet::GetOffsetOfSubstition("%H");
    if (off > -1)
    {
        strftime(tmpstr,10,"%H",timeinfo);
        _packet[off]=tmpstr[0];
        _packet[off+1]=tmpstr[1];
    }
    
    // minutes - %M
    off = Packet::GetOffsetOfSubstition("%M");
    if (off > -1)
    {
        strftime(tmpstr,10,"%M",timeinfo);
        _packet[off]=tmpstr[0];
        _packet[off+1]=tmpstr[1];
    }
    
    // seconds - %S
    off = Packet::GetOffsetOfSubstition("%S");
    if (off > -1)
    {
        strftime(tmpstr,10,"%S",timeinfo);
        _packet[off]=tmpstr[0];
        _packet[off+1]=tmpstr[1];
    }
    
    Parity(13); // apply parity to the text of the header
}

/**
 * @brief Set parity bits.
 * \param Offset is normally 5 for text rows, 13 for header
 */
void Packet::Parity(uint8_t offset)
{
    int i;
    //uint8_t c;
    for (i=offset;i<PACKETSIZE;i++)
    {
        _packet[i]=OddParityTable[_packet[i] & 0x7f];
    }
}

void Packet::Fastext(std::array<FastextLink, 6> links, int mag)
{
    uint16_t lp, ls;
    uint8_t p=5;
    
    _packet[p++]=Hamming8EncodeTable[0]; // Designation code 0
    mag&=0x07; // Mask the mag just in case. Keep it valid

    // add the link control byte. This will allow row 24 to show.
    _packet[42]=Hamming8EncodeTable[0x0f];

    // and a blank page CRC - this is set later by Packet::SetX27CRC
    _packet[43]=0x00;
    _packet[44]=0x00;

    // for each of the six links
    for (uint8_t i=0; i<6; i++)
    {
        lp=links[i].page;
        if (lp == 0) lp = 0x8ff; // turn zero into 8FF to be ignored
        ls=links[i].subpage;

        uint8_t m=(lp/0x100 ^ mag); // calculate the relative magazine
        _packet[p++]=Hamming8EncodeTable[lp & 0xF];             // page units
        _packet[p++]=Hamming8EncodeTable[(lp & 0xF0) >> 4];     // page tens
        _packet[p++]=Hamming8EncodeTable[ls & 0xF];             // S1
        _packet[p++]=Hamming8EncodeTable[((m & 1) << 3) | ((ls >> 4) & 0xF)]; // S2 + M1
        _packet[p++]=Hamming8EncodeTable[((ls >> 8) & 0xF)];    // S3
        _packet[p++]=Hamming8EncodeTable[((m & 6) << 1) | ((ls >> 4) & 0x3)]; // S4 + M2, M3
    }
}

int Packet::IDLA(uint8_t datachannel, uint8_t flags, uint8_t ial, uint32_t spa, uint8_t ri, uint8_t ci, std::vector<uint8_t> data)
{
    _coding = CODING_8BIT_DATA; // don't allow this to be re-processed with parity etc
    
    SetMRAG(datachannel & 0x7,((datachannel & 8) >> 3) + 30);
    
    _packet[5]=Hamming8EncodeTable[flags & 0xe]; // Format Type
    _packet[6]=Hamming8EncodeTable[ial&0xf]; // Interpretation and Address Length
    
    uint8_t p = 7;
    
    for (uint8_t i = 0; i < (ial&0x7) && i < 7; i++)
    {
        _packet[p++] = Hamming8EncodeTable[(spa >> (4 * i)) & 0xf]; // variable number of Service Packet Address nibbles
    }
    
    if (flags & IDLA_RI)
        _packet[p++]=ri; // Repeat Indicator
    
    uint8_t startOfCRC = p; // where the scope of CRC begins
    
    int sameCount = 0;
    
    if (flags & IDLA_CI)
    {
        _packet[p++]=ci; // explicit Continuity Indicator
        sameCount = (ci == 0x00 || ci == 0xff) ? 1 : 0;
    }
    
    uint8_t DLoffset = p; // store this position in case it needs to be updated later
    if (flags & IDLA_DL)
    {
        _packet[p++] = 0; // initialise Data Length as zero and update it as we add bytes
        sameCount = 0; // ignore this first zero as any amount of data that would result in a dummy byte would mean DL is > 0
    }
    
    // remaining space is 45 - p - 2 crc bytes
    
    unsigned int bytesSent = 0; // count how much of the payload we fit in packet
    while (p < 43)
    {
        if (bytesSent < data.size())
        {
            _packet[p] = data[bytesSent++];
            if (flags & IDLA_DL)
                _packet[DLoffset]++;
            
            if ((_packet[p] == _packet[p-1]) && ((uint8_t)(_packet[p]) == 0xff || (uint8_t)(_packet[p]) == 0x00))
            {
                sameCount++;
                
                if ((uint8_t)(_packet[p]) == (uint8_t)(_packet[p-1]))
                {
                    if (sameCount >= 7 && p < 42)
                    {
                        sameCount = 0;
                        _packet[++p] = 0xaa; // add a dummy byte
                        
                        if (flags & IDLA_DL)
                            _packet[DLoffset]++;
                    }
                }
            }
            else
                sameCount = 0; // reset the counter for dummy bytes
            
            p++;
        }
        else
        {
            _packet[p++] = 0xaa; // fill unused part of packet with dummy bytes
        }
    }
    
    uint16_t crc = 0;
    
    for (uint8_t i = startOfCRC; i < 43; i++)
    {
        IDLcrc(&crc, _packet[i]); // calculate CRC for user data
    }
    
    if (!(flags & IDLA_CI)) // implicit Continuity Indicator
    {
        // modify the CRC so that both bytes are equal to ci
        
        uint16_t tmpcrc = crc; // stash the crc
        
        crc = (ci << 8) | ci; // initialise crc with ci value in both bytes
        
        ReverseCRC(&crc, tmpcrc>>8); // reverse the crc from desired value with previously calculated crc as the input
        ReverseCRC(&crc, tmpcrc&0xff);
    }
    
    _packet[43]=crc&0xff; // store modified crc in packet
    _packet[44]=crc>>8;
    
    return bytesSent;
}

void Packet::IDLcrc(uint16_t *crc, uint8_t data)
{
    // Perform the IDL A crc
    *crc ^= data;
    
    for (uint8_t i = 0; i < 8; i++)
    {
        *crc = (*crc & 1) ? (*crc >> 1) ^ 0x8940 : (*crc >> 1);
    }
}

void Packet::ReverseCRC(uint16_t *crc, uint8_t byte)
{
    /* reverse the IDL A crc */
    uint8_t bit;
    for (uint8_t i = 0; i < 8; i++)
    {
        bit =  (byte >> (7-i)) & 1;
        *crc = (*crc & 0x8000) ? (((*crc << 1) | bit) ^ 0x1281) : ((*crc << 1) | bit);
    }
}

#ifdef RASPBIAN
/** get_temp
 *  Pinched from raspi-teletext demo.c
 * @return Four character temperature in degrees C eg. "45.7"
 */
bool Packet::get_temp(char* str)
{
    FILE *fp;
    char *pch;
    char tmp[100];

    fp = popen("/usr/bin/vcgencmd measure_temp", "r");
    fgets(tmp, 99, fp);
    pclose(fp);
    pch = strtok (tmp,"=\n");
    pch = strtok (NULL,"=\n");
    strncpy(str,pch,5);
    return true; // @todo
}
#endif

#ifndef WIN32
/** get_net
 *  Pinched from raspi-teletext demo.c
 * @return network address as 20 characters
 * Sample response
 * 3: wlan0    inet 192.168.1.14/24 brd 192.168.1.255 scope global wlan0\       valid_lft forever preferred_lft forever
 */
bool Packet::get_net(char* str)
{
    FILE *fp;
    char *pch;

    int n;
    char temp[100];
    fp = popen("/sbin/ip -o -f inet addr show scope global", "r");
    fgets(temp, 99, fp);
    pclose(fp);
    pch = strtok (temp," \n/");
    for (n=1; n<4; n++)
    {
        pch = strtok (NULL, " \n/");
    }
    // If we don't have a connection established, try not to crash
    if (pch==NULL)
    {
        strcpy(str,"IP address????");
        return false;
    }
    strncpy(str,pch,15);
    return true; // @todo
}
#endif

void Packet::Hamming24EncodeTriplet(uint8_t index, uint32_t triplet)
{
    if (index<1) return;
    
    uint8_t D5_D11;
    uint8_t D12_D18;
    uint8_t P5, P6;
    uint8_t Byte_0;

    Byte_0 = (Hamming24EncodeTable0[(triplet >> 0) & 0xFF] ^ Hamming24EncodeTable1[(triplet >> 8) & 0xFF] ^ Hamming24EncodeTable2[(triplet >> 16) & 0x03]);
    _packet[index*3+3] = Byte_0;

    D5_D11 = (triplet >> 4) & 0x7F;
    D12_D18 = (triplet >> 11) & 0x7F;

    P5 = 0x80 & ~(Hamming24ParityTable[0][D12_D18] << 2);
    _packet[index*3+4] = D5_D11 | P5;

    P6 = 0x80 & ((Hamming24ParityTable[0][Byte_0] ^ Hamming24ParityTable[0][D5_D11]) << 2);
    _packet[index*3+5] = D12_D18 | P6;
}

uint16_t Packet::PacketCRC(uint16_t crc)
{
    int i;
    uint16_t tempcrc = crc;
    
    if (_isHeader)
    {
        for (i=13; i<37; i++)
            PageCRC(&tempcrc, _packet[i]); // calculate CRC for header text
    }
    else if (_row < 26)
    {
        for (i=5; i<45; i++)
            PageCRC(&tempcrc, _packet[i]); // calculate CRC for text rows
    }
    
    return tempcrc;
}

void Packet::PageCRC(uint16_t *crc, uint8_t byte)
{
    // perform the teletext page CRC
    uint8_t b;
    
    for (int i = 0; i < 8; i++)
    {
        b = ((byte >> (7-i)) & 1) ^ ((*crc>>6) & 1) ^ ((*crc>>8) & 1) ^ ((*crc>>11) & 1) ^ ((*crc>>15) & 1);
        *crc = b | ((*crc&0x7FFF)<<1);
    }
}
