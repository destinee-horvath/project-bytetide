CC=gcc
CFLAGS=-Wall -std=c2x -g -fsanitize=address
LDFLAGS=-lm -lpthread
INCLUDE=-Iinclude

OBJDIR=objs
BINDIR=bin

.PHONY: clean

# Required for Part 1 - Make sure it outputs a .o file
# to either objs/ or ./
# In your directory
$(OBJDIR):
	mkdir -p $(OBJDIR)

#all .c, .o files go to obj directory
$(OBJDIR)/%.o: src/%.c | $(OBJDIR)
	$(CC) -c $< $(INCLUDE) $(CFLAGS) -o $@

$(OBJDIR)/chk $(OBJDIR)/crypt $(OBJDIR)/tree:
	mkdir -p $@

pkgchk.o: src/chk/pkgchk.c
	$(CC) -c $^ $(INCLUDE) $(CFLAGS) $(LDFLAGS)

pkgchecker: src/pkgmain.c src/chk/pkgchk.c
	$(CC) $^ $(INCLUDE) $(CFLAGS) $(LDFLAGS) -o $@ 

pkgmain: src/pkgmain.c src/chk/pkgchk.c src/crypt/sha256.c src/tree/merkletree.c
	$(CC) $^ $(INCLUDE) $(CFLAGS) $(LDFLAGS) -o $@

merkletree: src/pkgmain.c src/chk/pkgchk.c src/crypt/sha256.c src/tree/merkletree.c
	$(CC) $^ $(INCLUDE) $(CFLAGS) $(LDFLAGS) -o $@

# Required for Part 2 - Make sure it outputs `btide` file
# in your directory ./	
btide: src/btide.c src/config.c src/package.c src/peer.c src/chk/pkgchk.c src/crypt/sha256.c src/tree/merkletree.c
	$(CC) $^ $(INCLUDE) $(CFLAGS) $(LDFLAGS) -o $@

# Alter your build for p1 tests to build unit-tests for your
# merkle tree, use pkgchk to help with what to test for
# as well as some basic functionality
p1tests:
# bash p1test.sh
	gcc src/pkgmain.c src/chk/pkgchk.c src/crypt/sha256.c src/tree/merkletree.c -Iinclude -Wall -std=c2x -g -fsanitize=address -lm -lpthread -o tests/pkgmain
	echo "Running part 1 tests: "
	chmod u+x p1test.sh
	./p1test.sh
	rm -f tests/pkgmain

# Alter your build for p2 tests to build IO tests
# for your btide client, construct .in/.out files
# and construct a script to help test your client
# You can opt to constructing a program to
# be the tests instead, however please document
# your testing methods
p2tests:
	bash p2test.sh

clean:
	rm -f $(OBJDIR)/*
	rm -f pkgchecker pkgmain merkletree btide