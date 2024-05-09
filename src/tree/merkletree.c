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
void compute_hash(struct merkle_tree_node* node) {
    if (node == NULL) {
        return;
    }

    struct sha256_compute_data sha_hash;

    //initialise hash
    sha256_compute_data_init(&sha_hash); 

    //node is leaf
    if (node->is_leaf) {
        //hash value
        sha256_update(&sha_hash, node->expected_hash, sizeof(node->expected_hash));
    }

    //node is not a leaf 
    else {
        //concatenate child hashes 
        unsigned char temp_hash[2 * SHA256_HEXLEN + 1];  //+1 to account for null char
        
        int index = 0;
        if (node->left != NULL) {
            memcpy(temp_hash + index, node->left->computed_hash, SHA256_HEXLEN);
            index += SHA256_HEXLEN; //prevent overlap if right child exists
        }
        if (node->right !=NULL) {
            memcpy(temp_hash + index, node->right->computed_hash, SHA256_HEXLEN);
        }
        sha256_update(&sha_hash, temp_hash, 2 * SHA256_HEXLEN);

    }

    // sha256_finalize(&sha_hash, node->computed_hash);
    // sha256_output_hex(&sha_hash, node->computed_hash);
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

    //initialise hash to 0
    memset(node->computed_hash, 0, sizeof(node->computed_hash)); 
    memset(node->expected_hash, 0, sizeof(node->expected_hash)); 

    return node;
}

/**
 * Constructs merkle tree (bottom up approach)
*/
struct merkle_tree* build_merkle_tree(struct bpkg_obj* bpkg) {
    if (bpkg == NULL || bpkg->hashes == NULL) {
        return NULL;
    }    

    char file_path[BUFFER];
    snprintf(file_path, sizeof(file_path), "resources/pkgs/%s", bpkg->filename); 

    //read .data 
    FILE *file = fopen(file_path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Error: Failed to open .data file with filename %s\n", bpkg->filename);
        return NULL;
    }

    // //get size of file 
    // struct stat sb;
    // if (fstat(file, &sb) == -1) {
    //     fprintf(stderr, "Error: Failed to get file size\n");
    //     fclose(file);
    //     return NULL;
    // }

    //dynamically allocate memory to store all nodes on last level of tree 
    struct merkle_tree_node** current_nodes = malloc(bpkg->len_chunk * sizeof(struct merkle_tree_node*));
    if (!current_nodes) {
        fprintf(stderr, "Error: Failed to allocate memory\n");
        return NULL;
    }

    //make nodes for leaves
    for (size_t i = 0; i < bpkg->len_chunk; i++) {
        current_nodes[i] = make_node(NULL, NULL, 1);
    }

    for (size_t i = 0; i < bpkg->len_chunk; i++) {
        printf("CHUNK SIZE: %ls\n", bpkg->chunks_size[i]);
    }
    

    //iterate through file and make nodes
    for (size_t i = 0; i < bpkg->len_chunk; i++) {
        //dynamically allocate memory to store hash pointer (malloc(bytes))
        char* hash_buffer = malloc(*bpkg->chunks_size[i] + 1);
        if (hash_buffer == NULL) {
            fprintf(stderr, "Error: Failed to allocate memory\n");
            for (size_t k = 0; k <= i; k++) {
                destroy_tree_node(current_nodes[k]);
            }
            free(current_nodes);
            fclose(file);
            return NULL;
        }

        //go to correct offset
        if (fseek(file, (size_t)*bpkg->chunks_offset[i], SEEK_SET) != 0) {
            fprintf(stderr, "Error: fseek failed\n");
            free(hash_buffer);
            for (size_t k = 0; k <= i; k++) {
                destroy_tree_node(current_nodes[k]);
            }
            free(current_nodes);
            fclose(file);
            return NULL;
        }

        //read data of size 
        if (fread(hash_buffer, 1, (size_t)*bpkg->chunks_size[i], file) != (size_t)*bpkg->chunks_size[i]) {
            fprintf(stderr, "Error: fread failed\n");
            free(hash_buffer);
            for (size_t k = 0; k <= i; k++) {
                destroy_tree_node(current_nodes[k]);
            }
            free(current_nodes);
            fclose(file);
            return NULL;
        }

        //add null terminator
        hash_buffer[(size_t)*bpkg->chunks_size[i]] = '\0';

        //store in node
        strncpy(current_nodes[i]->computed_hash, hash_buffer, SHA256_HEXLEN);
        current_nodes[i]->computed_hash[SHA256_HEXLEN] = '\0';  //terminate hash with null character 

        //compute hash read
        compute_hash(current_nodes[i]);

        free(hash_buffer);
    }

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
            parent->right = NULL;      

            //if right child exists
            if (i + 1 < num_nodes_level) {
                parent->right = current_nodes[i + 1];
            }

            compute_hash(parent);
            parent_nodes[j] = parent;
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
    struct merkle_tree* tree = malloc(sizeof(struct merkle_tree));
    if (!tree) {
        fprintf(stderr, "Error: Failed to allocate memory\n");
        for (size_t k = 0; k <= (bpkg->len_chunk+bpkg->len_hash); k++) {
            destroy_tree_node(current_nodes[k]);
        }
        free(current_nodes);
        return NULL;
    }

    tree->root = current_nodes[0]; //last is root 
    tree->n_nodes = num_nodes; 
    

    //verify merkle tree construction (pass in expected height)
    verify_tree(num_nodes, (size_t)(log2(bpkg->len_chunk)) + 1);
   
    fclose(file);
    free(current_nodes);

    return tree;
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
 * Inserts nodes into merkle tree 
*/

/**
 * Deletes nodes from merkle tree 
*/
