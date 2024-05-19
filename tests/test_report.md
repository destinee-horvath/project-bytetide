SID: 530490838 
Date: Sat May 11 08:09:20 PM AEST 2024
AI used: No
AI link: -

Test 1 : Loading an existent bpkg file 
    Description:
        - Functions: bpkg_load(), bpkg_print_all()
        - Tests whether the bpkg_load() function can load an existing bpkg file and if all the contents stored are correct 
        - .bpkg file has some fields with characters exceeding maximum 
    Input: 
        - A path to the .bpkg file is passed to the bpkg_load() function
    Output:
        - All the contents of the .bpkg file printed with a new function in pkgmain bpkg_print_all()


Test 2: Loading a non-existent bpkg file 
    Description: 
        - Functions: bpkg_load(), bpkg_print_all()
        - Tests whether the bpkg_load() function can determine if:
            - a bpkg file doesnt exist and return an empty object 
            - a path to a bpkg file doesnt exist
    Input: 
        - A path to a non-existent .bpkg file and non-existent folder is passed to the bpkg_load() function
    Output: 
        - A empty .out file since no bpkg object is created 

Test 3: 
    Description: 
        - Function: bpkg_load(), bpkg_print_all()
        - Tests loading an existing bpkg file where there is more nhash than hashes 
        - Used to determine whether the bpkg_load() will still execute regardless of the nhash provided 
    Input: 
        - A path to an existing .bpkg file is passed to the bpkg_load() 
    Output: 
        - The object file should only contain its identifier, filename, size, and nhashes (hashes are all freed since it is invalid)

Test 4: 
    Description: 
        - Function: bpkg_load(), bpkg_print_all()
        - Tests loading an existing bpkg file with no contents and only fields
    Input: 
        - A path to a .bpkg file containing only fields with no contents 
    Output: 
        - The bpkg object should hold the initialised values (for example nhashes is 0)

Test 5: 
    Description: 
        - Function: bpkg_load(), bpkg_print_all()
        - Tests loading an existing large bpkg file many hashes and many chunks 
    Input: 
        - A path to an existing .bpkg file passed to bpkg_load() 
    Output: 
        - All contents of the bpkg file 

Test 6: 
    Description: 
        - Function: load_bpkg(), build_merkle_tree(), verify_level(), verify_tree(), in_order_traversal()
        - Tests constructing a perfect merkle tree 
        - Done by verifying merkle tree properties with verification functions 
    Input: 
        - Chunks from .bpkg file to construct tree 
    Output: 
        - Root of tree: if tree is correct, the all other values should match 

Test 7: 
    Description: 
        - Functions: load_bpkg(), build_merkle_tree(), verify_level(), verify_tree(), in_order_traversal()
        - Test constructing a complete merkle tree 
        - Done by verifying merkle tree properties with verification functions 
    Input: 
        - Chunks from .bpkg file to construct tree 
    Output: 
        - Root of tree: if tree is correct, the all other values should match 

Test 8: 
    Description:
        - Functions: read_data()
        - Tests whether the code will work with an empty .data file 
    Input: 
        - An empty .data file 
    Output: 
        - Nothing since no tree is constructed 

Test 9: 
    Description: 
        - Functions: load_bpkg(), build_merkle_tree(), in_order_traversal()
        - Testing construction of a merkle tree that only consists of a root (1 chunk and no hashes)
    Input: 
        - One chunk 
    Output:
        - Same chunk 


Test 10: 
    Description: 
        - Functions: read_data()
        - Tests whether the build_merkle_tree() function will detect a corruption in the .data file 
    Input: 
        - .data file 
    Output: 
        - Nothing since file is corrupt 


IMPLEMENTATION: 
    - New conditions are added to the pkgmain.c main() function to test some functions such as printing everything loaded from the .bpkg file form load_bpkg()
    - For all tests, the code is run using the .sh script where each script has a different command.
    - When the code is run, the output will be printed into a .out file 
    - This .out file is compared with the .expected file to determine if all the outputs are correct    
        - The number of lines in the .out and .expected files are compared 
        - For each line in the .out file, each line in the .expected is checked to ensure that all the hashes are produced 
    
    - To generate chunks:
        - For loading bpkg tests, a python code was written to generate random hashes
        - For merkle tree tests, in terminal: echo -n A | shasum -a 256

    - To compute hashes:
        - A function was written to concatenate two hashes 