#ifndef MERKLE_TREE_H
#define MERKLE_TREE_H

#include <stddef.h>
#include "chk/pkgchk.h"
#include "crypt/sha256.h"

#define SHA256_HEXLEN (64)

struct merkle_tree_node {
    void* key;
    void* value;
    struct merkle_tree_node* left;
    struct merkle_tree_node* right;
    struct merkle_tree_node* parent;
    int is_leaf; //1 if leaf otherwise 0
    char expected_hash[SHA256_HEXLEN+1];
    char computed_hash[SHA256_HEXLEN+1]; //+1 to account for null character 
};


struct merkle_tree {
    struct merkle_tree_node* root;
    size_t n_nodes; //number of nodes in tree including root 
};

struct bpkg_obj;  

struct bpkg_query;

void compute_hash(uint8_t* , size_t , char* ); 

struct merkle_tree_node* make_node(void* , void* , int);

void read_data(struct merkle_tree_node***, struct bpkg_obj*);

void build_merkle_tree(struct merkle_tree_node**, struct bpkg_obj*, struct merkle_tree** );

void destroy_tree(struct merkle_tree*);

void destroy_tree_node(struct merkle_tree_node*);

size_t verify_level(size_t , size_t);

size_t verify_tree(size_t , size_t);

void get_leaf_hashes(struct merkle_tree_node*, char***, size_t*);

void get_leaf_nodes(struct merkle_tree_node*, struct merkle_tree_node***, size_t*);

void get_root_complete_subtree(struct merkle_tree_node* , char*** , size_t* , struct bpkg_obj*);

size_t hash_exists(char* , struct bpkg_obj* );

struct merkle_tree_node* in_order_traversal(struct merkle_tree_node* , char*);

void traverse_subtree(struct merkle_tree_node* , char***, size_t*);
#endif
