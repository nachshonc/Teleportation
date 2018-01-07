#ifndef _THREADLOCAL_H
#define _THREADLOCAL_H
#define MIN_TEL_LIMIT 20
class ThreadLocal {
public:
	// statistics
	int numOps;
	int numThreads;
	int addRetriesShort = 0;
	int addRetriesTotal = 0;
	int remRetries = 0;
	int threadNum = 0;
	int tStarted = 0;
	int tCommitted = 0;
	int aLocked = 0;
	;
	int aInvalid = 0;
	int aNotLinked = 0;
	int aExplicit = 0;
	int aConflict = 0;
	int aCapacity = 0;
	int aDebug = 0;
	int aNested = 0;
	int aRetries = 0;
	int aUnknown = 0;
	int fallback = 0;
	int teleports = 0;
	int ops=0;
	unsigned long long teleportDistance = 0;
	int allocs = 0;
	int recycle = 0;

	// thread local
	int teleportLimit = 60;
	void* free;
	void* retired;
	int clock = 0;			// used for hazard pointers
	int toggle = 0;
	int criticalCount = 0;

	void merge(ThreadLocal* other) {
		addRetriesShort += other->addRetriesShort;
		addRetriesTotal += other->addRetriesTotal;
		remRetries += other->remRetries;
		tStarted += other->tStarted;
		tCommitted += other->tCommitted;
		aLocked += other->aLocked;
		aInvalid += other->aInvalid;
		aNotLinked += other->aNotLinked;
		aExplicit += other->aExplicit;
		aConflict += other->aConflict;
		aCapacity += other->aCapacity;
		aDebug += other->aDebug;
		aNested += other->aNested;
		aRetries += other->aRetries;
		aUnknown += other->aUnknown;
		fallback += other->fallback;
		teleports += other->teleports;
		teleportDistance += other->teleportDistance;
		allocs += other->allocs;
		recycle += other->recycle;
		ops+=other->ops;
	}
	;
	void display() {
		printf("allocs:\t\t%d\n", allocs);
		printf("recycled:\t%d\n", recycle);
		if (tStarted > 0) {
			int aborted = (tStarted - tCommitted);
			printf("started:\t%d\n", tStarted);
			printf("committed:\t%d (%.1f%%)\n", tCommitted, 100.0*tCommitted/tStarted);
			//teleports = tCommitted;
			printf("teleports:\t%d\n", teleports);
			if (teleports > 0) {
				printf("avgTelDist:\t%.1f\n",
						((double) teleportDistance) / ((double) teleports));
				printf("rate:\t%lld\n", (100ll * tCommitted) / tStarted);
				//printf("fallback:\t%d\n", (100 * fallback) / teleports);
				printf("fallback:\t%d\n", fallback);
				printf("tel/ops:\t%.2f\n", (double)teleports/(double)ops);
			}
			if (aborted > 0) {
				printf("Aborts:\n");
				printf("  capacity:\t%lld%%\n", (100ll * aCapacity) / aborted);
				printf("  conflict:\t%lld%%\n", (100ll * aConflict) / aborted);
				printf("  explicit:\t%lld%%\n", (100ll * aExplicit) / aborted);
				printf("    invalid:\t%lld%%\n", (100ll * aInvalid) / aborted);
				printf("    locked:\t%lld%%\n", (100ll * aLocked) / aborted);
				printf("    notLinked:\t%lld%%\n", (100ll * aNotLinked) / aborted);
				printf("  retries:\t%lld%%\n", (100ll * aRetries) / aborted);
				printf("  unknown:\t%lld%%\n", (100ll * aUnknown) / aborted);
			}
		}
	}
	;
	ThreadLocal(int id, int _numThreads, int _numOps) :
			numOps(_numOps), numThreads(_numThreads), threadNum(id), free(NULL), retired(
					NULL), toggle(0) {
	}
	;
};
#endif /*_THREADLOCAL_H */
