#!/bin/bash
echo "HandOverHand, without and with teleportation"
cat graphs/listHZ-time-m20-i5000.out | sed s/"\t"/" \t"/g
cat graphs/listHTZ-time-m20-i5000.out | sed s/"\t"/" \t"/g

echo "LazyList, With HazardPointers, with Epoch, and with Teleportation"
cat graphs/listLZ-time-m20-i5000.out | sed s/"\t"/" \t"/g
cat graphs/listLE-time-m20-i5000.out | sed s/"\t"/" \t"/g
cat graphs/listLTZ-time-m20-i5000.out | sed s/"\t"/" \t"/g

echo "LockFree, with HazardPoints, with Epoch, and with Teleportation"
cat graphs/listFZ-time-m20-i5000.out | sed s/"\t"/" \t"/g
cat graphs/listFE-time-m20-i5000.out | sed s/"\t"/" \t"/g
cat graphs/listFTZ-time-m20-i5000.out | sed s/"\t"/" \t"/g

echo "Without Memory Management: HandOverHand, Lazy, LockFree"
cat graphs/listH-time-m20-i5000.out | sed s/"\t"/" \t"/g
cat graphs/listL-time-m20-i5000.out | sed s/"\t"/" \t"/g
cat graphs/listF-time-m20-i5000.out | sed s/"\t"/" \t"/g
