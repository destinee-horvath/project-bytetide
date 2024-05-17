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

        strcat(new_filename, ".dat");
 
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

        //if nchunks is reached
        if (strncmp(line, "nchunks:", 8) == 0) {
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
        obj->chunks_hash[i-1] = malloc((HASH_SIZE+1) * sizeof(char)); 
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
    qry.len = 0;

    // //check if bpkg package and chunks exists 
    if (bpkg == NULL || bpkg->chunks_hash == NULL) {
        return qry;
    } 

    //dynamically allocate space to store child nodes 
    struct merkle_tree_node** child_nodes = malloc(bpkg->len_chunk * sizeof(struct merkle_tree_node*));
    if (!child_nodes) {
        fprintf(stderr, "Error: Failed to allocate memory\n");
        return qry;
    } 

    //make nodes for leaves
    for (size_t i = 0; i < bpkg->len_chunk; i++) {
        child_nodes[i] = make_node(NULL, NULL, 1);
        if (child_nodes[i] == NULL) {
            return qry;
        }
    }

    //read data into space allocated to child nodes
    size_t ret = read_data(&child_nodes, bpkg);
    if (ret == 1) {
        return qry;
    }

    //dynamically allocate space to store tree 
    struct merkle_tree* tree = malloc(sizeof(struct merkle_tree));
    if (!tree) {
        fprintf(stderr, "Error: Failed to allocate memory\n");
        for (size_t k = 0; k <= (bpkg->len_chunk+bpkg->len_hash); k++) {
            destroy_tree_node(child_nodes[k]);
        }
        free(child_nodes);
        return qry;
    }

    //build tree 
    build_merkle_tree(child_nodes, bpkg, &tree);

    //root matches -> everything else matches 
    if (strcmp(tree->root->computed_hash, bpkg->hashes[0]) == 0) {
        qry.hashes = malloc(bpkg->len_chunk * sizeof(char*));
        if (qry.hashes == NULL) {
            fprintf(stderr, "Error: Memory allocation failed");
            return qry;
        }
        //copy bpkg chunks to hash 
        for (size_t i = 0; i < bpkg->len_chunk; i++) {
            // qry.hashes[i] = malloc(HASH_SIZE * sizeof(char));
            qry.hashes[i] = strdup(bpkg->chunks_hash[i]);
        }
        qry.len = bpkg->len_chunk;
    } 

    //root doesnt match -> traverse tree and return chunks that match (check leaf nodes)
    else {
        qry.hashes = malloc(bpkg->len_chunk * sizeof(char*));
        qry.len = 0;

        char** all_leafs = malloc(bpkg->len_chunk * sizeof(char*));
        if (all_leafs == NULL) {
            fprintf(stderr, "Error: Memory allocation failed");
            return qry;
        }

        // for (size_t i = 0; i < bpkg->len_chunk; i++) {
        //     all_leafs[i] = malloc(HASH_SIZE * sizeof(char));
        //     if (all_leafs[i] == NULL) {
        //         fprintf(stderr, "Error: Memory allocation failed");
        //         return qry;
        //     }
        // }

        size_t tmp_count = 0;
        get_leaf_hashes(tree->root, &all_leafs, &tmp_count);
 
        //compare bpkg->chunks with all_leafs
        size_t count = 0;

        //iterate bpkg->hashes
        for (size_t i = 0; i < bpkg->len_chunk; i++) {
            //iterate all_leafs 
            for (size_t j = 0; j < bpkg->len_chunk; j++) {
                if (strcmp(bpkg->chunks_hash[i], all_leafs[j]) == 0) {  //leaves should be same order as chunks same order as chunks
                    //realloc qry.hash to tmp space  
                    char** tmp = realloc(qry.hashes, (count + 1) * sizeof(char*));
                    if (tmp == NULL) { 
                        fprintf(stderr, "Error: Memory allocation failed\n");
                        break;
                    } 

                    //allocate space
                    qry.hashes = tmp;

                    //dynamically allocate space for hash 
                    qry.hashes[count] = malloc(HASH_SIZE * sizeof(char));
                    if (qry.hashes[count] == NULL) { 
                        fprintf(stderr, "Error: Memory allocation failed\n");
                        break;
                    }

                    //copy
                    strcpy(qry.hashes[count], all_leafs[j]);
                    count++;
                }
            }
            
        }

        //free all leafs 
        for (size_t i = 0; i < bpkg->len_chunk; i++) {
            free(all_leafs[i]);
        }
        free(all_leafs);

        qry.len = count;

    }

    destroy_tree(tree);

    // for (size_t i = 0; i < bpkg->len_chunk; i++) {
    //     free(child_nodes[i]);
    // }
    // free(child_nodes);
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
 * 
 * Returns root of subtree with correct hash
 */
struct bpkg_query bpkg_get_min_completed_hashes(struct bpkg_obj* bpkg) { 
    struct bpkg_query qry = { 0 };
    qry.len = 0;

    if (bpkg == NULL) {
        return qry;
    }

    //dynamically allocate space to store child nodes 
    struct merkle_tree_node** child_nodes = malloc(bpkg->len_chunk * sizeof(struct merkle_tree_node*));
    if (!child_nodes) {
        fprintf(stderr, "Error: Failed to allocate memory\n");
        return qry; 
    } 

    //make nodes for hashes 
    for (size_t i = 0; i < bpkg->len_chunk; i++) {
        child_nodes[i] = make_node(NULL, NULL, 1);
        if (child_nodes[i] == NULL) {
            return qry;
        }
    }

    //read data into space allocated to child nodes
    size_t ret = read_data(&child_nodes, bpkg);
    if (ret == 1) {
        return qry;
    }
 
    //dynamically allocate space to store tree 
    struct merkle_tree* tree = malloc(sizeof(struct merkle_tree));
    if (!tree) {
        fprintf(stderr, "Error: Failed to allocate memory\n");
        for (size_t k = 0; k <= (bpkg->len_chunk+bpkg->len_hash); k++) {
            destroy_tree_node(child_nodes[k]);
        }
        free(child_nodes);
        return qry;
    } 

    //build tree (child_nodes are also freed here)
    build_merkle_tree(child_nodes, bpkg, &tree);
    
    //if root is correct, everything else must be correct 
    if (strcmp(tree->root->computed_hash, bpkg->hashes[0]) == 0) {
        qry.hashes = malloc(bpkg->len_hash * sizeof(char*));
        if (qry.hashes == NULL) { 
            fprintf(stderr, "Error: Memory allocation failed");
            return qry;
        }

        //copy root
        qry.hashes[0] = strdup(bpkg->hashes[0]);
        qry.len = 1;

    }

    //create tree with bpkg file 
    else {
        // //destroy previous tree 
        // destroy_tree(tree);

        // //dynamically allocate space to store new tree 
        // struct merkle_tree* chunk_tree = malloc(sizeof(struct merkle_tree));
        // if (!chunk_tree) {     
        //     return qry;
        // } 

        qry.hashes = malloc((bpkg->len_hash+bpkg->len_chunk) * sizeof(char*));
        size_t count = 0;
        char** hash_result = NULL;

        // //dynamically allocate space to store chunk hash nodes 
        // struct merkle_tree_node** chunk_nodes = malloc(bpkg->len_chunk * sizeof(struct merkle_tree_node*));
        // if (!chunk_nodes) {
        //     fprintf(stderr, "Error: Failed to allocate memory\n");
        //     return qry; 
        // } 

        // //make nodes for chunks 
        // for (size_t i = 0; i < bpkg->len_chunk; i++) {
        //     chunk_nodes[i] = make_node(NULL, NULL, 1);
        //     if (chunk_nodes[i] == NULL) {
        //         return qry;
        //     }
        // }
        
        // //store chunks from bpkg (instead of data)
        // for (size_t i = 0; i < bpkg->len_chunk; i++) {
        //     strcpy(chunk_nodes[i]->computed_hash, bpkg->chunks_hash[i]);
        // }

        // //build tree 
        // build_merkle_tree(chunk_nodes, bpkg, &chunk_tree);

        // //search tree
        // traverse_subtree(chunk_tree->root, hash_result, &count);

        // //destroy tree
        // destroy_tree(chunk_tree);

        //traverse tree and store completed nodes in hash_result 
        get_root_complete_subtree(tree->root, &hash_result, &count, bpkg);

        //copy from hash result to qry->hashes 
        for (size_t i = 0; i < count; i++) {
            qry.hashes[i] = strdup(hash_result[i]);
            qry.len++;
        }

        //free hash_result
        for (size_t i = 0; i < count; i++) {
            free(hash_result[i]); 
        }
        free(hash_result);
    }   

    destroy_tree(tree);
    return qry;
}


/**
 * flag: -hashes_of
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

    //dynamically allocate space to store child nodes 
    struct merkle_tree_node** child_nodes = malloc(bpkg->len_chunk * sizeof(struct merkle_tree_node*));
    if (!child_nodes) {
        fprintf(stderr, "Error: Failed to allocate memory\n");
        return qry; 
    } 

    //make nodes for hashes
    for (size_t i = 0; i < bpkg->len_chunk; i++) {
        child_nodes[i] = make_node(NULL, NULL, 1);
        if (child_nodes[i] == NULL) {
            return qry;
        }
    } 

    //read data into space allocated to child nodes
    size_t ret = read_data(&child_nodes, bpkg);
    if (ret == 1) {
        for (size_t i = 0; i <bpkg->len_chunk; i++) {
            free(child_nodes[i]);
        }
        free(child_nodes); 
        return qry; 
    }

    //dynamically allocate space to store tree 
    struct merkle_tree* tree = malloc(sizeof(struct merkle_tree));
    if (!tree) {
        fprintf(stderr, "Error: Failed to allocate memory\n");
        for (size_t k = 0; k <= (bpkg->len_chunk+bpkg->len_hash); k++) {
            destroy_tree_node(child_nodes[k]);
        }
        free(child_nodes);
        return qry;
    }

    //build tree & free child nodes 
    build_merkle_tree(child_nodes, bpkg, &tree);

    //traverse tree to find hash
    struct merkle_tree_node* root_hash;
    in_order_traversal(tree->root, hash, &root_hash);

    //hash not found 
    if (root_hash == NULL) {
        destroy_tree(tree);
        return qry;
    }

    else { 
        qry.hashes = malloc((bpkg->len_chunk+bpkg->len_hash) * sizeof(char*));
        size_t count = 0;
        char** hash_result = NULL;

        //traverse 
        traverse_subtree_hashes(root_hash, &hash_result, &count);

        //copy 
        for (size_t i = 0; i < count; i++) {
            qry.hashes[i] = strdup(hash_result[i]); 
            qry.len++;
        }

        //free
        for (size_t i = 0; i < count; i++) {
            free(hash_result[i]); 
        }
        free(hash_result);
    }

    destroy_tree(tree);

    // for (size_t i = 0; i <bpkg->len_chunk; i++) {
    //     free(child_nodes[i]);
    // }
    // free(child_nodes); 
 
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

