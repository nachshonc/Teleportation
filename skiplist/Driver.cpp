#include "Driver.h"

int start=0, finish=0;

void* thread(void* x) {
	thread_arg_t* arg = (thread_arg_t*) x;
	Set* set = arg->set;
	int mutPcnt = arg->mutPcnt;
	pthread_barrier_wait(arg->barrier);
	hrtime_t start = gethrtime();
	while(start==0){
		__sync_synchronize();
	}

	int i=0;
	while(!finish){
		i++;
		int n = arg->rand() % arg->range;
		int op = arg->rand() % 100;
		if (op < mutPcnt) {
			if (arg->rand() % 2) {
				bool succeed = set->add(n, arg->threadLocal);
				if (succeed) {
					arg->balance++;
					arg->balanceArr[n]++;
				}
			} else {
				bool succeed = set->remove(n, arg->threadLocal);
				if (succeed) {
					arg->balance--;
					arg->balanceArr[n]--;
				}
			}
		} else {
			set->contains(n, arg->threadLocal);
		}
	}
	arg->runTime = gethrtime() - start;
	arg->numOps = i;
}

int main(int argc, char **argv) {
	setbuf (stdout, NULL) ;
	extern char *optarg;
	extern int optopt;
	bool verbose = false;
	bool errflag = false;
	int c;
	int numThreads = 1;
	int numOps = DEFAULT_NUMOPS;
	int range = DEFAULT_RANGE;
	int mutPcnt = DEFAULT_MUT_PCNT;
	int initial = 0;
	while (( c = getopt(argc, argv, ":i:t:n:r:m:v")) != -1) {
		int n;
		switch(c){
		default:
			printf("getopt problem\n");
			exit(1);
		case 't':
			numThreads = atoi(optarg);
			if (numThreads < 1 || numThreads >1024)
				goto badarg;
			break;
		case 'n':
			numOps = atoi(optarg);
			if (numOps < 1)
				goto badarg;
			break;
		case 'v':
			verbose = true;
			break;
		case 'r':
			range = atoi(optarg);
			if (range < 1)
				goto badarg;
			break;
		case 'i':
			initial = atoi(optarg);
			if (initial < 0)
				goto badarg;
			break;
		case 'm':
			mutPcnt = atoi(optarg);
			if (mutPcnt < 0 || mutPcnt > 100)
				goto badarg;
			break;
		case ':':   /* -* without operand */
			printf("Option -%c requires an operand\n", optopt);
			errflag = true;
			break;
		case '?':
			printf("Unrecognized option: -%c\n", optopt);
			errflag = true;
			break;

			badarg:
			printf("Out of range value for option -%c\n", c);
			errflag = true;
		}
	}

	/*
	 * exit if there were command line errors
	 */
	if (errflag || argc == 1){
		printf("usage: %s -p numThreads -n numOps -r range -m mutPcnt", argv[0]);
		exit(1);
	}
	if (verbose) {
		printf ("running: %s -t %d -n %d -r %d -m %d\n",
				argv[0],
				numThreads,
				numOps,
				range,
				mutPcnt);
	}

	struct timeval* times = (struct timeval*) malloc(numThreads * sizeof(struct timeval));
	pthread_t* threads = (pthread_t*) malloc(numThreads * sizeof(pthread_t));

	pthread_barrier_t barrier;
	pthread_barrier_init(&barrier, NULL, numThreads);

	Set* set = new HEADER(); //See makefile
	thread_arg_t args[numThreads+1];

	// initialize thread arg structs
	// arg numThreads is for initialization
	for (int i = 0; i < numThreads + 1; i++) {
		thread_arg_t* arg = &args[i];
		arg->threadNum = i;
		arg->numOps = numOps / numThreads;
		arg->range = range;
		arg->balanceArr = new int[arg->range];
		arg->set = set;
		arg->barrier = &barrier;
		arg->mutPcnt = mutPcnt;
		arg->threadLocal = new ThreadLocal(i, numThreads, numOps);
		HEADER::Node* pool = new HEADER::Node[INITIAL_ALLOC];
		arg->threadLocal->free = &pool[0];
		for (int j = 0; j < INITIAL_ALLOC-1; j++) {
			pool[j].free = &pool[j+1];
		}
	}

	// initialize set
	ThreadLocal threadLocal(0, numThreads, numOps);
	for (int i = 0; i < initial; i++) {
		thread_arg_t* arg = &args[numThreads];
		int x = arg->rand() % arg->range;
		if (set->add(x, &threadLocal)) {
			arg->balance++;
			arg->balanceArr[x]++;
		}
	}

	for (int i = 0; i < numThreads; i++) {
		thread_arg_t* arg = &args[i];
		pthread_create(&threads[i], NULL, thread, (void *)arg);
	}
	sleep(0);
	start=1; __sync_synchronize();
	sleep(1);
	finish=1; __sync_synchronize();

	for (int i = 0; i < numThreads; i ++) {
		pthread_join(threads[i], NULL);
	}

	int64_t avgTime = 0;
	int64_t maxTime = 0;
	int64_t ops = 0;

	ThreadLocal summary(0, numThreads, numOps);
	for (int i = 0; i < numThreads; i ++) {
		int64_t runTime = args[i].runTime;
		maxTime = (runTime > maxTime) ? runTime : maxTime;
		avgTime += runTime;
		ops += args[i].numOps;
		summary.merge(args[i].threadLocal);
	}
	summary.display();
	avgTime /= numThreads;
	printf("Ops:\t%ld\n", ops);
	printf("Mops:\t%.2f\n", ops/1000000.0);
	printf("time:\t%lf\n", ((float)avgTime / 1000000.0));
	pthread_barrier_destroy(&barrier);
}
