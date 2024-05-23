#include "net/packet.h"


/**
 * - checks if file exists and adds it to all_packages
 * @params:
 *      char*                  : filename
 *      struct all_packages**  : all_packages
*/
void add_package(char* filename, struct all_packages** all_packages, char* dir) {
    //bpkg file exists
    if (access(filename, F_OK) == 0) {
        //load
        struct bpkg_obj* obj = bpkg_load(filename);
        if (obj == NULL) {
            printf("Error: file cannot be loaded\n");
            return;
        }

        struct package* pkg = (struct package*)malloc(sizeof(struct package));
        if (pkg == NULL) {
            perror("Error: memory allcoation failed");
            return;
        }
        pkg->loaded_bpkg = obj;

        size_t path_len = strlen(dir) + 1 + strlen(obj->filename) + 1; 
        pkg->filename = (char*)malloc(path_len);
        if (pkg->filename == NULL) {
            perror("Error: memory allocation failed");
            free(pkg);
            return;
        }
        snprintf(pkg->filename, path_len, "%s/%s", dir, obj->filename);

        // pkg->filename = strdup(obj->filename);
        pkg->identifier = strdup(obj->identifier);

        //load data file 
        struct bpkg_query all_chunks_data = bpkg_get_completed_chunks(obj);

        //check if complete 
        if (all_chunks_data.len == obj->len_chunk) { //complete if all chunks returned 
            pkg->status = 1;
        }
        else {
            pkg->status = 0;
        }

        bpkg_query_destroy(&all_chunks_data);
        bpkg_obj_destroy(obj);

        //store in all_packages 
        (*all_packages)->size += 1;
        (*all_packages)->packages = (struct package**)realloc((*all_packages)->packages, (*all_packages)->size * sizeof(struct package*));
        if ((*all_packages)->packages == NULL) {
            perror("Error: memory reallocation failed");
            free(pkg->filename);
            free(pkg->identifier);
            free(pkg);
            return;
        }
        (*all_packages)->packages[(*all_packages)->size - 1] = pkg;

    } 
    else {
        printf("file %s not found\n", filename);
    }
}

/**
 * - removes an existing file from all_packages
 * @params:
 *      char*                  : ident_to_remove
 *      struct all_packages**  : all_packages
*/
void remove_package(char* ident_to_remove, struct all_packages** all_packages) {

    if ((*all_packages)->size == 0) {
        printf("No packages");
        return;
    }

    for (int i = 0; i < (*all_packages)->size; i++) {
        if (strcmp((*all_packages)->packages[i]->identifier, ident_to_remove) == 0) {

            free((*all_packages)->packages[i]->filename);
            free((*all_packages)->packages[i]->identifier);
            free((*all_packages)->packages[i]);
            
            // shift down 
            for (int j = i; j < (*all_packages)->size - 1; j++) {
                (*all_packages)->packages[j] = (*all_packages)->packages[j + 1];
            }

            //decrease memory
            (*all_packages)->size--;
            (*all_packages)->packages = (struct package**)realloc((*all_packages)->packages, (*all_packages)->size * sizeof(struct package*));
            if ((*all_packages)->packages == NULL && (*all_packages)->size > 0) {
                perror("Error: memory allocation failed");
            }

            printf("Package has been removed\n");
            return;
        }
    }
    
    printf("ident %s not found.\n", ident_to_remove);
}

/**
 * - prints all existing packages in all_packages
 * @params:
 *      struct all_packages** : all_packages
*/
void print_packages(struct all_packages* all_packages) {
    if (all_packages->size == 0) {
        printf("No packages managed\n");
        return;
    }
    for (int i = 0; i < all_packages->size; i++) {
        struct package* pkg = all_packages->packages[i];
        printf("%d. %.32s, %s : ", i + 1, pkg->identifier, pkg->filename);

        if (pkg->status == 1) {
            printf("COMPLETE\n");
        } else {
            printf("INCOMPLETE\n");
        }
    }
}

/**
 * - destroys all packages
*/
void destroy_all_packages(struct all_packages** all_packages) {
    if (*all_packages == NULL) {
        return;
    }

    for (int i = 0; i < (*all_packages)->size; i++) {
        struct package* pkg = (*all_packages)->packages[i];
        if (pkg != NULL) {
            free(pkg->filename);
            free(pkg->identifier);
            free(pkg);
        }
    }

    free((*all_packages)->packages);

    free(*all_packages);
    *all_packages = NULL;
}