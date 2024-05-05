#include "merkletree.h"

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>


/**
 * Calculate hash value 
*/
void compute_hash(struct merkle_tree_node* node) {
    if (node == NULL) {
        return;
    }

    struct sha256_compute_data sha_hash;
    //initialise hash
    sha256_init(&sha_hash); 

    //node is leaf
    if (node->is_leaf) {
        //hash value
        sha256_update(&sha_hash, node->value, sizeof(node->value));
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

    sha256_finalize(&sha_hash, node->computed_hash);
    sha256_output_hex(&sha_hash, node->computed_hash);
}

/**
 * Creates a node
*/
struct merkle_tree_node* make_node(void* key, void* value, int is_leaf) {
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

    //dynamically allocate array to hold child nodes 
    struct merkle_tree_node** nodes = malloc(bpkg->len_chunk * sizeof(struct merkle_tree_node*));
    if (!nodes) {
        fprintf(stderr, "Failed to allocate memory for nodes\n");
        return NULL;
    }

    //make leaf nodes 
    for (size_t i = 0; i < bpkg->len_chunk; i++) {
        nodes[i] = make_node(NULL, bpkg->chunks[i].hash, 1); 
        compute_hash(nodes[i]); //note: value changed in memory
    }

    //combine nodes to create internal nodes
    size_t num_leaf = bpkg->len_chunk; 

    size_t depth = 1;
    size_t total_nodes = num_leaf;

    while (num_leaf > 1) { 
        size_t count = 0;
        for (size_t i = 0; i < num_leaf; i += 2) {
            //initialise internal node 
            struct merkle_tree_node* parent = make_node(NULL, NULL, 0); 
            parent->left = nodes[i];

            //if a right node exists (even number of nodes)
            if (i + 1 < num_leaf) { 
                parent->right = nodes[i + 1];
            } 
            //no right node (odd number of nodes)
            else {
                parent->right = NULL;
            }
            
            compute_hash(parent); 
            nodes[count++] = parent;
        }

        num_leaf = count;
        depth++;
        total_nodes += num_leaf; 

        //verify merkle tree property 
        if (count == (1 << (depth-1))) {

        } 
    }

    //dynamically allocate space to construct root 
    struct merkle_tree* tree = malloc(sizeof(struct merkle_tree));
    if (!tree) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return NULL;
    }
    tree->root = nodes[0];
    tree->n_nodes = 1;

    total_nodes += 1; 
    depth++;

    //verify merkle tree construction 
    //total_nodes = (2^d) - 1
    //num_leaf = 2^(d-1)-1
    if (total_nodes == ((1 << depth) - 1) && num_leaf == ((1 << depth-1) - 1)) {
        
    }
    else {

    }

    free(nodes);
    return tree;
}

/**
 * Destroy merkle tree
*/
void destroy_tree(struct merkle_tree* node) {
    if (node == NULL) {
        return;
    }
    destroy_tree(node->left);
    destroy_tree(node->right);

    free(node);
}


/**
 * Performs inorder traversal of merkle tree 
*/
void build_merkle_tree(struct merkle_tree* node) {
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
