#include <packet830.h>

using namespace vbit;

Packet830::Packet830()
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
  // @todo Find which event happened and send the relevant packet
  p=new Packet(8,25,"                                        "); // @todo I think this is a mistake. SHOULD NOT BE DOING new here
  return p;
}

bool Packet830::IsReady(bool force)
{
  // We will be waiting for 10 fields between becoming true
  bool result=GetEvent(EVENT_P830_FORMAT_1) ||
      GetEvent(EVENT_P830_FORMAT_2_LABEL_0) ||
      GetEvent(EVENT_P830_FORMAT_2_LABEL_1) ||
      GetEvent(EVENT_P830_FORMAT_2_LABEL_2) ||
      GetEvent(EVENT_P830_FORMAT_2_LABEL_3);
  return result;
}
