#include "chk/pkgchk.h"
#include "tree/merkletree.h"
#include "crypt/sha256.h"

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
    //dynamically allocate space for obj file
    struct bpkg_obj* obj = malloc(sizeof(struct bpkg_obj));
    if (obj == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory\n");        
        // fclose(file); //close file 
        return NULL;
    }

    obj->path = strdup(path);

    // //initialise values for object file
    obj->identifier = malloc((SIZE_IDENT)*sizeof(char));
    if (obj->identifier == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory\n");
        return NULL;
    }

    obj->size = 0;
    obj->len_hash = 0;
    obj->hashes = NULL; 
    obj->len_chunk = 0;
    obj->chunks_hash = NULL;
    obj->chunks_offset = 0;
    obj->chunks_size = 0;

    //check if file path is valid (by opening an existing package)
    FILE* file = fopen(path, "rb");
    //if file path doesnt exist
    if (file == NULL) {
        // fprintf(stderr, "Error: File path does not exist: %s\n", path);
        
        char* last_slash;
        char* last_dot;

        //strip the '/' character
        last_slash = strrchr(path, '/');
        if (last_slash != NULL) { //move to last '/'
            last_slash++;
        } else { //no '/' found
            last_slash = (char*)path;
        }

        char new_filename[SIZE_FILENAME];
        strcpy(new_filename, last_slash);

        //add '.'
        last_dot = strrchr(new_filename, '.');
        if (last_dot != NULL) {
            *last_dot = '\0'; 
        }

        strcat(new_filename, ".data");

        obj->filename = strdup(new_filename);
        return obj;
    }

    obj->filename = malloc((SIZE_FILENAME+1)*sizeof(char));
    if (obj->filename == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory\n");
        free(obj->identifier);
        return obj;
    }

    //else file path exists 
    char line[BUFFER];

    //check sections of .bpkg file
    fscanf(file, "ident:%1024s\n", obj->identifier);
    fscanf(file, "filename:%256s\n", obj->filename);
    fscanf(file, "size:%u\n", &obj->size);
    fscanf(file, "nhashes:%u\n", &obj->len_hash);

    //dynamically allocate space for hash pointers 
    obj->hashes = malloc(obj->len_hash * sizeof(char*));

    //read hashes
    for (size_t i = 0; i < obj->len_hash+1; i++) {
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
        if (i == 0) {
            continue; //skip over hashes: (offset 1)
        }

        //dynamically allocate space for hash
        obj->hashes[i-1] = malloc((HASH_SIZE) * sizeof(char)); //+1 to account for null bit
        if (obj->hashes[i-1] == NULL) {
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
        if (sscanf(start, "%64s", obj->hashes[i-1]) != 1) {
            fprintf(stderr, "Error: Failed to scan hash\n");
            free(obj->hashes[i-1]);
        }
    }


    fscanf(file, "nchunks: %u\n", &obj->len_chunk);

    //dynamically allocate space for chunk pointers 
    obj->chunks_hash = malloc((obj->len_chunk) * sizeof(char*));
    obj->chunks_offset = malloc(obj->len_chunk * sizeof(uint32_t*));  
    obj->chunks_size = malloc(obj->len_chunk * sizeof(uint32_t*)); 

    for (size_t i = 0; i < obj->len_chunk+1; i++) {
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
        if (i == 0) {
            continue; //skip over chunks: (offset 1)
        }

        //dynamically allocate space for hash
        obj->chunks_hash[i-1] = malloc((HASH_SIZE+1)* sizeof(char)); 

        if (obj->chunks_hash[i-1] == NULL) {
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

        //split line by ','
        char *token_hash, *token_offset, *token_size;

        //store hash
        token_hash = strtok(start, ",");
        if (token_hash == NULL) {
            fprintf(stderr, "Error: token fail\n");
            continue;
        }
        strncpy(obj->chunks_hash[i-1], token_hash, HASH_SIZE);
        obj->chunks_hash[i-1][HASH_SIZE] = '\0'; //end with null terminator 

        //store offset
        token_offset = strtok(NULL, ",");
        if (token_offset == NULL) {
            fprintf(stderr, "Error: token fail\n");
            continue;
        }
        obj->chunks_offset[i-1] = malloc(sizeof(uint32_t));
        *obj->chunks_offset[i-1] = (uint32_t)atoi(token_offset);

        //store size
        token_size = strtok(NULL, ",");
        if (token_size == NULL) {
            fprintf(stderr, "Error: token fail\n");
            continue;
        }
        obj->chunks_size[i-1] = malloc(sizeof(uint32_t));
        *obj->chunks_size[i-1] = (uint32_t)atoi(token_size);
    }

    //close file 
    fclose(file);
    
    //returns the allocated space for struct bpkg_obj
    return obj;
}

/**
 * flag: -file_check
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

    //dynamically allocate space for string 
    query_result.hashes[0] = malloc(FILECHECK_SIZE); 
    if (query_result.hashes[0] == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory\n");
        free(query_result.hashes);
        return query_result;  
    }

    //check if bpkg->filename exists in folder 
    struct stat buffer;
    //construct file path 
    char file_path[BUFFER];



    if (bpkg->filename != NULL) {
        //contatenate strings to create file path 
        snprintf(file_path, sizeof(file_path), "resources/pkgs/%s", bpkg->filename); 
        
        //file exists
        if (stat(file_path, &buffer) == 0) {
            strcpy(query_result.hashes[0], "File Exists");
        }
        //file does not exist
        else {
            //use the path provided in arg
            snprintf(file_path, sizeof(file_path), "resources/pkgs/%s", bpkg->filename);
            
            //create file 
            FILE *file = fopen(file_path, "w");
            if (file != NULL) {
                strcpy(query_result.hashes[0], "File Exists");
                fclose(file);
            } 
            else {
                fprintf(stderr, "Error: Failed to create file\n");
                free(query_result.hashes);
                free(query_result.hashes[0]);
            }
        }

    }

    
    return query_result;
}

/**
 * flag: -all_hashes
 * Retrieves a list of all hashes within the package/tree
 * @param bpkg, constructed bpkg object
 * @return query_result, This structure will contain a list of hashes
 * 		and the number of hashes that have been retrieved
 */
struct bpkg_query bpkg_get_all_hashes(struct bpkg_obj* bpkg) {
    struct bpkg_query qry = { 0 };
    qry.len = 0; 
    
    //check if bpkg package and hashes exists 
    if (bpkg == NULL || bpkg->hashes == NULL || bpkg->chunks_hash == NULL) {
        return qry;
    }

    //dynamically allocate memory for hashes 
    qry.hashes = malloc((bpkg->len_hash + bpkg->len_chunk) * sizeof(char*));
    if (qry.hashes == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory\n");
        return qry;
    }

    size_t i, j;
    //copy hashes bpkg to qry
    for (i = 0; i < bpkg->len_hash; i++) {
        //dynamically allocate memory
        qry.hashes[i] = strdup(bpkg->hashes[i]);
        
        if (qry.hashes[i] == NULL) {
            fprintf(stderr, "Error: cannot copy\n");
            while (i > 0) {
                free(qry.hashes[i]);
                i--;
            }
            qry.len = 0;
            return qry;
        }
        qry.len++;
    }   

    //copy chunks bpkg to qry
    for (j = 0; j < bpkg->len_chunk; j++) {
        qry.hashes[i + j] = strdup(bpkg->chunks_hash[j]);
        if (qry.hashes[i + j] == NULL) {
            fprintf(stderr, "Error: cannot copy\n");
            while (i > 0) {
                free(qry.hashes[i]);
                i--;
            }
            qry.len = 0;
            return qry;
        }
        qry.len++;
    }

    return qry;
}

/**
 * flag: -chunk_check
 * Retrieves all completed chunks of a package object
 * @param bpkg, constructed bpkg object
 * @return query_result, This structure will contain a list of hashes
 * 		and the number of hashes that have been retrieved
 */
struct bpkg_query bpkg_get_completed_chunks(struct bpkg_obj* bpkg) { 
    struct bpkg_query qry = { 0 };

    printf("bpkg_get_completed_chunks\n");

    struct merkle_tree* tree = build_merkle_tree(bpkg);
    if (tree == NULL) {
        printf("res is Null\n");
        return qry; 
    }
    printf("complete %ld\n", tree->n_nodes);

    //check if bpkg package and chunks exists 
    if (bpkg == NULL || bpkg->chunks_hash == NULL) {
        return qry;
    }

    //dynamically allocate memory for chunks 
    qry.hashes = malloc(bpkg->len_chunk * sizeof(char*));
    if (qry.hashes == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory\n");
        return qry;
    }

    //copy chunks bpkg to qry
    for (size_t i = 0; i < bpkg->len_chunk; i++) {
        qry.hashes[i] = strdup(bpkg->chunks_hash[i]);  //this is an error
        
        if (qry.hashes[i] == NULL) {
            fprintf(stderr, "Error: cannot copy\n");
            bpkg_query_destroy(&qry); 
            break;
        }
    }
    
    destroy_tree(tree);

    return qry;
}


/**
 * flag: -min_hashes
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

    if (bpkg == NULL) {
        return qry;
    }

    // struct merkle_tree_node* root = build_merkle_tree(bpkg);

    qry.hashes = malloc(sizeof(char*));  
    if (!qry.hashes) {
        fprintf(stderr, "Error: Failed to allocate memory\n");
        return qry;
    }

    //hash complete if computed hash match expected hash ??

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

    if (bpkg == NULL) {
        return qry;
    }

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
    // free(qry);
}

/**
 * Deallocates memory at the end of the program,
 * make sure it has been completely deallocated
 */
void bpkg_obj_destroy(struct bpkg_obj* obj) {
    if (obj != NULL) {
        free(obj->path);
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
        }

        if (obj->chunks_offset != NULL) {
            for (size_t i = 0; i < obj->len_chunk; i++) {
                free(obj->chunks_offset[i]); 
            }
            free(obj->chunks_offset);
        }

        if (obj->chunks_hash != NULL) {
            for (size_t i = 0; i < obj->len_chunk; i++) {
                free(obj->chunks_size[i]);  
            }
            free(obj->chunks_size);
        }

        free(obj);  
    }
}

