Part 1: 
    Loading .bpkg files (src/chk/pkgchk.c):
        - The fields of the .bpkg file are loaded into dynamically allocated space representing bpkg_obj 

    Construction of merkle tree (src/tree/merkletree.c):
        - Bottom up approach is used to build the merkle tree 
            - The .data file is used to read the hashes to be used as leaf nodes
            - The internal nodes of the tree are concatenations of the leaf nodes
            - Concept to construct tree:
                - two arrays will be used: a current and a parent 
                - two nodes stored in the current array are concatenated to create a parent node 
    
    Functions (src/chk/pkgchk.c src/tree/merkletree.c):
        - The functions are used to perform specific operations on the tree, depending on the hash entered into the commands
        - The purpose of each function is documented above each function 
        - Testcase functions are located at the bottom of pkgchk.c
    
    pkgmain.c (src/pkgmain.c): 
        - main code to handle all functions depending on arguments entered into command 

    To compile code: 
        - make pkgmain

Testing Part 1:     
    Additional flags were added to pkgmain.c to perform the tests. These call testcase functions which are located at the bottom of pkgchk.c
    The tests are located in a directory called tests. A shell script is used to iterate through all the test folders within tests. 

    To run part 1 tests: 
        make p1test 

----------------------------------------------------------------------------------------------
Part 2: 
    Loading a .cfg file (src/config.c):
        - The contents of the .cfg file are loaded into dynamically allocated space

    Creating and destroying peers (src/peers.c src/btide.c):
        - Utilises ports and sockets to connect and disconnect manage connections of peers in a network 
        - The port of the current peer and the macimum number of peers that can be created was determined from the config file 

    Creating and destroying packages (src/packages.c src/btide.c):
        - The config file is used to determine the folder in which the .bpkg and data file is located
        - Packages were created with information from the files 

    btide.c (src/btide.c):
        - main code to handle inputs and perform operations depending on command entered

    - Documentation of functions located in .c files 

    To compile code: 
        - make btide 

Testing Part 2:
    * Part 2 tests not attempted

