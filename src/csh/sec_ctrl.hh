#ifndef __CSH_SEC_CTRL_HH__
#define __CSH_SEC_CTRL_HH__

#include "mem/port.hh"
#include "params/SecCtrl.hh"
#include "sim/sim_object.hh"

namespace gem5
{

class SecCtrl : public SimObject
{
  private:
    class CPUSidePort: public ResponsePort
    {
      private:
        SecCtrl *ctrl;

      public:
        CPUSidePort(const std::string& name, SecCtrl* _ctrl):
          ResponsePort(name, _ctrl),
          ctrl(_ctrl)
        {}

        AddrRangeList getAddrRanges() const override;

      protected:
        Tick recvAtomic(PacketPtr pkt) override;
        void recvFunctional(PacketPtr pkt) override;
        bool recvTimingReq(PacketPtr pkt) override;
        void recvRespRetry() override;
    };

    class MemSidePort: public RequestPort
    {
      private:
        SecCtrl *ctrl;

      public:
        MemSidePort(const std::string& name, SecCtrl* _ctrl):
          RequestPort(name, _ctrl),
          ctrl(_ctrl)
        {}

      protected:
        bool recvTimingResp(PacketPtr pkt) override;
        void recvReqRetry() override;
        void recvRangeChange() override;
    };

    bool handleRequest(PacketPtr pkt);
    bool handleResponse(PacketPtr pkt);
    void handleReqRetry();
    void handleRespRetry();
    Tick handleAtomic(PacketPtr pkt);
    void handleFunctional(PacketPtr pkt);
    AddrRangeList getAddrRanges() const;
    void handleRangeChange();

    CPUSidePort cpuSidePort;
    MemSidePort memSidePort;

  public:
    SecCtrl(const SecCtrlParams &p);

    Port& getPort(const std::string &if_name,
        PortID idx=InvalidPortID) override;
};

} // namespace gem5

#endif // __CSH_SEC_CTRL_HH__
