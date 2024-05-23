#include "net/config.h"
#include "net/packet.h"
#include "net/peer.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <sys/socket.h>
#include <pthread.h>

//
// PART 2
//

// pthread_mutex_t peers_mutex = PTHREAD_MUTEX_INITIALIZER;
struct all_peers* all_peers;

/**
 * - determines the command entered
 * @params:
 *      char* : command 
 * @ret: 
 *     -1 : invalid 
 *      0 : QUIT
 *      1 : CONNECT
 *      2 : DISCONNECT 
 *      3 : PEERS
 *      4 : ADDPACKAGE
 *      5 : REMPACKAGE
 *      6 : PACKAGES
 *      7 : FETCH
*/
int command_handler(char* command) {
    if (command == NULL) {
        return -1;
    }
    else if (strncmp(command, "QUIT ", 5) == 0 || //two cases for quit
            strncmp(command, "QUIT", 4) == 0) { 
        
        return 0;
    }
    else if (strncmp(command, "CONNECT ", 8) == 0) {
        return 1;
    } 
    else if (strncmp(command, "DISCONNECT ", 11) == 0) {
        return 2;
    }
    else if (strcmp(command, "PEERS") == 0) {
        return 3;
    } 
    else if (strncmp(command, "ADDPACKAGE ", 11) == 0) {
        return 4;
    } 
    else if (strncmp(command, "REMPACKAGE ", 11) == 0) {
        return 5;
    } 
    else if (strcmp(command, "PACKAGES") == 0) {
        return 6;
    } 
    else if (strncmp(command, "FETCH ", 6) == 0) {
        return 7;
    } 
    
    return -1;
}

/**
 * - creates a server 
 * @params: 
 *      char*  : address
 *      int    : port
 *      int    : max
 * @ret: 
 *      int    : server_fd
*/
int make_server(char* address, int port, int max) {
    int server_fd;
    struct sockaddr_in server_addr;

    //create socket file descriptor
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        fprintf(stderr, "Error: socket failed\n");
        exit(EXIT_FAILURE);
    }

    //configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    //bind socket to address and port
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        fprintf(stderr, "Error: bind failed\n");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    //listen for incoming connections
    if (listen(server_fd, max) < 0) {
        fprintf(stderr, "Error: listen failed\n");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    return server_fd;
}

/**
 * - check if peer has been disconnected.
 * @params:
 *      void* : arg
*/
void *check_disconnection(void *arg) {
    //need to continuously monitor all peers for disconnections
    while (1) {
        if (all_peers != NULL) {
            for (int i = 0; i < (*all_peers).size; i++) {
                check_peer_disconnect((*all_peers).peers[i]->socket, &all_peers);
            }
        }
        
        sleep(1);
    }
}

/**
 * - listening_socket used to accept incoming peer connections
 * @params:
 *      void* : arg
*/
void *accept_connections(void *arg) {
    int listen_socket = *(int *)arg;

    while (1) {
        int peer_socket = accept_connect(listen_socket, &all_peers);
        if (peer_socket >= 0) {
            printf("peer accepted connection\n");
        }
    }
}

int main(int argc, char** argv) {

    if (argc > 1) {
        char* config_file = argv[1];

        //create a config_obj
        struct config_obj* obj = load_config(config_file);
        if (obj == NULL) {
            return 0;
        }

        //check if file has valid information (function exits if not valid)
        check_config_obj(obj);

        //create socket 
        int server = make_server(obj->dir, obj->port, obj->max_peers);

        //dynamically allocate space to store all peers 
        all_peers = malloc(sizeof(struct all_peers));
        if (all_peers == NULL) {
            fprintf(stderr, "Error: failed to allocate memory\n");
            return 1; 
        }

        all_peers->peers = NULL;
        all_peers->size = 0;
        all_peers->max_peers = obj->max_peers;

        pthread_t accept_thread, monitor_thread;
        pthread_create(&accept_thread, NULL, accept_connections, &server);
        pthread_create(&monitor_thread, NULL, check_disconnection, NULL);
        
        
        char command[BUFFER];

        char* token;
        char* address;
        char* port_str;
        int port;

        while (1) {
            fgets(command, sizeof(command), stdin);
            command[strcspn(command, "\n")] = 0; 

            printf("command: %s\n", command);

            switch (command_handler(command)) {
                case -1: //invalid
                    printf("Invalid command\n");
                    break;
                    
                case 0: //QUIT
                    // destroy_peers(&all_peers);
                    exit(0);

                case 1: //CONNECT
                    token = strtok(command, " "); 
                    token = strtok(NULL, " ");   

                    //split address and port by ':'
                    address = strtok(token, ":");
                    port_str = strtok(NULL, ":");

                    if (address == NULL || port_str == NULL) {
                        printf("Missing arguments\n");
                        break;
                    }

                    port = atoi(port_str);
                    int res = connect_peer(&all_peers, address, port);
                    if (res > -1) {
                        printf("Connection established with peer\n");
                        print_peers(&all_peers);
                    }
                    else {
                        printf("Failed to connect peer\n");
                    }
                    break;

                case 2: //DISCONNECT
                    token = strtok(command, " "); 
                    token = strtok(NULL, " ");   

                    //split address and port by ':'
                    address = strtok(token, ":");
                    port_str = strtok(NULL, ":");

                    if (address == NULL || port_str == NULL) {
                        printf("Missing arguments\n");
                        break;
                    }

                    port = atoi(port_str);

                    //check if peer exists
                    int peer_index = find_peer(&all_peers, address, port);
                    if (peer_index == -1) {
                        printf("Peer not found\n");
                        break;
                    }

                    disconnect_peer(&all_peers, address, port);

                    printf("Disconnected from peer\n");
                    print_peers(&all_peers);
                    break;

                case 3: //PEERS
                    print_peers(&all_peers);
                    break;

                case 4: //ADDPACKAGE
                    break;

                case 5: //REMPACKAGE
                    break;

                case 6: //PACKAGES
                    break;
                
                case 7: //FETCH
                    break;

                default: 
                    printf("Unknown command\n");
                    break;
            

            }
        }

        destroy_peers(&all_peers);
        close(server);
        destroy_config_obj(obj);
    }

    else {
        printf("no config file\n");
    }

    return 0;
}
