#include "Driver.h"

void* thread(void* x) {
  thread_arg_t* arg = (thread_arg_t*) x;
  Set* set = arg->set;
  int mutPcnt = arg->mutPcnt;
  pthread_barrier_wait(arg->barrier);
  hrtime_t start = gethrtime();

  for (int i=0; i< arg->numOps; i++) {
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

  Set* set = new HEADER();//see makefile. Expands to the relevant set
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
  }

  // initialize set
  ThreadLocal threadLocal(0, numThreads, numOps);
  for (int i = 0; i < initial; i++) {
    thread_arg_t* arg = &args[0];
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

  for (int i = 0; i < numThreads; i ++) {
    pthread_join(threads[i], NULL);
  }

  //print out abort info
  for(int i = 0; i < numThreads; i++) {
     printf("Thread %d\n", i);
     (&args[i])->threadLocal->display();
     printf("\n\n");
  }

  printf("All Threads\n");
  ThreadLocal* collector = (&args[1])->threadLocal;
  for(int i = 1; i < numThreads; i++) {
    collector->merge((&args[i])->threadLocal);
  }
  collector->display();

  // sanity checks
  bool ok = true;
  int loBalance = 0;
  int hiBalance = 0;
  int badPresent = 0;
  int badAbsent = 0;
  int totalBalance = 0;
  for (int j = 0; j < range; j ++) {
    bool present = set->contains(j, &threadLocal);
    int balance = 0;
    for (int i = 0; i < numThreads + 1; i++) {
      balance += args[i].balanceArr[j];
    }
    totalBalance += balance;
    if (balance < 0) {
      if (verbose) {
	printf("error:\t%d has bad balance %d\n", j, balance);
      }
      loBalance++;
      ok = false;
    } else if (balance > 1) {
      if (verbose) {
	printf("error:\t%d has bad balance %d\n", j, balance);
      }
      hiBalance++;
      ok = false;
    } else if (balance == 0 && present) {
      if (verbose) {
	printf("error:\t%d unexpectedly present\n", j);
      }
      badPresent++;
      ok = false;
    } else if (balance == 1 && !present) {
      if (verbose) {
	printf("error:\t%d unexpectedly absent\n", j);
      }
      badAbsent++;
      ok = false;
    }
  }
  if (loBalance > 0) {
    printf("error:\t%d entries had balances < 0\n", loBalance);
  }
  if (hiBalance > 0) {
    printf("error:\t%d entries had balances > 1\n", hiBalance);
  }
  if (badPresent > 0) {
    printf("error:\t%d entries unexpectedly present\n", badPresent);
  }
  if (badAbsent > 0) {
    printf("error:\t%d entries unexpectedly absent\n", badAbsent);
  }
  int setSize = set->size();
  if (totalBalance != setSize) {
    printf("error:\tsize(%d) != balance(%d) difference: %d\n",
	   setSize, totalBalance, setSize - totalBalance);
    ok = false;
  } else {
    printf("size test OK\n");
  }
  printf("OK\t\n");
  if (!set->sanity()) {
    printf("warning: failed sanity check\n");
    ok = false;
  } else {
    printf("sanity test OK\n");
  }
  if (ok) {
    printf("all tests OK\n");
  } else {
    if (verbose) {
      set->display();
    }
  }
  pthread_barrier_destroy(&barrier);
}


