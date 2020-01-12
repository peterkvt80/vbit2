#include <packet830.h>

using namespace vbit;

Packet830::Packet830(ttx::Configure *configure) :
  _configure(configure)
{
  //ctor
  std::cerr << "[Packet830::Packet830] constructor" << std::endl;
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
	time_t timeRaw;
	time_t timeLocal;
	struct tm tmLocal;
	struct tm tmGMT;
	int offsetHalfHours, year, month, day, hour, minute, second;
	uint32_t modifiedJulianDay;
	
	// std::cerr << "[Packet830::Packet830] GetPacket" << std::endl;

	char val[40]; // 40 bytes for payload
	memset(val, 0x15, 40); // fill with hamming coded 0

	p->SetMRAG(8, 30); // Packet 8/30

	uint8_t muxed = _configure->GetMultiplexedSignalFlag();

	uint8_t m = _configure->GetInitialMag();
	uint8_t pn = _configure->GetInitialPage();
	uint16_t sc = _configure->GetInitialSubcode();
	val[1] = HamTab[pn & 0xF];
	val[2] = HamTab[(pn & 0xF0) >> 4];
	val[3] = HamTab[sc & 0xF];
	val[4] = HamTab[((sc & 0xF0) >> 4) | ((m & 1) << 3)];
	val[5] = HamTab[(sc & 0xF00) >> 8];
	val[6] = HamTab[((sc & 0xF000) >> 12) | ((m & 6) << 1)];

	memcpy(&val[20],_configure->GetServiceStatusString().data(),20); // copy status display from std::string into packet data
	
	if (GetEvent(EVENT_P830_FORMAT_1))
	{
		ClearEvent(EVENT_P830_FORMAT_1);
		val[0] = HamTab[muxed]; // Format 1 designation code
		
		uint16_t nic = _configure->GetNetworkIdentificationCode();
		val[7] = _vbi_bit_reverse[(nic & 0xFF00) >> 8];
		val[8] = _vbi_bit_reverse[nic & 0xFF];
		
		/* calculate number of seconds local time is offset from UTC */
		time(&timeRaw);
		localtime_r(&timeRaw, &tmLocal);
		
		/* convert tmLocal into a timestamp without correcting for timezones and summertime */
		#ifdef WIN32
		timeLocal = _mkgmtime(&tmLocal);
		#else
		timeLocal = timegm(&tmLocal);
		#endif
		
		offsetHalfHours = difftime(timeLocal, timeRaw) / 1800;
		//std::cerr << "Difference in half hours from UTC: "<< offsetHalfHours << std::endl;
		// time offset code -bits 2-6 half hours offset from UTC, bit 7 sign bit
		// bits 0 and 7 reserved - set to 1
		val[9] = ((offsetHalfHours < 0) ? 0xC1 : 0x81) | ((abs(offsetHalfHours) & 0x1F) << 1);
		
		// get the time current UTC time into separate variables
		gmtime_r(&timeRaw, &tmGMT);
		year = tmGMT.tm_year + 1900;
		month = tmGMT.tm_mon + 1;
		day = tmGMT.tm_mday;
		hour = tmGMT.tm_hour;
		minute = tmGMT.tm_min;
		second = tmGMT.tm_sec;
		
		modifiedJulianDay = calculateMJD(year, month, day);
		// generate five decimal digits of modified julian date decimal digits and increment each one.
		val[10] = (modifiedJulianDay % 100000 / 10000 + 1);
		val[11] = ((modifiedJulianDay % 10000 / 1000 + 1) << 4) | (modifiedJulianDay % 1000 / 100 + 1);
		val[12] = ((modifiedJulianDay % 100 / 10 + 1) << 4) | (modifiedJulianDay % 10 + 1);
		
		// generate six decimal digits of UTC time and increment each one before transmission
		val[13] = (((hour / 10) + 1) << 4) | ((hour % 10) + 1);
		val[14] = (((minute / 10) + 1) << 4) | ((minute % 10) + 1);
		val[15] = (((second / 10) + 1) << 4) | ((second % 10) + 1);
		
		// bytes 22-25 of the packet are marked reserved in the spec. Different broadcasters fill them with different values - we will leave them as hamming coded zero.
		
		p->SetPacketRaw(val);
		p->Parity(25); // set correct parity for status display
		return p;
	}

	// PDC labels 8/30/1
	if (GetEvent(EVENT_P830_FORMAT_2_LABEL_0))
	{
		ClearEvent(EVENT_P830_FORMAT_2_LABEL_0);
		val[0] = HamTab[muxed | 2]; // Format 2 designation code
		
		//@todo
	}
	if (GetEvent(EVENT_P830_FORMAT_2_LABEL_1))
	{
		ClearEvent(EVENT_P830_FORMAT_2_LABEL_1);
		val[0] = HamTab[muxed | 2]; // Format 2 designation code
		
		//@todo
	}
	if (GetEvent(EVENT_P830_FORMAT_2_LABEL_2))
	{
		ClearEvent(EVENT_P830_FORMAT_2_LABEL_2);
		val[0] = HamTab[muxed | 2]; // Format 2 designation code
		
		//@todo
	}
	if (GetEvent(EVENT_P830_FORMAT_2_LABEL_3))
	{
		ClearEvent(EVENT_P830_FORMAT_2_LABEL_3 );
		val[0] = HamTab[muxed | 2]; // Format 2 designation code
		
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
