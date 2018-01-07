CC = /usr/bin/g++
DEBUGFLAGS = -g
CFLAGS = $(DEBUGFLAGS) -mrtm -lm -O3 -lstdc++ -std=c++11
LDFLAGS = -pthread
DEPENDS = plat.h SpinLock.h ThreadLocal.h SkipList.h Driver.h

all:  skipL \
	skipLZ skipLZD		\
	skipLTZ skipLTZD	\
	skipHZ skipHZD		\
	skipHTZ skipHTZD


skipL: Driver.cpp SkipListL.h SkipListL.cpp HazardManager.h $(DEPENDS)
	$(CC) $(CFLAGS) $(LDFLAGS) -DHEADER=SkipListL Driver.cpp SkipListL.cpp -o skipL

skipLZ: Driver.cpp SkipListLZ.h SkipListLZ.cpp HazardManager.h $(DEPENDS)
	$(CC) $(CFLAGS) $(LDFLAGS) -DHEADER=SkipListLZ Driver.cpp SkipListLZ.cpp -o skipLZ
skipLZD: DebugDriver.cpp SkipListLZ.h SkipListLZ.cpp HazardManager.h $(DEPENDS)
	$(CC) $(CFLAGS) $(LDFLAGS) -DHEADER=SkipListLZ -DDEBUG DebugDriver.cpp SkipListLZ.cpp -o skipLZD

skipLTZ: Driver.cpp SkipListLTZ.h SkipListLTZ.cpp HazardManager.h $(DEPENDS)
	$(CC) $(CFLAGS) $(LDFLAGS) -DHEADER=SkipListLTZ Driver.cpp SkipListLTZ.cpp -o skipLTZ
skipLTZD: DebugDriver.cpp SkipListLTZ.h SkipListLTZ.cpp HazardManager.h $(DEPENDS)
	$(CC) $(CFLAGS) $(LDFLAGS) -DHEADER=SkipListLTZ -DDEBUG DebugDriver.cpp SkipListLTZ.cpp -o skipLTZD

skipHZ: DebugDriver.cpp SkipListHZ.h SkipListHZ.cpp HazardManager.h $(DEPENDS)
	$(CC) $(CFLAGS) $(LDFLAGS) -DHEADER=SkipListHZ Driver.cpp SkipListHZ.cpp -o skipHZ
skipHZD: DebugDriver.cpp SkipListHZ.h SkipListHZ.cpp HazardManager.h $(DEPENDS)
	$(CC) $(CFLAGS) $(LDFLAGS) -DHEADER=SkipListHZ -DDEBUG DebugDriver.cpp SkipListHZ.cpp -o skipHZD

skipHTZ: DebugDriver.cpp SkipListHTZ.h SkipListHTZ.cpp HazardManager.h $(DEPENDS)
	$(CC) $(CFLAGS) $(LDFLAGS) -DHEADER=SkipListHTZ Driver.cpp SkipListHTZ.cpp -o skipHTZ
skipHTZD: DebugDriver.cpp SkipListHTZ.h SkipListHTZ.cpp HazardManager.h $(DEPENDS)
	$(CC) $(CFLAGS) $(LDFLAGS) -DHEADER=SkipListHTZ -DDEBUG DebugDriver.cpp SkipListHTZ.cpp -o skipHTZD



clean:
	rm -f *.o graphs/*.out skipL \
	skipLZ skipLZD		\
	skipLTZ skipLTZD	\
	skipHZ skipHZ		\
	skipHTZ skipHTZ		\
	skipHTZD skipHZD


