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
struct merkle_tree_node* make_node(void* key, void* value, int is_leaf) {
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
void read_data(struct merkle_tree_node*** current_nodes, struct bpkg_obj* bpkg) {
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
        return;
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
            return;
        }

        //read data of size (doesnt include null byte)
        fread(hash_buffer, 1, (size_t)*bpkg->chunks_size[i], file);

        //add null terminator
        // hash_buffer[(size_t)*bpkg->chunks_size[i]] = '\0';   

        //compute hash and store   
        compute_hash(hash_buffer, (size_t)*bpkg->chunks_size[i], (*current_nodes)[i]->computed_hash);

        free(hash_buffer);
    }
}

/**
 * Constructs merkle tree (bottom up approach) from a list of chunks 
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
            char concat[2 * SHA256_HEXLEN + 1];
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

    //dynamically allocate memory for tree 
    // struct merkle_tree* tree = malloc(sizeof(struct merkle_tree));
    // if (!tree) {
    //     fprintf(stderr, "Error: Failed to allocate memory\n");
    //     for (size_t k = 0; k <= (bpkg->len_chunk+bpkg->len_hash); k++) {
    //         destroy_tree_node(current_nodes[k]);
    //     }
    //     free(current_nodes);
    //     return;
    // }

    (*tree)->root = current_nodes[0]; //last is root 
    (*tree)->n_nodes = num_nodes; 
    

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
 * Performs inorder traversal of merkle tree 
*/
void in_order_traversal(struct merkle_tree_node* node) {
    if (node == NULL) {
        return; 
    }

    ///traverse left 
    in_order_traversal(node->left);

    printf("Node computed hash: %s\n", node->computed_hash);

    //traverse right
    in_order_traversal(node->right); 
} 

/**
 * Returns all the computed hashes of leaf nodes
*/
void get_leaf_hashes(struct merkle_tree_node* node, char*** leaf_nodes, size_t* count) {
    if (node == NULL) {
        return;  
    }

    if (node->is_leaf == 1 && node->left == NULL && node->right == NULL) {
        printf("%ld %s\n",*count, node->computed_hash);
        // strdup(*leaf_nodes[*count], node->computed_hash); 
        (*leaf_nodes)[*count] = strdup(node->computed_hash);    
        (*count)++;
        return;
    }

    ///traverse left 
    get_leaf_hashes(node->left, leaf_nodes, count);

    //traverse right
    get_leaf_hashes(node->right, leaf_nodes, count);
} 
 
/**
 * Returns all the nodes of leaf nodes
*/
void get_leaf_nodes(struct merkle_tree_node* node, struct merkle_tree_node*** leaf_nodes, size_t* count) {
    if (node == NULL) {
        return; 
    }

    if (node->is_leaf == 1 && node->left == NULL && node->right == NULL) {
        // printf("%ld %s\n",*count, node->computed_hash);
        // strdup(*leaf_nodes[*count], node->computed_hash); 

        //*(leaf_nodes->computed_hash) = strdup(node->computed_hash);    
        (*count)++;
        return;
    }

    ///traverse left 
    get_leaf_nodes(node->left, leaf_nodes, count);

    //traverse right
    get_leaf_nodes(node->right, leaf_nodes, count);
} 

/**
 * Combine bpkg->hash bpkg->chunk_hash
 * function: iterate through tree
 *      call helper function to determine if hash in tree 
 *      if exist, add hash to list end traversal of current subtree 
 *      otherwise continue traverse 
*/

void get_root_complete_subtree(struct merkle_tree_node* node, struct bpkg_query* all_hashes, char*** result) {
    

}
