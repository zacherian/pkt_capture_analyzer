# pkt_capture_analyzer
C/C++ source that can analyze a packet capture dump from a catalyst 9k switch to provide inter-packet latencies. You should be able to compile this with "g++ process_pktlog.c" and running the resulting a.out file .
Pass two arguments 
1. the name of the packet capture file
2. name of the incoming interface of the punt packet, which is also outgoing interface of the inject packet
    It can be a physical or logical(subinterface, portchannel) interface
