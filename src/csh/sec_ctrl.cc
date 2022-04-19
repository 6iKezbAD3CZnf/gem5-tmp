#include "csh/sec_ctrl.hh"

#include "base/trace.hh"
#include "debug/SecCtrl.hh"

namespace gem5
{

SecCtrl::SecCtrl(const SecCtrlParams &p) :
    SimObject(p),
    cpuSidePort(name() + ".cpu_side_port", this),
    memSidePort(name() + ".mem_side_port", this),
    blocked(false)
{
    DPRINTF(SecCtrl, "Constructing\n");
}

void
SecCtrl::CPUSidePort::sendPacket(PacketPtr pkt)
{
    panic_if(blockedPacket != nullptr, "Should never try to send if blocked!");

    if (!sendTimingResp(pkt)) {
        blockedPacket = pkt;
    }
}

void
SecCtrl::CPUSidePort::trySendRetry()
{
    if (needRetry && blockedPacket == nullptr) {
        needRetry = false;
        DPRINTF(SecCtrl, "Sending retry req for %d\n", id);
        sendRetryReq();
    }
}

AddrRangeList
SecCtrl::CPUSidePort::getAddrRanges() const
{
    return ctrl->getAddrRanges();
}

Tick
SecCtrl::CPUSidePort::recvAtomic(PacketPtr pkt)
{
    return ctrl->handleAtomic(pkt);
}

void
SecCtrl::CPUSidePort::recvFunctional(PacketPtr pkt)
{
    ctrl->handleFunctional(pkt);
}

bool
SecCtrl::CPUSidePort::recvTimingReq(PacketPtr pkt)
{
    if (!ctrl->handleRequest(pkt)) {
        needRetry = true;
        return false;
    } else {
        return true;
    }
}

void
SecCtrl::CPUSidePort::recvRespRetry()
{
    assert(blockedPacket != nullptr);

    PacketPtr pkt = blockedPacket;
    blockedPacket= nullptr;

    sendPacket(pkt);
}

void
SecCtrl::MemSidePort::sendPacket(PacketPtr pkt)
{
    panic_if(blockedPacket != nullptr, "Should never try to send if blocked!");

    bool needsResponse = pkt->needsResponse();

    if (sendTimingReq(pkt)) {
        if (!needsResponse) {
            ctrl->blocked = false;
        }
    } else {
        blockedPacket = pkt;
    }
}

bool
SecCtrl::MemSidePort::recvTimingResp(PacketPtr pkt)
{
    return ctrl->handleResponse(pkt);
}

void
SecCtrl::MemSidePort::recvReqRetry()
{
    assert(blockedPacket != nullptr);

    PacketPtr pkt = blockedPacket;
    blockedPacket = nullptr;

    sendPacket(pkt);
}

void
SecCtrl::MemSidePort::recvRangeChange()
{
    ctrl->handleRangeChange();
}

bool
SecCtrl::handleRequest(PacketPtr pkt)
{
    if (blocked) {
        return false;
    }

    DPRINTF(SecCtrl, "Got request for addr %#x\n", pkt->getAddr());

    blocked = true;
    memSidePort.sendPacket(pkt);
    return true;
}

bool
SecCtrl::handleResponse(PacketPtr pkt)
{
    assert(blocked);
    DPRINTF(SecCtrl, "Got response for addr %#x\n", pkt->getAddr());

    blocked = false;
    cpuSidePort.sendPacket(pkt);
    cpuSidePort.trySendRetry();
    return true;
}

Tick
SecCtrl::handleAtomic(PacketPtr pkt)
{
    return memSidePort.sendAtomic(pkt);
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
