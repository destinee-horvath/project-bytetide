//handle server and client communication
//handle packets 

#include "net/peer.h"
#include "net/packet.h"

/**
 * - adds a peer to all_peers
 * @params: 
 *      struct all_peers** : all_peers
 *      struct peer_obj*   : new_peer
*/
void add_peer(struct all_peers** all_peers, struct peer_obj* new_peer) {
    if ((*all_peers)->size == (*all_peers)->max_peers) {
        printf("Error: max peers reached\n");
        close(new_peer->socket);
        free(new_peer);
        return;
    }

    //reallocate space for all_peers
    (*all_peers)->peers = realloc((*all_peers)->peers, 
                                  ((*all_peers)->size + 1) * sizeof(struct peer_obj*));
    
    if (!(*all_peers)->peers) {
        perror("Error: realloc failed");
        close(new_peer->socket);
        free(new_peer);
        return;
    }

    (*all_peers)->peers[(*all_peers)->size] = new_peer;
    (*all_peers)->size++;
}

/**
 * - removes a peer with a socket number from all_peers
 * @params: 
 *      struct all_peers** : all_peers
 *      int                : socket
*/
void remove_peer(struct all_peers** all_peers, int socket) {
    for (int i = 0; i < (*all_peers)->size; i++) {
        if ((*all_peers)->peers[i]->socket == socket) {
            close((*all_peers)->peers[i]->socket);
            free((*all_peers)->peers[i]);

            //shift left
            for (int j = i; j < (*all_peers)->size - 1; j++) {
                (*all_peers)->peers[j] = (*all_peers)->peers[j + 1];
            }

            (*all_peers)->size--;
            return;
        }
    }   
}

/**
 * - finds a peer with a given address and port 
 * @params: 
 *      struct all_peers** : all_peers
 *      char*              : address
 *      int                : port
*/
int find_peer(struct all_peers** all_peers, char* address, int port) {
    for (int i = 0; i < (*all_peers)->size; i++) {
        struct sockaddr_in peer_addr = (*all_peers)->peers[i]->address;
        
        if (peer_addr.sin_port == htons(port) && 
            strcmp(inet_ntoa(peer_addr.sin_addr), address) == 0) {
            return i;
        }
    }
    return -1;
}

/**
 * - handles accepting new connection on listening socket
 * @params:        
 *      struct all_peers**  : all_peers
 *      int                 : listening_socket
*/
int accept_connect(int listening_socket, struct all_peers** all_peers) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    //accept connection
    int new_socket = accept(listening_socket, (struct sockaddr *)&client_addr, &addr_len);
    if (new_socket < 0) {
        perror("Accept failed");
        return -1;
    }

    //dynamically allocate space for new peer 
    struct peer_obj* new_peer = (struct peer_obj *)malloc(sizeof(struct peer_obj));
    if (!new_peer) {
        perror("Malloc failed");
        close(new_socket);
        return -1;
    }
    new_peer->address = client_addr;
    new_peer->socket = new_socket;

    add_peer(all_peers, new_peer);

    printf("Accepted connection from peer: %s:%d\n",
           inet_ntoa(client_addr.sin_addr),
           ntohs(client_addr.sin_port));

    return new_socket;
}

/**
 * - connects port to a peer at address 
 * @params: 
 *      struct all_peers**  : all_peers
 *      char*               : address
 *      int                 : port
 * @ret: 
 *      -1   if error 
 *      >= 0 if successful 
*/
int connect_peer(struct all_peers** all_peers, char* address, int port) {

    if ((*all_peers)->size == (*all_peers)->max_peers) {
        printf("Maximum peers reached\n");
        return -1;
    }
    if (address == NULL || port < 0) {
            printf("Address missing\n");
        return -1;
    }

    //check if peer exists with address and port
    int peer_index = find_peer(all_peers, address, port);
    if (peer_index != -1) {
        printf("Error: peer connected\n");
        return (*all_peers)->peers[peer_index]->socket;
    }

    //create socket file descriptor 
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("Error: failed to create socket\n");
        return -1;
    }

    //configure server address 
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    //convert IPv4 and IPv6 addreses (text -> binary form)
    if (inet_pton(AF_INET, address, &server_addr.sin_addr) <= 0) {
        perror("Error: invalid address\n");
        close(sock_fd);
        return -1;
    }

    //connect to server 
    if (connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error: connection failed\n");
        close(sock_fd);
        return -1;
    }

    //make new peer and add to all_peers
    struct peer_obj* new_peer = malloc(sizeof(struct peer_obj));
    if (!new_peer) {
        perror("Malloc failed");
        close(sock_fd);
        return -1;
    }
    new_peer->address = server_addr;
    new_peer->socket = sock_fd;

    add_peer(all_peers, new_peer);

    // printf("Connection established with peer\n");
    // print_peers(all_peers);

    return sock_fd;
}

/**
 * - prints current connections
 * @params: 
 *      struct all_peers** : all_peers
*/
void print_peers(struct all_peers** all_peers) {
    if ((*all_peers)->size == 0) {
        printf("Not connected to any peers\n");
        return;
    }
    
    printf("Connected to:\n\n");
    for (int i = 0; i < (*all_peers)->size; i++) {
        char addr_str[INET_ADDRSTRLEN];
        
        //convert binary form of IP address to text form
        inet_ntop(AF_INET, &((*all_peers)->peers[i]->address.sin_addr), 
            addr_str, INET_ADDRSTRLEN);
        
        printf("%d. %s:%d\n", 
                i + 1, addr_str, ntohs((*all_peers)->peers[i]->address.sin_port));
    }
}

/** 
 * - frees a peer to be disconnected (disconnect last peer)
 * @params: 
 *      struct all_peers** : all_peers
 *      char*              : address
 *      port               : port
*/
void disconnect_peer(struct all_peers** all_peers, char* address, int port) {
    if ((*all_peers)->size == 0) {
        printf("No peers\n");
        return;
    }

    int peer_index = find_peer(all_peers, address, port);
    if (peer_index == -1) {
        printf("Peer not found\n");
        return;
    }

    struct peer_obj* peer = (*all_peers)->peers[peer_index];

    //tell peer about disconnection 
    struct btide_packet dsn_packet;
    dsn_packet.msg_code = PKT_MSG_DSN;
    dsn_packet.error = 0;
    memset(dsn_packet.pl.data, 0, PAYLOAD_MAX);

    send(peer->socket, &dsn_packet, sizeof(dsn_packet), 0);
    close(peer->socket);

    remove_peer(all_peers, peer->socket);

    // printf("Disconnected from peer\n");
    // print_peers(all_peers);
}

/**
 * - checks if a peer is disconnected
 * @params:
 *      int                : socket
 *      struct all_peers** : all_peers
*/
void check_peer_disconnect(int socket, struct all_peers** all_peers) {
    char buffer[1];
    int result = recv(socket, buffer, sizeof(buffer), MSG_PEEK);

    if (result == 0) {
        for (int i = 0; i < (*all_peers)->size; i++) {
            if ((*all_peers)->peers[i]->socket == socket) {
                printf("peer disconnected.\n");

                remove_peer(all_peers, i);
                return;
            }
        }
    } 
}

/**
 * - function to destroy all peers 
 * @params: 
 *      struct all_peers**  : all_peers
*/
void destroy_peers(struct all_peers** all_peers) {
    if (all_peers == NULL || *all_peers == NULL) {
        return;
    }

    if ((*all_peers)->size > 0) {
        for (int i = 0; i < (*all_peers)->size; i++) {
            close((*all_peers)->peers[i]->socket);
            free((*all_peers)->peers[i]);
        }
        free((*all_peers)->peers);
    }

    free(*all_peers);
    *all_peers = NULL;
}