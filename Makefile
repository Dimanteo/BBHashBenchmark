CC=g++
CXXFLAGS=-O2 -MD
CXXFLAGS_DEBUG=-O0 -g

all : hashes.out

hashes.out : hashes.cpp
	$(CC) $(CXXFLAGS) hashes.cpp -o $@

debug.out : hashes.cpp
	$(CC) $(CXXFLAGS_DEBUG) hashes.cpp -o $@

.PHONY: clean

clean :
	rm -f *.out *.o *.d