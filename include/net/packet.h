#ifndef NETPKT_H
#define NETPKT_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "chk/pkgchk.h"


#define PAYLOAD_MAX (4092)

#define PKT_MSG_ACK 0x0c
#define PKT_MSG_ACP 0x02
#define PKT_MSG_DSN 0x03
#define PKT_MSG_REQ 0x06
#define PKT_MSG_RES 0x07
#define PKT_MSG_PNG 0xFF
#define PKT_MSG_POG 0x00

// struct bpkg_obj;

union btide_payload {
    uint8_t data[PAYLOAD_MAX];
};

//packet to be sent 
struct btide_packet {
    uint16_t msg_code;
    uint16_t error;
    union btide_payload pl;
};

struct all_packages {
    struct package** packages; 
    int size; 
};

struct package {
    struct bpkg_obj* loaded_bpkg;
    int status; //1 for complete, 0 for incomplete 
    char* filename;
    char* identifier;
};

void add_package(char*, struct all_packages**, char*);

void remove_package(char*, struct all_packages**);

void print_packages(struct all_packages*);

void destroy_all_packages(struct all_packages**);

#endif
