#include "chk/pkgchk.h"
#include "crypt/sha256.h"
#include "tree/merkletree.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define SHA256_HEX_LEN (64)


int arg_select(int argc, char** argv, int* asel, char* harg) {
	
	
	char* cursor = argv[2];
	*asel = 0;
	if(argc < 3) {
		puts("bpkg or flag not provided");
		exit(1);
	}

	if(strcmp(cursor, "-all_hashes") == 0) {
		*asel = 1;
	}
	if(strcmp(cursor, "-chunk_check") == 0) {
		*asel = 2;
	}
	if(strcmp(cursor, "-min_hashes") == 0) {
		*asel = 3;
	}
	if(strcmp(cursor, "-hashes_of") == 0) {
		if(argc < 4) {
			puts("filename not provided");
			exit(1);
		}
		*asel = 4;
		strncpy(harg, argv[3], SHA256_HEX_LEN);
	}
	if(strcmp(cursor, "-file_check") == 0) {
		if(argc < 3) {
			puts("filename not provided");
			exit(1);
		}
		*asel = 5;
		// strncpy(harg, argv[3], SHA256_HEX_LEN);
	}
	if (strcmp(cursor, "-test_bpkg") == 0) {
		*asel = 6;
	}
	if (strcmp(cursor, "-test_tree") == 0) {
		*asel = 7;
	}
	if (strcmp(cursor, "-test_data") == 0) {
		*asel = 8;
	}
	return *asel;
}


void bpkg_print_hashes(struct bpkg_query* qry) {
	for(int i = 0; i < qry->len; i++) {
		printf("%.64s\n", qry->hashes[i]);
	}	
}

void bpkg_print_all(struct bpkg_obj* obj) {
	if (obj == NULL) {
        return;
    }
	//something was wrong loading the data 
    if (obj->identifier == NULL) {
		return;
	}

	printf("%s\n", obj->identifier);
	printf("%s\n", obj->filename);
	printf("%d\n", obj->size);

	printf("%d\n", obj->len_hash);
	if (obj->hashes == NULL) {
		return;
	}
	for (size_t i = 0; i < obj->len_hash; i++) {
		printf("%s\n", obj->hashes[i]);
	}

	printf("%d\n", obj->len_chunk);
	for (size_t i = 0; i < obj->len_chunk; i++) {
		printf("%s,%d,%d\n", obj->chunks_hash[i], *obj->chunks_offset[i], *obj->chunks_size[i]);
	}
}

int main(int argc, char** argv) {

	int argselect = 0;
	char hash[SHA256_HEX_LEN];
 

	if(arg_select(argc, argv, &argselect, hash)) {
		struct bpkg_query qry = { 0 };
		struct bpkg_obj* obj = bpkg_load(argv[1]);
		
		if(!obj) {
			puts("Unable to load pkg and tree");
			exit(1);
		}

		if(argselect == 1) {
			qry = bpkg_get_all_hashes(obj);
			bpkg_print_hashes(&qry);
			bpkg_query_destroy(&qry);
		} else if(argselect == 2) { 

			qry = bpkg_get_completed_chunks(obj);
			bpkg_print_hashes(&qry); 
			bpkg_query_destroy(&qry);
		} else if(argselect == 3) {

			qry = bpkg_get_min_completed_hashes(obj);
			bpkg_print_hashes(&qry);
			bpkg_query_destroy(&qry); 
		} else if(argselect == 4) {

			qry = bpkg_get_all_chunk_hashes_from_hash(obj, hash);
			bpkg_print_hashes(&qry);
			bpkg_query_destroy(&qry); 
		} else if(argselect == 5) {

			qry = bpkg_file_check(obj);
			bpkg_print_hashes(&qry);
			bpkg_query_destroy(&qry);

		} 
		/**
		 * For testcases
		*/
		else if (argselect == 6) {
			bpkg_print_all(obj);
		}
		else if (argselect == 7) {
			test_make_tree(obj);
		}
		else if (argselect == 8) {
			test_read_data(obj);
		}
        else {
			puts("Argument is invalid");
			return 1;
		}

		bpkg_obj_destroy(obj);

	}

	return 0;
}
