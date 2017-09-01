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
  // std::cerr << "[Packet830::Packet830] GetPacket" << std::endl;
  
  char val[40] = {0x23}; // 40 bytes for payload
  
  p->SetMRAG(8, 30); // Packet 8/30
  
  uint8_t muxed = _configure->GetMultiplexedSignalFlag();
  
  // @todo initial page
  
  // @todo Find which event happened and send the relevant packet
	if (GetEvent(EVENT_P830_FORMAT_1))
	{
		ClearEvent(EVENT_P830_FORMAT_1);
		val[0] = HamTab[muxed]; // Format 1 designation code
		
		uint16_t nic = _configure->GetNetworkIdentificationCode();
		val[7] = _vbi_bit_reverse[(nic & 0xFF00) >> 8];
		val[8] = _vbi_bit_reverse[nic & 0xFF];
		
		// @todo time, and date
		
		strncpy(&val[20],_configure->GetServiceStatusString().data(),20); // copy status display from std::string into packet data
		
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
  bool result=GetEvent(EVENT_P830_FORMAT_1) ||
      GetEvent(EVENT_P830_FORMAT_2_LABEL_0) ||
      GetEvent(EVENT_P830_FORMAT_2_LABEL_1) ||
      GetEvent(EVENT_P830_FORMAT_2_LABEL_2) ||
      GetEvent(EVENT_P830_FORMAT_2_LABEL_3);
  return result;
}
