
#ifndef _THREADLOCAL_H
#define _THREADLOCAL_H
#define DEFAULT_TELEPORT_DISTANCE 256
#define MIN_TEL_LIMIT 20
class ThreadLocal {
public:
  // statistics
  int numOps=0;
  int numThreads=0;
  int addRetriesShort=0;
  int addRetriesTotal=0;
  int remRetries=0;
  int threadNum;
  int tStarted=0;
  int tCommitted=0;
  int aLocked=0;
  int aInvalid=0;
  int aNotLinked=0;
  int aExplicit=0;
  int aConflict=0;
  int aCapacity=0;
  int aDebug=0;
  int aNested=0;
  int aRetries=0;
  int aUnknown=0;
  int fallback=0;
  int teleports=0;
  int teleportDistance=0;
  int allocs=0;
  int recycle=0;
  int numRetired=0;
  int scans=0;

  // thread local
  int teleportLimit=DEFAULT_TELEPORT_DISTANCE;
  void* free;
  void* retired;
  int clock=0;			// used for hazard pointers
  int toggle=0;
  int criticalCount=0;

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
  };
  void display() {
    //if(allocs!=0)
	    printf("allocs:\t\t%d\n", allocs);
    //if(recycle!=0)
	    printf("recycled:\t%d\n", recycle);
    if (tStarted > 0) {
      int aborted = (tStarted - tCommitted);
      printf("started:\t%d\n", tStarted);
      printf("committed:\t%d\n", tCommitted);
      printf("teleports:\t%d\n", teleports);
      if (teleports > 0) {
	printf("avgTelDist:\t%f\n", ((double) teleportDistance) / ((double) teleports));
      }
      printf("rate:\t\t%d\n", (100 * tCommitted) / tStarted);
      printf("fallback:\t%d\n", (100 * fallback) / teleports);
      if (aborted > 0) {
	printf("capacity:\t%d\n", (100 * aCapacity) / aborted);
	printf("conflict:\t%d\n", (100 * aConflict) / aborted);
	printf("explicit:\t%d\n", (100 * aExplicit) / aborted);
	printf("invalid:\t%d\n", (100 * aInvalid) / aborted);
	printf("locked:\t\t%d\n", (100 *  aLocked) / aborted);
	printf("notLinked:\t%d\n", (100 *  aNotLinked) / aborted);
	printf("retries:\t%d\n", (100 *  aRetries) / aborted);
	printf("unknown:\t%d\n", (100 *  aUnknown) / aborted);
      }
    }
  };
  ThreadLocal(int id, int _numThreads, int _numOps)
    : threadNum(id), numThreads(_numThreads),
      numOps(_numOps), free(NULL), retired(NULL), toggle(0) {
  };
};
#endif /*_THREADLOCAL_H */
