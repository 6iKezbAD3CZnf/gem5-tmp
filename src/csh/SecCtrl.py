from m5.params import *
from m5.SimObject import SimObject

class SecCtrl(SimObject):
    type = 'SecCtrl'
    cxx_header = "csh/sec_ctrl.hh"
    cxx_class = 'gem5::SecCtrl'

    cpu_side_port = ResponsePort("CPU side port")
    mem_side_port = RequestPort("Memory side port")
    cnt_port = RequestPort("Counter port")
    mt_port = RequestPort("Merkle tree port")
    mac_port = RequestPort("Mac port")
