/** Packet source for 8/30/1 and 8/30/2
 */
#ifndef _PACKET830_H_
#define _PACKET830_H_

#include "packetsource.h"
#include "configure.h"
#include "vbit2.h"

namespace vbit
{


class Packet830 : public PacketSource
{
  public:
    /** Default constructor */
    Packet830(ttx::Configure *configure);
    /** Default destructor */
    virtual ~Packet830();

    /** @todo Routines for cni, nic, MJD and station ident
     *  @todo Routines for PDC flag management
     */

    // overrides
    Packet* GetPacket(Packet* p) override;

    /**
     * Packet 830 must always wait for the correct field.
     * @param force is ignored
     */
    bool IsReady(bool force=false);

  protected:

  private:
    ttx::Configure* _configure;
    long calculateMJD(int year, int month, int day);
};

}

#endif // _PACKET830_H_
