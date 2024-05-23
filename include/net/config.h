#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdint.h>

#define BUFFER_CONFIG (516)
#define MAX_LEN_DIR (256)
#define COMMAND_BUFFER (2048)

struct config_obj {
    char dir[MAX_LEN_DIR];
    int max_peers;
    uint16_t port;
};

struct config_obj* load_config(char* filename);

void check_config_obj(struct config_obj* obj);

void destroy_config_obj(struct config_obj* obj);
#endif