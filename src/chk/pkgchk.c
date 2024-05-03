#include "chk/pkgchk.h"

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include <sys/stat.h> //for stat to check if file exists

// PART 1


/**
 * Loads the package for when a valid path is given
 */
struct bpkg_obj* bpkg_load(const char* path) {
    //check if file path is valid (by opening an existing package)
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Error: File path does not exist: %s\n", path);
        return NULL;
    }

    //dynamically allocate space for obj file
    struct bpkg_obj* obj = malloc(sizeof(struct bpkg_obj));
    if (obj == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory\n");        
        fclose(file); //close file 
        return NULL;
    }

    //initialise values for object file
    obj->identifier = NULL;
    obj->filename = NULL;
    obj->size = 0;
    obj->len_hash = 0;
    obj->hashes = NULL; 
    obj->len_chunk = 0;
    obj->chunks_hash = NULL;
    obj->chunks_offset = 0;
    obj->chunks_size = 0;

    char line[BUFFER];

    //check sections of .bpkg file
    fscanf(file, "ident: %1024s\n", obj->identifier);
    fscanf(file, "filename: %256s\n", obj->filename);
    fscanf(file, "size: %zu\n", &obj->size);
    fscanf(file, "nhashes: %zu\n", &obj->len_hash);

    //dynamically allocate space for hash pointers 
    obj->hashes = malloc(obj->len_hash * sizeof(char*));

    //read hashes
    for (size_t i = 0; i < obj->len_hash; i++) {
        //read line 
        if (fgets(line, sizeof(line), file) == NULL) {
            fprintf(stderr, "Error: Fail to read line");
            //free other hashes
            for (size_t j = 0; j < i; j++) {
                free(obj->hashes[j]);
            }
            free(obj->hashes);
            obj->hashes = NULL;
            break; 
        }
        //dynamically allocate space for hash
        obj->hashes[i] = malloc((HASH_SIZE + 1)* sizeof(char)); //+1 to account for null bit
        if (obj->hashes[i] == NULL) {
            fprintf(stderr, "Error: Failed to allocate memory\n");
            //free other hashes
            for (size_t j = 0; j < i; j++) {
                free(obj->hashes[j]);
            }
            free(obj->hashes);
            obj->hashes = NULL;
            return obj;
        }

        //shift start pointer 
        char* start = line;
        while (*start == ' ' || *start == '\t') {
            start++;        
        }
        //store hash
        if (sscanf(start, "%64s", obj->hashes[i]) != 1) {
            fprintf(stderr, "Error: Failed to scan hash\n");
            free(obj->hashes[i]);
        }
    
    }

    fscanf(file, "nchunks: %zu\n", &obj->len_chunk);
    for (size_t i = 0; i < obj->len_chunk; i++) {
        //read line 
        if (fgets(line, sizeof(line), file) == NULL) {
            fprintf(stderr, "Error: Fail to read line");
            //free other hashes
            for (size_t j = 0; j < i; j++) {
                free(obj->chunks_hash[j]);
            }
            free(obj->chunks_hash);
            obj->chunks_hash = NULL;
            break; 
        }
        //dynamically allocate space for hash
        obj->chunks_hash[i] = malloc((HASH_SIZE + 1)* sizeof(char)); //+1 to account for null bit
        if (obj->chunks_hash[i] == NULL) {
            fprintf(stderr, "Error: Failed to allocate memory\n");
            //free other hashes
            for (size_t j = 0; j < i; j++) {
                free(obj->chunks_hash[j]);
            }
            free(obj->chunks_hash);
            obj->chunks_hash = NULL;
            return obj;
        }

        //shift start pointer 
        char* start = line;
        while (*start == ' ' || *start == '\t') {
            start++;        
        }
        //store hash
        if (sscanf(start, "%64s", obj->chunks_hash[i]) != 1) {
            fprintf(stderr, "Error: Failed to scan hash\n");
            free(obj->chunks_hash[i]);
        }
    
    }

    //close file 
    fclose(file);

    //returns the allocated space for struct bpkg_obj
    return obj;
}

/**
 * Checks to see if the referenced filename in the bpkg file
 * exists or not.
 * @param bpkg, constructed bpkg object
 * @return query_result, a single string should be
 *      printable in hashes with len sized to 1.
 * 		If the file exists, hashes[0] should contain "File Exists"
 *		If the file does not exist, hashes[0] should contain "File Created"
 */
struct bpkg_query bpkg_file_check(struct bpkg_obj* bpkg) {
    struct bpkg_query query_result;

    //dynamically allocate space for hash pointer
    query_result.hashes = malloc(sizeof(char*)); 
    if (query_result.hashes == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory\n");
        return query_result; 
    }
    query_result.len = 1; //query set to len 1 

    //bpkg obj doesnt exists
    if (bpkg == NULL) {
        return query_result;
    }

    //check if bpkg->filename exists in folder 
    struct stat buffer;

    //construct file path 
    char file_path[BUFFER];
    snprintf(file_path, sizeof(file_path), "resources/pkgs/%s", bpkg->filename); //to contatenate strings 

    //dynamically allocate space for string 
    query_result.hashes[0] = malloc(FILECHECK_SIZE); 
    if (query_result.hashes[0] == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory\n");
        free(query_result.hashes);
        return query_result;  
    }

    //file exists
    if (stat(file_path, &buffer) == 0) {
        strcpy(query_result.hashes[0], "File Exists");
    }
    //file does not exist
    else {
        //create file 
        FILE *file = fopen(file_path, "w");
        if (file != NULL) {
            strcpy(query_result.hashes[0], "File Created");
            fclose(file);
        } 
        else {
            fprintf(stderr, "Error: Failed to create file\n");
            free(query_result.hashes);
            free(query_result.hashes[0]);
        }
    }

    return query_result;
}

/**
 * Retrieves a list of all hashes within the package/tree
 * @param bpkg, constructed bpkg object
 * @return query_result, This structure will contain a list of hashes
 * 		and the number of hashes that have been retrieved
 */
struct bpkg_query bpkg_get_all_hashes(struct bpkg_obj* bpkg) {
    struct bpkg_query qry = { 0 };
    
    //check if bpkg package exists 

    //copy number of hashes 

    //dynamically allocate memory for hashes 

    //copy hashes bpkg to qry

    return qry;
}

/**
 * Retrieves all completed chunks of a package object
 * @param bpkg, constructed bpkg object
 * @return query_result, This structure will contain a list of hashes
 * 		and the number of hashes that have been retrieved
 */
struct bpkg_query bpkg_get_completed_chunks(struct bpkg_obj* bpkg) { 
    struct bpkg_query qry = { 0 };
    return qry;
}


/**
 * Gets the mininum of hashes to represented the current completion state
 * Example: If chunks representing start to mid have been completed but
 * 	mid to end have not been, then we will have (N_CHUNKS/2) + 1 hashes
 * 	outputted
 *
 * @param bpkg, constructed bpkg object
 * @return query_result, This structure will contain a list of hashes
 * 		and the number of hashes that have been retrieved
 */
struct bpkg_query bpkg_get_min_completed_hashes(struct bpkg_obj* bpkg) {
    struct bpkg_query qry = { 0 };
    return qry;
}


/**
 * Retrieves all chunk hashes given a certain an ancestor hash (or itself)
 * Example: If the root hash was given, all chunk hashes will be outputted
 * 	If the root's left child hash was given, all chunks corresponding to
 * 	the first half of the file will be outputted
 * 	If the root's right child hash was given, all chunks corresponding to
 * 	the second half of the file will be outputted
 * @param bpkg, constructed bpkg object
 * @return query_result, This structure will contain a list of hashes
 * 		and the number of hashes that have been retrieved
 */
struct bpkg_query bpkg_get_all_chunk_hashes_from_hash(struct bpkg_obj* bpkg, 
    char* hash) {
    
    struct bpkg_query qry = { 0 };
    return qry;
}


/**
 * Deallocates the query result after it has been constructed from
 * the relevant queries above.
 */
void bpkg_query_destroy(struct bpkg_query* qry) {
    if (qry->hashes != NULL) {
        //iterate through all hashes and deallocate
        for (size_t i = 0; i < qry->len; i++) {
            free(qry->hashes[i]);
        }
        free(qry->hashes);
        qry->hashes = NULL;
    }
    free(qry);
}

/**
 * Deallocates memory at the end of the program,
 * make sure it has been completely deallocated
 */
void bpkg_obj_destroy(struct bpkg_obj* obj) {
    if (obj != NULL) {
        free(obj->identifier);  
        free(obj->filename);    
        
        //free each hash
        if (obj->hashes != NULL) {
            for (size_t i = 0; i < obj->len_hash; i++) {
                free(obj->hashes[i]);  
            }
            free(obj->hashes); 
            obj->hashes = NULL;
        }

        if (obj->chunks_hash != NULL) {
            //free each chunk hash
            for (size_t i = 0; i < obj->len_chunk; i++) {
                free(obj->chunks_hash[i]); 
            }
            free(obj->chunks_hash); 
            obj->chunks_hash = NULL;
        }

        if (obj->chunks_offset != NULL) {
            free(obj->chunks_offset); 
            obj->chunks_offset = NULL;
        }

        if (obj->chunks_size != NULL) {
            free(obj->chunks_size);  
            obj->chunks_size = NULL;
        }

        free(obj);  
    }
}

