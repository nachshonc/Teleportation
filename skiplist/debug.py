#!/pkg/local/bin/python

# main data structure:
# map: statistic -> (map: numThreads -> list of integers)

import sys
import subprocess
import time

maxThreads = 12
numOps = 100 * 1000
numTrials = 5;
mutPcnt = 50

if len(sys.argv) < 2:
    print "Usage: python debug.py pgm [args...]"
    sys.exit(1)

print 'testing {} runs'.format(numTrials);

pgm  = sys.argv[1]
args = " ".join(sys.argv[2:])
for t in range(1, maxThreads+1):
	cmd = "./" + pgm + ' -t {} -n {} -m {} -i 5000 {}'.format(t, numOps, mutPcnt, args)
	print cmd,
        sys.stdout.flush()
	for i in range(1, numTrials+1):
		lines = subprocess.check_output(cmd, shell=True)
		print ".",
                sys.stdout.flush()
		struc = lines.split("\n");
		for line in struc:
			stat = line.split(":")
			if stat[0] == 'error':
				print '\nerror:\n\t\t{}'.format(stat[1])
			elif stat[0] == 'warning':
				print '\nwarning:\n\t\t{}'.format(stat[1])
	print ""
