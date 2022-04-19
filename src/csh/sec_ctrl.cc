#include "csh/sec_ctrl.hh"

#include "base/trace.hh"
#include "debug/SecCtrl.hh"

namespace gem5
{

SecCtrl::SecCtrl(const SecCtrlParams &p) :
    SimObject(p),
    cpuSidePort(name() + ".cpu_side_port", this),
    memSidePort(name() + ".mem_side_port", this),
    cntPort(name() + ".cnt_port", this),
    mtPort(name() + ".mt_port", this),
    macPort(name() + ".mac_port", this),
    dataBorder(0),
    flags(0), requestorId(0),
    blocked(false),
    cnt_blocked(false),
    mt_blocked(false),
    mac_blocked(false),
    responsePkt(nullptr)
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
    DPRINTF(SecCtrl, "Sending new ranges\n");

    AddrRangeList addrRanges = ctrl->getAddrRanges();
    panic_if(addrRanges.size() != 1, "Multiple addresses");
    AddrRange addrRange = addrRanges.front();
    panic_if(addrRange.interleaved(), "This address is interleaved");

    ctrl->dataBorder = addrRange.end() * 2 / 5;
    ctrl->cntBorder = addrRange.end() * 3 / 5;
    ctrl->mtBorder = addrRange.end() * 4 / 5;
    AddrRange dataAddrRange = AddrRange(
            addrRange.start(),
            ctrl->dataBorder);

    DPRINTF(SecCtrl,
            "Original range is %s. New range is %s\n",
                addrRange.to_string(),
                dataAddrRange.to_string());
    return { dataAddrRange };
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
    DPRINTF(SecCtrl, "Got request %s\n", pkt->print());
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
    DPRINTF(SecCtrl, "recvRangeChange\n");

    // DPRINTF(SecCtrl, "port name = %s\n", name());
    if (name() == "system.sec_ctrl.mem_side_port")
        ctrl->handleRangeChange();
}

PacketPtr
SecCtrl::createPkt(Addr addr, unsigned size, bool isRead)
{
    RequestPtr req(new Request(addr, size, flags, requestorId));
    MemCmd cmd = isRead ? MemCmd::ReadReq : MemCmd::WriteReq;
    PacketPtr retPkt = new Packet(req, cmd);
    uint8_t *reqData = new uint8_t[size]; // just empty here
    retPkt->dataDynamic(reqData);

    return retPkt;
}

bool
SecCtrl::handleRequest(PacketPtr pkt)
{
    if (blocked || cnt_blocked || mt_blocked || mac_blocked) {
        DPRINTF(SecCtrl, "Rejected %s\n", pkt->print());
        return false;
    }

    // DPRINTF(SecCtrl, "Got request for addr %#x\n", pkt->getAddr());

    blocked = true;
    cnt_blocked = true;
    mt_blocked = true;
    mac_blocked = true;
    memSidePort.sendPacket(pkt);
    PacketPtr cntPkt = createPkt(dataBorder, 1, true);
    PacketPtr mtPkt = createPkt(cntBorder, 1, true);
    PacketPtr macPkt = createPkt(mtBorder, 1, true);
    cntPort.sendPacket(cntPkt);
    mtPort.sendPacket(mtPkt);
    macPort.sendPacket(macPkt);
    return true;
}

bool
SecCtrl::handleResponse(PacketPtr pkt)
{
    if (pkt->getAddr() < dataBorder) {
        assert(blocked);
        DPRINTF(SecCtrl, "Got response for addr %#x\n", pkt->getAddr());

        blocked = false;
        responsePkt = pkt;
    } else if (pkt->getAddr() < cntBorder) {
        assert(cnt_blocked);
        DPRINTF(SecCtrl, "Got cnt response for addr %#x\n", pkt->getAddr());

        cnt_blocked = false;
    } else if (pkt->getAddr() < mtBorder) {
        assert(mt_blocked);
        DPRINTF(SecCtrl, "Got mt response for addr %#x\n", pkt->getAddr());

        mt_blocked = false;
    } else {
        assert(mac_blocked);
        DPRINTF(SecCtrl, "Got mac response for addr %#x\n", pkt->getAddr());

        mac_blocked = false;
    }

    if (!blocked && !cnt_blocked && !mt_blocked && !mac_blocked) {
        if (responsePkt != nullptr) {
            cpuSidePort.sendPacket(responsePkt);
            responsePkt = nullptr;
        }
        cpuSidePort.trySendRetry();
    }

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
    cntPort.getAddrRanges();
    mtPort.getAddrRanges();
    macPort.getAddrRanges();
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
    } else if (if_name == "cnt_port") {
        return cntPort;
    } else if (if_name == "mt_port") {
        return mtPort;
    } else if (if_name == "mac_port") {
        return macPort;
    } else {
        return SimObject::getPort(if_name, idx);
    }
}

} // namespace gem5
