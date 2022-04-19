from m5.objects import Cache
from m5.params import *

class MetaCache(Cache):
    size = '128kB'
    assoc = 4
    tag_latency = 2
    data_latency = 2
    response_latency = 2
    mshrs = 4
    tgts_per_mshr = 20
