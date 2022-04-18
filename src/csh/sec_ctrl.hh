#ifndef __CSH_SEC_CTRL_HH__
#define __CSH_SEC_CTRL_HH__

#include "params/SecCtrl.hh"
#include "sim/sim_object.hh"

namespace gem5
{

class SecCtrl : public SimObject
{
  public:

    /**
     * Constructor
     */
    SecCtrl(const SecCtrlParams &p);
};

} // namespace gem5

#endif // __CSH_SEC_CTRL_HH__
