#include "net/config.h"

/**
 * - used to read a config file with a given filename
 * @params:
 *      char* : filename
*/
struct config_obj* load_config(char* filename) {
    //open config file 
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: config file doesnt exist\n");
        exit(1);
    }

    //dynamically allocate memory for config 
    struct config_obj* config = malloc(sizeof(struct config_obj));
    if (config == NULL) {
        fprintf(stderr, "Error: failed to allocate memory\n");
        fclose(file);
        return NULL;
    }
    memset(config, 0, sizeof(struct config_obj)); //initialise space to 0

    char buffer[BUFFER_CONFIG];
    //readd config
    while (fgets(buffer, sizeof(buffer), file)) {
        if (strncmp(buffer, "directory:", 10) == 0) {
            sscanf(buffer + 10, "%255s", config->dir);
        } 
        else if (strncmp(buffer, "max_peers:", 10) == 0) {
            sscanf(buffer + 10, "%d", &config->max_peers);
        } 
        else if (strncmp(buffer, "port:", 5) == 0) {
            sscanf(buffer + 5, "%hu", &config->port);
        } 
        else {
            fprintf(stderr, "Error: %s in config\n", buffer);
            destroy_config_obj(config);
            return NULL;
        }
    }

    fclose(file);

    return config;
}

/**
 * - checks if config file meets requirements
 * @params: 
 *      struct config_obj*  : obj 
*/
void check_config_obj(struct config_obj* obj) {
    struct stat st;

    //directory doesnt exist 
    if (stat(obj->dir, &st) == -1) {
        //create a directory 
        if (mkdir(obj->dir, 0700) == -1) {
            fprintf(stderr, "Error: failed to create directory");
            exit(3);
        }
    }

    //check directory exists
    if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Error: directory doesnt exist");
        exit(3);
    }

    //check max_peers
    if (obj->max_peers < 1 || obj->max_peers > 2048) {
        fprintf(stderr, "Error: max_peers not in range");
        exit(4);
    }

    //check port 
    if (obj->port <= 1024 || obj->port > 65535) {
        fprintf(stderr, "Error: port not in range");
        exit(5);
    }

} 

/**
 * - frees a config_obj 
 * @params:
 *      struct config_obj*  : obj 
*/
void destroy_config_obj(struct config_obj* obj) {
    free(obj);
    return;
}