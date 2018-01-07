OPT= -O3
DBG= -g
FLAGS= -mrtm -pthread
PRGS=  tree-tsx tree-hoh #tree-read

default: all

all:${PRGS}

tree-tsx: Driver.cpp TreeHT.hpp *.h makefile
	g++ -std=c++1y ${OPT} ${DBG} ${FLAGS} -o $@ -DHEADER=TreeHT Driver.cpp 

tree-hoh: Driver.cpp TreeH.hpp *.h makefile
	g++ -std=c++1y ${OPT} ${DBG} ${FLAGS} -o $@ -DHEADER=TreeH Driver.cpp 
	
tree-read: Driver.cpp *.h *.hpp
	g++ -std=c++1y ${OPT} ${DBG} ${FLAGS} -o $@ -DHEADER=TreeNoSync Driver.cpp 


clean:
	rm ${PRGS}
