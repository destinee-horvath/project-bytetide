#ifndef MERKLE_TREE_H
#define MERKLE_TREE_H

#include <stddef.h>

#define SHA256_HEXLEN (64)

struct merkle_tree_node {
    void* key;
    void* value;
    struct merkle_tree_node* left;
    struct merkle_tree_node* right;
    int is_leaf; //1 if leaf otherwise 0
    char expected_hash[SHA256_HEXLEN+1];
    char computed_hash[SHA256_HEXLEN+1]; //+1 to account for null character 
};


struct merkle_tree {
    struct merkle_tree_node* root;
    size_t n_nodes;
};

#endif
