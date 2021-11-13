#include <packet830.h>

using namespace vbit;

Packet830::Packet830(ttx::Configure *configure) :
    _configure(configure)
{
    //ctor
    ClearEvent(EVENT_P830_FORMAT_1);
    ClearEvent(EVENT_P830_FORMAT_2_LABEL_0 );
    ClearEvent(EVENT_P830_FORMAT_2_LABEL_1 );
    ClearEvent(EVENT_P830_FORMAT_2_LABEL_2 );
    ClearEvent(EVENT_P830_FORMAT_2_LABEL_3 );
}

Packet830::~Packet830()
{
    //dtor
}

Packet* Packet830::GetPacket(Packet* p)
{
    time_t timeRaw = _configure->GetMasterClock();
    time_t timeLocal;
    struct tm *tmLocal;
    struct tm *tmGMT;
    int offsetHalfHours, year, month, day, hour, minute, second;
    uint32_t modifiedJulianDay;

    std::vector<uint8_t> data(40, 0x15); // 40 bytes filled with hamming coded 0

    p->SetMRAG(8, 30); // Packet 8/30

    uint8_t muxed = _configure->GetMultiplexedSignalFlag();

    uint8_t m = _configure->GetInitialMag();
    uint8_t pn = _configure->GetInitialPage();
    uint16_t sc = _configure->GetInitialSubcode();
    data.at(1) = Hamming8EncodeTable[pn & 0xF];
    data.at(2) = Hamming8EncodeTable[(pn & 0xF0) >> 4];
    data.at(3) = Hamming8EncodeTable[sc & 0xF];
    data.at(4) = Hamming8EncodeTable[((sc & 0xF0) >> 4) | ((m & 1) << 3)];
    data.at(5) = Hamming8EncodeTable[(sc & 0xF00) >> 8];
    data.at(6) = Hamming8EncodeTable[((sc & 0xF000) >> 12) | ((m & 6) << 1)];

    std::copy_n(_configure->GetServiceStatusString().begin(), 20, data.begin() + 20); // copy status display from std::string into packet data

    if (GetEvent(EVENT_P830_FORMAT_1))
    {
        ClearEvent(EVENT_P830_FORMAT_1);
        data.at(0) = Hamming8EncodeTable[muxed]; // Format 1 designation code
        
        uint16_t nic = _configure->GetNetworkIdentificationCode();
        data.at(7) = ReverseByteTab[(nic & 0xFF00) >> 8];
        data.at(8) = ReverseByteTab[nic & 0xFF];
        
        /* calculate number of seconds local time is offset from UTC */
        tmLocal = localtime(&timeRaw);
        
        /* convert tmLocal into a timestamp without correcting for timezones and summertime */
        #ifdef WIN32
        timeLocal = _mkgmtime(tmLocal);
        #else
        timeLocal = timegm(tmLocal);
        #endif
        
        offsetHalfHours = difftime(timeLocal, timeRaw) / 1800;
        // time offset code -bits 2-6 half hours offset from UTC, bit 7 sign bit
        // bits 0 and 7 reserved - set to 1
        data.at(9) = ((offsetHalfHours < 0) ? 0xC1 : 0x81) | ((abs(offsetHalfHours) & 0x1F) << 1);
        
        // get the time current UTC time into separate variables
        tmGMT = gmtime(&timeRaw);
        year = tmGMT->tm_year + 1900;
        month = tmGMT->tm_mon + 1;
        day = tmGMT->tm_mday;
        hour = tmGMT->tm_hour;
        minute = tmGMT->tm_min;
        second = tmGMT->tm_sec;
        
        modifiedJulianDay = calculateMJD(year, month, day);
        // generate five decimal digits of modified julian date decimal digits and increment each one.
        data.at(10) = (modifiedJulianDay % 100000 / 10000 + 1);
        data.at(11) = ((modifiedJulianDay % 10000 / 1000 + 1) << 4) | (modifiedJulianDay % 1000 / 100 + 1);
        data.at(12) = ((modifiedJulianDay % 100 / 10 + 1) << 4) | (modifiedJulianDay % 10 + 1);
        
        // generate six decimal digits of UTC time and increment each one before transmission
        data.at(13) = (((hour / 10) + 1) << 4) | ((hour % 10) + 1);
        data.at(14) = (((minute / 10) + 1) << 4) | ((minute % 10) + 1);
        data.at(15) = (((second / 10) + 1) << 4) | ((second % 10) + 1);
        
        // bytes 22-25 of the packet are marked reserved in the spec. Different broadcasters fill them with different values - we will leave them as hamming coded zero.
        
        p->SetPacketRaw(data);
        p->Parity(25); // set correct parity for status display
        return p;
    }

    // PDC labels 8/30/1
    if (GetEvent(EVENT_P830_FORMAT_2_LABEL_0))
    {
        ClearEvent(EVENT_P830_FORMAT_2_LABEL_0);
        data.at(0) = Hamming8EncodeTable[muxed | 2]; // Format 2 designation code
        
        //@todo
    }
    if (GetEvent(EVENT_P830_FORMAT_2_LABEL_1))
    {
        ClearEvent(EVENT_P830_FORMAT_2_LABEL_1);
        data.at(0) = Hamming8EncodeTable[muxed | 2]; // Format 2 designation code
        
        //@todo
    }
    if (GetEvent(EVENT_P830_FORMAT_2_LABEL_2))
    {
        ClearEvent(EVENT_P830_FORMAT_2_LABEL_2);
        data.at(0) = Hamming8EncodeTable[muxed | 2]; // Format 2 designation code
        
        //@todo
    }
    if (GetEvent(EVENT_P830_FORMAT_2_LABEL_3))
    {
        ClearEvent(EVENT_P830_FORMAT_2_LABEL_3 );
        data.at(0) = Hamming8EncodeTable[muxed | 2]; // Format 2 designation code
        
        //@todo
    }
    return nullptr;
}

bool Packet830::IsReady(bool force)
{
    // We will be waiting for 10 fields between becoming true
    // 8/30/1 should go out on the system clock seconds interval.
    (void)force; // silence error about unused parameter
    bool result=GetEvent(EVENT_P830_FORMAT_1) ||
        GetEvent(EVENT_P830_FORMAT_2_LABEL_0) ||
        GetEvent(EVENT_P830_FORMAT_2_LABEL_1) ||
        GetEvent(EVENT_P830_FORMAT_2_LABEL_2) ||
        GetEvent(EVENT_P830_FORMAT_2_LABEL_3);
    return result;
}

long Packet830::calculateMJD(int year, int month, int day)
{
    // calculate modified julian day number
    int a, b, c, d;
    a = (month - 14) / 12;
    b = day - 32075 + (1461 * (year + 4800 + a) / 4);
    c = (367 * (month - 2 - 12 * a) / 12);
    d = 3 * (((year + 4900 + a) / 100) / 4);
    return b + c - d - 2400001;
}
