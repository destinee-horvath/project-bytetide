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
struct all_packages* all_packages;

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
        struct config_obj* config = load_config(config_file);
        if (config == NULL) {
            return 0;
        }

        //check if file has valid information (function exits if not valid)
        check_config_obj(config);

        //create socket 
        int server = make_server(config->dir, config->port, config->max_peers);

        //dynamically allocate space to store all peers 
        all_peers = malloc(sizeof(struct all_peers));
        if (all_peers == NULL) {
            fprintf(stderr, "Error: failed to allocate memory\n");
            return 1;  
        }

        all_peers->peers = NULL;
        all_peers->size = 0;
        all_peers->max_peers = config->max_peers;

        //dynamically allocate space for all packages 
        all_packages = malloc(sizeof(struct all_packages));
        if (all_packages == NULL) {
            fprintf(stderr, "Error: failed to allocate memory\n");
            return 1;  
        }

        all_packages->packages = NULL;
        all_packages->size = 0;

        pthread_t accept_thread, monitor_thread;
        pthread_create(&accept_thread, NULL, accept_connections, &server);
        pthread_create(&monitor_thread, NULL, check_disconnection, NULL);
        
        
        char command[COMMAND_BUFFER];

        char* token;
        char* address;
        char* port_str;
        int port;

        while (1) {
            fgets(command, sizeof(command), stdin);
            command[strcspn(command, "\n")] = 0; 

            switch (command_handler(command)) {
                case -1: //invalid
                    printf("Invalid command\n");
                    break;
                    
                case 0: //QUIT
                    // destroy_peers(&all_peers);
                    destroy_all_packages(&all_packages);
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
                    break;

                case 3: //PEERS
                    print_peers(&all_peers);
                    break;

                case 4: //ADDPACKAGE
                    token = strtok(command, " ");
                    token = strtok(NULL, " ");   

                    if (token == NULL) {
                        printf("Missing arguments\n");
                        break;
                    }

                    size_t path_len = strlen(config->dir) + 1 + strlen(token) + 1; // +1 '/' and +1 '\0'
                    char* file_path = (char*)malloc(path_len);
                    if (file_path == NULL) {
                        fprintf(stderr, "Error: failed to allocate memory");
                        break;
                    }
                    
                    snprintf(file_path, path_len, "%s/%s", config->dir, token); //note add '/'

                    add_package(file_path, &all_packages);

                    free(file_path);
                    break;

                case 5: //REMPACKAGE
                    token = strtok(command, " ");
                    token = strtok(NULL, " ");   

                    remove_package(token, &all_packages);
                    break;

                case 6: //PACKAGES
                    print_packages(all_packages);
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
        destroy_config_obj(config);
    }

    else {
        printf("no config file\n");
    }

    return 0;
}
