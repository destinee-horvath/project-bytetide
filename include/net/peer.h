#ifndef PEER_H
#define PEER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>
#include <netinet/in.h>

struct peer_obj {
    struct sockaddr_in address;
    int socket;
};

struct all_peers {
    struct peer_obj** peers;
    int size;
    int max_peers;
};

void add_peer(struct all_peers**, struct peer_obj*);

void remove_peer(struct all_peers**, int);

int find_peer(struct all_peers**, char*, int);

int accept_connect(int, struct all_peers**);

int connect_peer(struct all_peers**, char*, int);

void print_peers(struct all_peers**);

void disconnect_peer(struct all_peers**, char*, int);

void check_peer_disconnect(int, struct all_peers**);

void destroy_peers(struct all_peers**);

#endif