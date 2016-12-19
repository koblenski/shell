TGT= shell
OBJECTS=
CFLAGS= -march=native -pipe -std=c11
CXXFLAGS= -march=native -pipe -std=c++11
CC= gcc
CXX= g++
unexport VERBOSE

all: ${TGT}

${TGT}:

.PHONY: clean test

clean:
	$(RM) $(TGT) $(TGT).out

test: ${TGT}
	SHELLPATH="/usr/bin:/bin" ./shell batch.txt > output.txt
	diff output.txt golden_output.txt
	SHELLPATH="/usr/bin:/bin" VERBOSE="true" ./shell batch.txt > output.txt
	diff output.txt golden_output_verbose.txt
