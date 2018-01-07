#!/pkg/local/bin/python

# collect statistics from bencharks
# writes files to graph subdirectory
# Note: debug versions confuse statistics collection
import sys
import subprocess
import time

maxThreads = 12
numOps = 100 * 1000
numTrials = 5;
mutPcts = {20}
initialSizes = {5000}

def median(mylist):
    sorts = sorted(mylist)
    length = len(sorts)
    if not length % 2:
        return (sorts[length / 2] + sorts[length / 2 - 1]) / 2.0
    return sorts[length / 2]

if len(sys.argv) < 1:
    print "Usage: python bench.py [args...]"
    sys.exit(1)

print 'OLD script: deprecated!'
print 'collecting median of {} runs'.format(numTrials);

args = " ".join(sys.argv[2:])
pgm = sys.argv[1]
for i in initialSizes:
    for m in mutPcts:
        # map: string -> list of ints
        stats = dict(
            allocs={},
            recycled={},
            capacity={},
            conflict={},
            explicit={},
            fallback={},
            rate={},
            locked={},
            retries={},
            started={},
            unknown={}
            )
        # map: string -> list of floats
        fstats = dict(
            time={},
            avgTelDist={}
            )
        for t in range(1, maxThreads+1):
            conflict = 0
            cmd = "./" + pgm + ' -t {} -n {} -m {} -i {} {}'.format(t, numOps, m, i, args)
            start = time.time();
            print cmd,
            sys.stdout.flush()
            for trials in range(1, numTrials+1):
                lines = subprocess.check_output(cmd, shell=True)
                print ".",
                sys.stdout.flush()
                struc = lines.split("\n");
                for line in struc:
                    stat = line.split(":")
                    if stat[0] in stats.keys():
                        if t in stats[stat[0]].keys():
                            stats[stat[0]][t].append(int(stat[1]))
                        else:
                            stats[stat[0]][t] = [int(stat[1])]
                    elif stat[0] in fstats.keys():
                        if t in fstats[stat[0]].keys():
                            fstats[stat[0]][t].append(float(stat[1]))
                        else:
                            fstats[stat[0]][t] = [float(stat[1])]
                    elif stat[0] == 'error':
                        print 'error:\n\t\t{}'.format(stat[1])
                        file = open("graphs/PANIC", 'w');
                        file.write(stat[1]);
                        file.close();
                        sys.exit(0)
                    elif stat[0] == 'warning':
                        print 'warning:\n\t\t{}'.format(stat[1])
            print ""
            print 'Elapsed: {}'.format(time.time() - start)
        for k,v in stats.iteritems():
            if len(v) > 0:
                filename = 'graphs/{}-{}-m{}-i{}.out'.format(pgm, k, m, i)
                print "writing {}".format(filename)
                file = open(filename, 'w')
                for t in v.keys():
                    if len(v[t]) != numTrials:
                        print "ERROR:\t{} -> {}".format(k, v)
                        print "ERROR:\t{} length {}, expected {}".format(t, length, numTrials)
                        sys.exit(0)
                    file.write('{}\t'.format(median(v[t])))
                file.write("\n")
                file.close()
        for k,v in fstats.iteritems():
            if len(v) > 0:
                filename = 'graphs/{}-{}-m{}-i{}.out'.format(pgm, k, m, i)
                print "writing {}".format(filename)
                file = open(filename, 'w')
                for t in v.keys():
                    file.write('{}\t'.format(median(v[t])))
                file.write("\n")
                file.close()
                
