CC = /usr/bin/g++
DEBUGFLAGS = -g
CFLAGS = $(DEBUGFLAGS) -mrtm -lm -O3 -lstdc++ -std=c++11 -DUSE_HPs
LDFLAGS = -pthread
DEPENDS = plat.h SpinLock.h ThreadLocal.h List.h Driver.h

all: listH listHZ listHTZ \
	listL listLZ listLTZ \
	listF listFZ listFTZ \
	listFE listLE

#all: listHZ listHZD listHTZ listHTZD \
	listLZ listLZD listLTZ listLTZD \
	listFZ listFZD listFTZ listFTZD	\
	listFE listFED listLE listLED

listH: Driver.cpp ListHZ.h ListHZ.cpp SimpleManager.h $(DEPENDS)
	$(CC) $(CFLAGS) $(LDFLAGS) -DDISABLE_MM -DHAND_OVER_HAND -DHEADER=ListHZ Driver.cpp ListHZ.cpp -o listH
listHZ: Driver.cpp ListHZ.h ListHZ.cpp SimpleManager.h $(DEPENDS)
	$(CC) $(CFLAGS) $(LDFLAGS) -DHEADER=ListHZ -DHAND_OVER_HAND Driver.cpp ListHZ.cpp -o listHZ
listHZD: DebugDriver.cpp ListHZ.h ListHZ.cpp SimpleManager.h $(DEPENDS)
	$(CC) $(CFLAGS) $(LDFLAGS) -DHEADER=ListHZ -DDEBUG DebugDriver.cpp ListHZ.cpp -o listHZD

listHTZ: Driver.cpp ListHTZ.h ListHTZ.cpp SimpleManager.h $(DEPENDS)
	$(CC) $(CFLAGS) $(LDFLAGS) -DHEADER=ListHTZ -DHAND_OVER_HAND Driver.cpp ListHTZ.cpp -o listHTZ
listHTZD: DebugDriver.cpp ListHTZ.h ListHTZ.cpp SimpleManager.h $(DEPENDS)
	$(CC) $(CFLAGS) $(LDFLAGS) -DHEADER=ListHTZ -DDEBUG DebugDriver.cpp ListHTZ.cpp -o listHTZD

listL: Driver.cpp ListLZ.h ListLZ.cpp HazardManager.h $(DEPENDS)
	$(CC) $(CFLAGS) $(LDFLAGS) -DDISABLE_MM -DHEADER=ListLZ Driver.cpp ListLZ.cpp -o listL
listLZ: Driver.cpp ListLZ.h ListLZ.cpp HazardManager.h $(DEPENDS)
	$(CC) $(CFLAGS) $(LDFLAGS) -DHEADER=ListLZ Driver.cpp ListLZ.cpp -o listLZ
listLZD: DebugDriver.cpp ListLZ.h ListLZ.cpp HazardManager.h $(DEPENDS)
	$(CC) $(CFLAGS) $(LDFLAGS) -DHEADER=ListLZ -DDEBUG DebugDriver.cpp ListLZ.cpp -o listLZD

listLTZ: Driver.cpp ListLTZ.h ListLTZ.cpp HazardManager.h $(DEPENDS)
	$(CC) $(CFLAGS) $(LDFLAGS) -DHEADER=ListLTZ Driver.cpp ListLTZ.cpp -o listLTZ
listLTZD: DebugDriver.cpp ListLTZ.h ListLTZ.cpp HazardManager.h $(DEPENDS)
	$(CC) $(CFLAGS) $(LDFLAGS) -DHEADER=ListLTZ -DDEBUG DebugDriver.cpp ListLTZ.cpp -o listLTZD

listF: Driver.cpp ListFZ.h ListFZ.cpp HazardManager.h $(DEPENDS)
	$(CC) $(CFLAGS) $(LDFLAGS) -DDISABLE_MM -DHEADER=ListFZ Driver.cpp ListFZ.cpp -o listF
listFZ: Driver.cpp ListFZ.h ListFZ.cpp HazardManager.h $(DEPENDS)
	$(CC) $(CFLAGS) $(LDFLAGS) -DHEADER=ListFZ Driver.cpp ListFZ.cpp -o listFZ
listFZD: DebugDriver.cpp ListFZ.h ListFZ.cpp HazardManager.h $(DEPENDS)
	$(CC) $(CFLAGS) $(LDFLAGS) -DHEADER=ListFZ -DDEBUG DebugDriver.cpp ListFZ.cpp -o listFZD

listFXZ: Driver.cpp ListFXZ.h ListFXZ.cpp HazardManager.h $(DEPENDS)
	$(CC) $(CFLAGS) $(LDFLAGS) -DHEADER=ListFXZ Driver.cpp ListFXZ.cpp -o listFXZ
listFXZD: DebugDriver.cpp ListFXZ.h ListFXZ.cpp HazardManager.h $(DEPENDS)
	$(CC) $(CFLAGS) $(LDFLAGS) -DHEADER=ListFXZ -DDEBUG DebugDriver.cpp ListFXZ.cpp -o listFXZD

listFTZ: Driver.cpp ListFTZ.h ListFTZ.cpp HazardManager.h $(DEPENDS)
	$(CC) $(CFLAGS) $(LDFLAGS) -DHEADER=ListFTZ Driver.cpp ListFTZ.cpp -o listFTZ
listFTZD: DebugDriver.cpp ListFTZ.h ListFTZ.cpp HazardManager.h $(DEPENDS)
	$(CC) $(CFLAGS) $(LDFLAGS) -DHEADER=ListFTZ -DDEBUG DebugDriver.cpp ListFTZ.cpp -o listFTZD

listFE: Driver.cpp ListFE.h ListFE.cpp EpochManager.h $(DEPENDS)
	$(CC) $(CFLAGS) $(LDFLAGS) -DHEADER=ListFE Driver.cpp ListFE.cpp -o listFE
listFED: DebugDriver.cpp ListFE.h ListFE.cpp EpochManager.h $(DEPENDS)
	$(CC) $(CFLAGS) $(LDFLAGS) -DHEADER=ListFE -DDEBUG DebugDriver.cpp ListFE.cpp -o listFED

listLE: Driver.cpp ListLE.h ListLE.cpp EpochManager.h $(DEPENDS)
	$(CC) $(CFLAGS) $(LDFLAGS) -DHEADER=ListLE Driver.cpp ListLE.cpp -o listLE
listLED: DebugDriver.cpp ListLE.h ListLE.cpp EpochManager.h $(DEPENDS)
	$(CC) $(CFLAGS) $(LDFLAGS) -DHEADER=ListLE -DDEBUG DebugDriver.cpp ListLE.cpp -o listLED

clean:
	rm -f *.o graphs/*.out \
	listHZ listHZD listHTZ listHTZD \
	listLZ listLZD listLTZ listLTZD \
	listFZ listFZD listFTZ listFTZD \
	listFE listFED listLE listLED \
	listL listF listH

