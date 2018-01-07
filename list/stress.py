#!/pkg/local/bin/python

import sys
import subprocess
import time

maxThreads = 8
numOps = 1000
numTrials = 5;
mutPcts = {0, 20, 50}
initialSizes = {2000}
programs = {"listHZD", "listHTZD",
	"listLZD", "listLTZD",
	"listFZD", "listFTZD",
	"listFED", "listLED"
}

def median(mylist):
    sorts = sorted(mylist)
    length = len(sorts)
    if not length % 2:
        return (sorts[length / 2] + sorts[length / 2 - 1]) / 2.0
    return sorts[length / 2]

if len(sys.argv) < 1:
    print "Usage: python driver.py [args...]"
    sys.exit(1)

for pgm in programs:
    for i in initialSizes:
        for m in mutPcts:
            for t in range(1, maxThreads+1):
                conflict = 0
                cmd = "./" + pgm + ' -t {} -n {} -m {} -i {}'.format(t, numOps, m, i)
                start = time.time();
                print cmd,
                sys.stdout.flush()
                for trials in range(1, numTrials+1):
                    lines = subprocess.check_output(cmd, shell=True)
                    print ".",
                    sys.stdout.flush()
                print ""
