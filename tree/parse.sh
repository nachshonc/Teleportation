#!/bin/bash
cat graphs/tree-hoh-time-m20-i5000-.out | sed s/"\t"/" \t"/g
cat graphs/tree-tsx-time-m20-i5000-.out | sed s/"\t"/" \t"/g
cat graphs/tree-hoh-time-m20-i5000--R.out | sed s/"\t"/" \t"/g
cat graphs/tree-tsx-time-m20-i5000--R.out | sed s/"\t"/" \t"/g
