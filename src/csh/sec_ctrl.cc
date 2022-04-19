#include "csh/sec_ctrl.hh"

#include "base/trace.hh"
#include "debug/SecCtrl.hh"

namespace gem5
{

SecCtrl::SecCtrl(const SecCtrlParams &p) :
    SimObject(p),
    cpuSidePort(name() + ".cpu_side_port", this),
    memSidePort(name() + ".mem_side_port", this)
{}

AddrRangeList
SecCtrl::CPUSidePort::getAddrRanges() const
{
    return ctrl->getAddrRanges();
}

void
SecCtrl::CPUSidePort::recvFunctional(PacketPtr pkt)
{
    ctrl->handleFunctional(pkt);
}

bool
SecCtrl::CPUSidePort::recvTimingReq(PacketPtr pkt)
{
    return ctrl->handleRequest(pkt);
}

void
SecCtrl::CPUSidePort::recvRespRetry()
{
    ctrl->handleRespRetry();
}

bool
SecCtrl::MemSidePort::recvTimingResp(PacketPtr pkt)
{
    return ctrl->handleResponse(pkt);
}

void
SecCtrl::MemSidePort::recvReqRetry()
{
    ctrl->handleReqRetry();
}

void
SecCtrl::MemSidePort::recvRangeChange()
{
    ctrl->handleRangeChange();
}

bool
SecCtrl::handleRequest(PacketPtr pkt)
{
    return memSidePort.sendTimingReq(pkt);
}

bool
SecCtrl::handleResponse(PacketPtr pkt)
{
    return cpuSidePort.sendTimingResp(pkt);
}

void
SecCtrl::handleReqRetry()
{
    cpuSidePort.sendRetryReq();
}

void
SecCtrl::handleRespRetry()
{
    memSidePort.sendRetryResp();
}

void
SecCtrl::handleFunctional(PacketPtr pkt)
{
    memSidePort.sendFunctional(pkt);
}

AddrRangeList
SecCtrl::getAddrRanges() const
{
    DPRINTF(SecCtrl, "Sending new ranges\n");
    return memSidePort.getAddrRanges();
}

void
SecCtrl::handleRangeChange()
{
    cpuSidePort.sendRangeChange();
}

Port &
SecCtrl::getPort(const std::string &if_name, PortID idx)
{
    panic_if(idx != InvalidPortID, "This object doesn't support vector ports");

    if (if_name == "cpu_side_port") {
        return cpuSidePort;
    } else if (if_name == "mem_side_port") {
        return memSidePort;
    } else {
        return SimObject::getPort(if_name, idx);
    }
}

} // namespace gem5
