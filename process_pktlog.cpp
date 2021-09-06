#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

using namespace std;

/*
Started out thinking I might need to use C++ STL but did not need to. Accomplished everything
using good old C libs.
Pass two arguments 
1. the name of the packet capture file
2. name of the incoming interface of the punt packet, which is also outgoing interface of the inject packet
    It can be a physical or logical(subinterface, portchannel) interface

Generates a CSV file as output using(1) as basefilename with csv extension.
*/
int 
main(int argc, char* argv[]) {
    FILE *in_file, *out_file;
    if ((in_file = fopen(argv[1],"r")) == NULL)
    {
        printf("\nFile not found %s\n", argv[1]);
        exit(-1); 
    }
    char filename[128];
    sprintf(filename, "%s_%s.csv", argv[1], argv[2]);
    if ((out_file = fopen(filename,"w")) == NULL)
    {
        printf("\nCould not open CSV file for writing %s\n", argv[2]);
        exit(-1); 
    }
    fprintf(out_file, "Packet, INTERFACE, Timestamp, Dest IP, Dest Port, Flat_time msecs, Diff msecs  \n");

    // read as many line as you can
    char line[256];
    const char * ts_token = "Timestamp:";
    const char * pktnum_token = "Packet Number:";
    const char * intf_token = " pal:";
    int year, month, day, hour, minutes, secs, msecs;
   int pkt_num = 0;
    char intf[128];
    double prev_pkt_timev4_ctrl = 0, prev_pkt_timev4_echo = 0;
    double curr_pkt_time = 0;
    double prev_pkt_timev6_ctrl = 0;
    double prev_v6_run_avg = 0, prev_v4_echo_run_avg = 0;

    while (fgets(line, sizeof(line), in_file)) {
        /* note that fgets don't strip the terminating \n, checking its
           presence would allow to handle lines longer that sizeof(line) */
        char * buf = strstr(line, pktnum_token);
        if (buf) {
            year = month = day = hour = minutes = secs = msecs =0;
             // Now we are within a specific packet's capture
            //printf("\n line %s", buf);
            sscanf(buf, "Packet Number: %d,", &pkt_num);
            fprintf(out_file, "\n %d,", pkt_num);
            buf = strstr(line, ts_token);
            sscanf(buf,"Timestamp: %d/%d/%d %d:%d:%d.%d ", &year, &month, &day, &hour, &minutes, &secs, &msecs);
            //printf(" minutes %d secs %d msecs %d", minutes, secs, msecs); 
            
            bool intf_match = false;
            bool v4pkt = true;
            // extract input interface name 
            while (fgets(line, sizeof(line), in_file)) {
                buf = strstr(line, intf_token);
                if(buf){
                    sscanf(buf, " pal: %s ", intf);
                    fprintf(out_file, " %s", intf);
                    
                    fprintf(out_file, ", %02d:%02d:%02d.%d ", hour, minutes, secs, msecs);
                    if(strcmp(intf, argv[2]) == 0) {
                        // matches the interface we are chasing
                        intf_match = true;
                        //printf( " %s", intf);
                        curr_pkt_time = hour * 60.0 * 60.0 * 1000.0 + minutes * 60.0 * 1000.0 + secs * 1000.0 + msecs;
                    }
                }
                char ip_str[256];
                if(intf_match) {
                    /*
                    ipv6  hdr : dest ip: fe80::7a72:5dff:fe1b:7fdf
                    ipv6  hdr : src ip : fe80::1211:22ff:fe22:3333
                    */
                    buf = strstr(line, "ipv6  hdr : dest ip: ");
                    if(buf){
                        v4pkt = false;
                        sscanf(buf,"ipv6  hdr : dest ip: %s", ip_str);
                        fprintf(out_file," , %s,", ip_str);
                        //curr_pkt_timev6 = hour * 60.0 * 60.0 * 1000.0 + minutes * 60.0 * 1000.0 + secs * 1000.0 + msecs;
                        //fprintf(out_file, ", %lf, %lf ", curr_pkt_timev6, (curr_pkt_timev6 - prev_pkt_timev6));
                    }
                    /* 
                    ipv4  hdr : dest ip: 2.1.1.2, src ip: 2.1.1.2 => dest and src ip appear on the same line
                    */
                    buf = strstr(line, "ipv4  hdr : dest ip: ");
                    if(buf){
                        sscanf(buf,"ipv4  hdr : dest ip: %s, src", ip_str);
                        // scanf picks up the trailing "," as part of dest IP address hence no need to add comma in output
                        fprintf(out_file," , %s", ip_str);
                        //curr_pkt_timev4 = hour * 60.0 * 60.0 * 1000.0 + minutes * 60.0 * 1000.0 + secs * 1000.0 + msecs;
                        //fprintf(out_file, ", %lf, %lf ", curr_pkt_timev4, (curr_pkt_timev4 - prev_pkt_timev4));
                    }
                }
                buf = strstr(line, "dest port:");
                if(buf){
                    if(intf_match){
                        int dport = 0;
                        sscanf(buf, "dest port: %d,", &dport);
                        fprintf(out_file," %d", dport);
                        if(dport == 3785) {
                            //echo pkt
                            fprintf(out_file, ", %lf, %lf ", curr_pkt_time, (curr_pkt_time - prev_pkt_timev4_echo));
                            prev_pkt_timev4_echo = curr_pkt_time;
                        }
                        else {
                            assert(dport == 3784);
                            if(v4pkt) {
                                fprintf(out_file, ", %lf, %lf \n", curr_pkt_time, (curr_pkt_time - prev_pkt_timev4_ctrl));
                                prev_pkt_timev4_ctrl = curr_pkt_time;
                            }
                            else {
                                fprintf(out_file, ", %lf, %lf \n", curr_pkt_time, (curr_pkt_time - prev_pkt_timev6_ctrl));
                                prev_pkt_timev6_ctrl = curr_pkt_time;
                            }
                        }
                    }
                    break; // fall to outer loop
                }
             }
        }
    }
    fprintf(out_file,"\n");
    fclose(out_file);
    fclose(in_file);
}
