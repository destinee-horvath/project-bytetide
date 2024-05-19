#include "tree/merkletree.h"
#include "crypt/sha256.h"
#include "chk/pkgchk.h"

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include <math.h>  


/**
 * Calculate hash value 
*/
void compute_hash(uint8_t* data, size_t size, char* output) {
    //initialize hash
    struct sha256_compute_data sha_hash;
    sha256_compute_data_init(&sha_hash);

    //update hash
    sha256_update(&sha_hash, (void*)data, size);

    //finalize hash
    sha256_finalize(&sha_hash, (uint8_t*)output);

    //output as hex
    sha256_output_hex(&sha_hash, output);
    output[SHA256_HEXLEN+1] = '\0';  //terminate with null 
}

/**
 * Creates a node
*/
struct merkle_tree_node* make_node(void* key, void* value, size_t is_leaf) {
    //dynamically allocate memory to store node
    struct merkle_tree_node* node = malloc(sizeof(struct merkle_tree_node));
    if (node == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory\n");
        return NULL;
    }
    node->key = key;
    node->value = value;
    node->is_leaf = is_leaf;
    node->left = NULL;
    node->right = NULL;

    //initialise to 0
    memset(node->computed_hash, 0, sizeof(node->computed_hash)); 
    memset(node->expected_hash, 0, sizeof(node->expected_hash)); 

    return node;
}

/**
 * Reads .data file
*/
size_t read_data(struct merkle_tree_node*** current_nodes, struct bpkg_obj* bpkg) {
    //construct file_path
    char *rm_last_file = strrchr(bpkg->path, '/');
    char file_path[BUFFER]; 

    if (rm_last_file != NULL) {
        int len_dir = rm_last_file - bpkg->path + 1; //+1 to include '/'
        strncpy(file_path, bpkg->path, len_dir); 
        file_path[len_dir] = '\0'; 
        strncat(file_path, bpkg->filename, BUFFER - strlen(file_path) - 1);
    } 
    else {
        strncpy(file_path, bpkg->filename, BUFFER - 1); 
        file_path[BUFFER - 1] = '\0'; 
    }

    //read .data 
    FILE *file = fopen(file_path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Error: Failed to open .data file with filename %s\n", bpkg->filename);
        return 1;
    }  
    //iterate through file and make nodes 
    for (size_t i = 0; i < bpkg->len_chunk; i++) { 
        //dynamically allocate memory to store hash pointer (malloc(bytes))
        uint8_t* hash_buffer = malloc(*bpkg->chunks_size[i]);
        if (hash_buffer == NULL) {
            fprintf(stderr, "Error: Failed to allocate memory\n");
            for (size_t k = 0; k <= i; k++) {
                destroy_tree_node(*current_nodes[k]);
            }
            free(*current_nodes);
            fclose(file);
            return 1;
        }

        //read data of size (doesnt include null byte)
        fread(hash_buffer, 1, (size_t)*bpkg->chunks_size[i], file);

        //add null terminator
        // hash_buffer[(size_t)*bpkg->chunks_size[i]] = '\0';   

        //compute hash and store   
        compute_hash(hash_buffer, (size_t)*bpkg->chunks_size[i], (*current_nodes)[i]->computed_hash);

        free(hash_buffer);
    }

    return 0;
}

/**
 * Constructs merkle tree (bottom up approach) from a list of chunks 
 * @params: 
 *      struct merkle_tree_node**   : current_nodes      | initially stores children nodes and acts as tmp space when constructing tree 
 *      struct bpkg_obj*            : bpkg               | original bpkg file 
 *      struct merkle_tree**        : tree               | constructed tree
*/
void build_merkle_tree(struct merkle_tree_node** current_nodes, struct bpkg_obj* bpkg, struct merkle_tree** tree) {
    //variables used to determine if tree is valid 
    size_t num_nodes_level = bpkg->len_chunk; //to store num nodes on a level
    size_t num_nodes = bpkg->len_chunk;
    size_t level = (size_t)(log2((size_t)bpkg->len_chunk)) + 1; //calculate height of tree 
    
    struct merkle_tree_node** parent_nodes = NULL;

    //combine nodes to create internal nodes
    //iterate by height until root is reached 
    while (num_nodes_level > 1) {
        size_t next_level_size = (num_nodes_level + 1) / 2;

        parent_nodes = malloc(next_level_size * sizeof(struct merkle_tree_node*));
        if (!parent_nodes) {
            fprintf(stderr, "Error: Failed to allocate memory\n");
            return;
        }

        //set count for nodes at current level
        size_t count = 0;

        //iterate array of current level
        for (size_t i = 0, j = 0; i < num_nodes_level; i += 2, j++) {
            struct merkle_tree_node* parent = make_node(NULL, NULL, 0);

            parent->left = current_nodes[i];
            if (parent->left != NULL) {  
                parent->left->parent = parent;
            }

            parent->right = NULL;
            if (i + 1 < num_nodes_level) {
                parent->right = current_nodes[i + 1];
                if (parent->right) {
                    parent->right->parent = parent; 
                }
            }
 
            parent_nodes[j] = parent;

            //to calculate hash 
            char concat[2 * SHA256_HEXLEN + 1] = {0};

            sprintf(concat, "%s%s", parent->left->computed_hash, parent->right->computed_hash);

            compute_hash((uint8_t*)concat, strlen(concat), parent->computed_hash);

            // printf("Parent: %s\n", parent->computed_hash);
            count++;
        }


        free(current_nodes);
        //set parent_nodes to be current_nodes 
        current_nodes = parent_nodes;

        //used to verify tree
        num_nodes_level = next_level_size;

        size_t res = verify_level(num_nodes_level, level);
        if (res == 0) {
            printf("error in constructing level %ld of tree\n", level);
        }

        num_nodes_level = count;
        num_nodes += count;
        level--;

    }   

    if (num_nodes_level == 1) {
        (*tree)->root = current_nodes[0]; //last is root  
        (*tree)->n_nodes = num_nodes;
    } 
    else {
        (*tree)->root = NULL;
        fprintf(stderr, "Error\n"); 
    }

    //verify merkle tree construction (pass in expected height)
    verify_tree(num_nodes, (size_t)(log2(bpkg->len_chunk)) + 1);
   
    free(current_nodes);
}

/**
 * Checks if the number of nodes on a level is correct 
*/
size_t verify_level(size_t num_nodes, size_t level) {
    //num_nodes in each level should be 2^(level - 1) in a perfect binary tree 
    if (num_nodes == pow(2, level - 2)) {
        return 1;
    }
    return 0;
}

size_t verify_tree(size_t num_nodes, size_t height) {
    //total num_nodes should be 2^h - 1 in a perfect binary tree 
    if (num_nodes == (size_t)(pow(2, height) - 1)) {
        return 1;
    }
    return 0;
}

/**
 * Destroy merkle tree
*/
void destroy_tree(struct merkle_tree* node) {
    if (node == NULL) {
        return;
    }
    destroy_tree_node(node->root);
    free(node);
}

void destroy_tree_node(struct merkle_tree_node* node) {
    if (node == NULL) {
        return;
    }
    destroy_tree_node(node->left);
    destroy_tree_node(node->right);

    // free(node->computed_hash); 
    free(node);
}

/**
 * - returns the hashes of the chunks of the merkle tree through leaf_nodes 
 * @params: 
 *      struct merkle_tree_node*    : node      | current node 
 *      char***  : leaf_nodes                   | all the chunks of the merkle tree
 *      size_t*  : count                        | number of chunks stored in leaf_nodes
*/
void get_leaf_hashes(struct merkle_tree_node* node, char*** leaf_nodes, size_t* count) {
    if (node == NULL) {
        return;  
    }

    if (node->is_leaf == 1 && node->left == NULL && node->right == NULL) {
        (*leaf_nodes)[*count] = strdup(node->computed_hash);
        if ((*leaf_nodes)[*count] == NULL) {
            fprintf(stderr, "Error: Memory allocation failed\n");
            return;
        }
        (*count)++;
        return;
    }
 
    ///traverse left 
    get_leaf_hashes(node->left, leaf_nodes, count);

    //traverse right
    get_leaf_hashes(node->right, leaf_nodes, count);
} 
 
/**
 * - returns the nodes of the chunks of the merkle tree through leaf_nodes 
 * @params: 
 *      struct merkle_tree_node*    : node                         | current node 
 *      struct merkle_tree_node***  : leaf_nodes                   | all the chunks of the merkle tree
 *      size_t*                     : count                        | number of chunks stored in leaf_nodes
*/
void get_leaf_nodes(struct merkle_tree_node* node, struct merkle_tree_node*** leaf_nodes, size_t* count) {
    if (node == NULL) {
        return; 
    } 

    if (node->is_leaf == 1 && node->left == NULL && node->right == NULL) {
        strcpy((*leaf_nodes[*count])->computed_hash, node->computed_hash);    
        (*count)++;
        return;
    }

    ///traverse left 
    get_leaf_nodes(node->left, leaf_nodes, count);

    //traverse right
    get_leaf_nodes(node->right, leaf_nodes, count);
} 

/**
 * - Traverses through the merkle tree to return the subtree roots of complete hashes 
 * - Marks subsequent nodes of the subtree root as visited to prevent visiting it again 
 * 
 * @params: 
 *      struct merkle_tree_node*    : node                         | current node 
 *      char***                     : result                       | stores the hashes of the subtree roots of completed hashes 
 *      size_t*                     : count                        | number of chunks stored in leaf_nodes
 *      struct bpkg_obj*            : bpkg                         | original bpkg file 
*/
void get_root_complete_subtree(struct merkle_tree_node* node, char*** result, size_t* count, struct bpkg_obj* bpkg) {
    if (node == NULL) {
        return;
    }

    //traverse left 
    get_root_complete_subtree(node->left, result, count, bpkg);

    //if parent node in result, mark subtree of parent visited 
    if (((size_t)(intptr_t)(node->value)) == 1) { //convert void* tosize_t
        // printf("internal node children have been visited\n");
        if (node->left != NULL) {
            node->left->value = ((void *)(intptr_t)(1));  
        }
        if (node->right != NULL) {
            node->right->value = ((void *)(intptr_t)(1));
        }
    }
 
    //check if hash exists 
    if (hash_exists(node->computed_hash, bpkg) && ((size_t)(intptr_t)(node->value)) == 0) {

        // printf("-- %ld %s \n", node->is_leaf, node->computed_hash);
        char** new_hashes = realloc(*result, (*count + 1) * sizeof(char*));
        if (new_hashes == NULL) {
            fprintf(stderr, "Error: Fail to allocate memory\n");
            return;
        }
        *result = new_hashes;
        (*result)[*count] = strdup(node->computed_hash); //duplicate hash 
        (*count)++;

        //if internal node mark next nodes visited to stop traversal 
        if (node->left != NULL) {
            node->left->value = ((void *)(intptr_t)(1));   //convert size_t to void
        }
        if (node->right != NULL) {
            node->right->value = ((void *)(intptr_t)(1));
        }
        return;
    }

    //traverse right 
    get_root_complete_subtree(node->right, result, count, bpkg);
}

/**
 * Helper function to determine if a hash exists within a bpkg file 
 * @params: 
 *      char*             : hash     | hash to be found in bpkg 
 *      struct bpkg_obj*  : bpkg     | original bpkg file 
 * @return: 
 *      size_t            : 1 if exists, otherwise
*/
size_t hash_exists(char* hash, struct bpkg_obj* bpkg) {
    //check if internal node 
    for (size_t i = 0; i < bpkg->len_hash; i++) {
        if (strncmp(hash, bpkg->hashes[i], HASH_SIZE-1) == 0) {
            return 1;
        }
    }

    //check if leaf 
    for (size_t i = 0; i < bpkg->len_chunk; i++) {
        if (strncmp(hash, bpkg->chunks_hash[i], HASH_SIZE-1) == 0) {
            return 1;
        }
    }
    return 0;
}

/**
 * - Performs inorder traversal of merkle tree to find hash
 * @params: 
 *      struct merkle_tree_node*    : node                         | current node 
 *      char*                       : hash                         | hash to be found 
 *      struct merkle_tree_node*    : sub_root                     | node with hash
*/
void in_order_traversal(struct merkle_tree_node* node, char* hash, struct merkle_tree_node** sub_root) {
    if (node == NULL) { 
        return; 
    }
    
    if (strncmp(node->computed_hash, hash, HASH_SIZE-2) == 0 && hash != NULL) { 
        *sub_root = node;
        return;
    }

    ///traverse left 
    in_order_traversal(node->left, hash, sub_root);

    //traverse right
    in_order_traversal(node->right, hash, sub_root);
} 

/**
 * - performs traversal of a subtree from a given node and stores hashes and chunks in hashes
 * @params: 
 *      struct merkle_tree_node*    : node          | current node 
 *      char***                     : hashes        | all the chunks and hashses of the tree
 *      size_t*                     : count         | number of chunks and hashes stored in hashes
*/
void traverse_subtree(struct merkle_tree_node* node, char*** hashes, size_t* count) {
    if (node == NULL) {
        return;
    }

    //traverse left 
    traverse_subtree(node->left, hashes, count);

    char** new_hashes = realloc(*hashes, (*count + 1) * sizeof(char*));
    if (new_hashes == NULL) {
        fprintf(stderr, "Error: Fail to allocate memory\n");
        return;
    }
    *hashes = new_hashes;
    (*hashes)[*count] = strdup(node->computed_hash); //duplicate hash 
    (*count)++;
    
    //traverse right 
    traverse_subtree(node->right, hashes, count);
}

/**
 * - performs traversal of a subtree from a given node and only stores hashes
 * @params: 
 *      struct merkle_tree_node*    : node          | current node 
 *      char***                     : hashes        | all the hashes of the tree
 *      size_t*                     : count         | number of hashes stored in hashes
*/
void traverse_subtree_hashes(struct merkle_tree_node* node, char*** hashes, size_t* count) {
    if (node == NULL) {
        return;
    }

    // printf("TRAVERSING: %s %ld\n", node->computed_hash, node->is_leaf);   
    
    //traverse left 
    traverse_subtree_hashes(node->left, hashes, count);

    //only store chunks
    if (node->is_leaf == 1) {
        //allocate new memory 
        char** new_hashes = realloc(*hashes, (*count + 1) * sizeof(char*));
        if (new_hashes == NULL) {
            fprintf(stderr, "Error: Fail to allocate memory\n");
            return; 
        }   

        *hashes = new_hashes;
        (*hashes)[*count] = strdup(node->computed_hash); //duplicate hash 
        (*count)++;
    }
    
    //traverse right 
    traverse_subtree_hashes(node->right, hashes, count);
}

/**
 * - performs traversal of a subtree from a given node only stored the children of subtree
 * @params: 
 *      struct merkle_tree_node*    : node          | current node 
 *      char***                     : hashes        | all the hashes of the chunks of the subtree
 *      size_t*                     : count         | number of chunks stored in hashes
*/
void find_subtree_chunks(struct merkle_tree_node* node, char*** hashes, size_t* count) {
    if (node == NULL) {
        return;
    }

    //traverse left 
    find_subtree_chunks(node->left, hashes, count);

    if (node->is_leaf == 1) {
        char** new_hashes = realloc(*hashes, (*count + 1) * sizeof(char*));
        if (new_hashes == NULL) {
            fprintf(stderr, "Error: Fail to allocate memory\n");
            return;
        }
        *hashes = new_hashes;
        (*hashes)[*count] = strdup(node->computed_hash); //duplicate hash 
        (*count)++;
    }

    //traverse right 
    find_subtree_chunks(node->right, hashes, count);
}

/**
 * - performs traversal of a subtree from a given node only stored the children of subtree
 * @params: 
 *      struct merkle_tree_node*    : node            | current node 
 *      struct merkle_tree_node**   : node_with_hash  | used to store the node with the hash (ret)
 *      char*                       : hash            | hash to find
*/
void get_root_from_hash(struct merkle_tree_node* node, struct merkle_tree_node** node_with_hash, char* hash) {
    if (node == NULL) {
        return;
    }

    get_root_from_hash(node->left, node_with_hash, hash);

    if (strncmp(node->computed_hash, hash, HASH_SIZE-1) == 0) {
        *node_with_hash = node;
        return;
    }

    get_root_from_hash(node->right, node_with_hash, hash);
}