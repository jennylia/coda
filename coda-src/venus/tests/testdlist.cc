#ifdef __cplusplus
extern "C" {
#endif __cplusplus

#include "coda_assert.h"
#include <stdio.h>
#include <struct.h>

#ifdef __cplusplus
}
#endif __cplusplus

#include <dlist.h>
#include <vcrcommon.h>

#include "tallyent.h"

#ifdef TESTING
#define LOG(level, stmt)	printf stmt
#endif /* TESTING */

/* List of tallyents sorted by <priority, vuid> */
dlist *TallyList;


tallyent::tallyent(int p, vuid_t v, int b, TallyStatus s) {

    priority = p;
    vuid = v;
    incomplete = 0;
    switch(s) {
      case TSavailable:
	available_blocks = b;
        break;
      case TSunavailable:
	unavailable_blocks = b;
	break;
      case TSunknown:
	incomplete = 1;
        break;
      default:
        CODA_ASSERT(1 == 0);
        break;
    }
}

tallyent::tallyent(tallyent&) {
    abort();
}

tallyent::operator=(tallyent& i) {
    abort();
    return(0);
}

tallyent::~tallyent() {
    CODA_ASSERT(TallyList != NULL);
    TallyList->remove(&prioq_handle);
}


int tallyentPriorityFN(dlink *d1, dlink *d2) {
  tallyent *t1, *t2;

  CODA_ASSERT(d1 != NULL);
  CODA_ASSERT(d2 != NULL);

  t1 = strbase(tallyent, d1, prioq_handle);
  t2 = strbase(tallyent, d2, prioq_handle);

  CODA_ASSERT(t1 != NULL);
  CODA_ASSERT(t2 != NULL);

  if (t1->priority == t2->priority) {

    if (t1->vuid == t2->vuid)
      return(0);
    else if (t1->vuid > t2->vuid)
      return(1);
    else 
      return(-1);

  } else if (t1->priority > t2->priority)
    return(1);
  else
    return(-1);
}

void InitTally() {
  TallyList = new dlist(tallyentPriorityFN);
}


dlink *Find(int priority, vuid_t vuid) {
  dlist_iterator next(*TallyList);
  dlink *d;

  while (d = next()) {
    tallyent *te = strbase(tallyent, d, prioq_handle);
    if ((te->priority == priority) && (te->vuid == vuid)) return(d);
  }

  return(0);
}

void Tally(int priority, vuid_t vuid, int blocks, TallyStatus status) {
  tallyent *te;
  dlink *d;

  LOG(100, ("Tally: priority=%d, vuid=%d, blocks=%d, status=%d\n", 
	 priority, (int)vuid, blocks, (int)status));

  CODA_ASSERT(TallyList != NULL);
  d = Find(priority, vuid);
  if (d != NULL) {
    te = strbase(tallyent, d, prioq_handle);
    CODA_ASSERT(te != NULL);
    CODA_ASSERT(te->priority == priority);
    CODA_ASSERT(te->vuid == vuid);
    switch(status) {
      case TSavailable:
        te->available_blocks += blocks;
        break;
      case TSunavailable:
        te->unavailable_blocks += blocks;
        break;
      case TSunknown:
        te->incomplete = 1;
        break;
      default:
        CODA_ASSERT(1 == 0);
        break;
    }
    LOG(100, ("tallyent::tallyent: updated <priority=%d, vuid=%d>\n", priority, (int)vuid));
  } else {
    te = new tallyent(priority, vuid, blocks, status);
    TallyList->insert(&(te->prioq_handle));
    LOG(100, ("tallyent::tallyent: inserted <priority=%d, vuid=%d>\n", priority, (int)vuid));
  }
}

void Print(vuid_t vuid) {
  LOG(0, ("\n\nTally for vuid=%d:\n", (int)vuid));
  dlist_iterator next(*TallyList);
  dlink *d;
  while (d = next()) {
    tallyent *te = strbase(tallyent, d, prioq_handle);
    CODA_ASSERT(te != NULL);

    if (te->vuid != vuid) continue;

    float percent_available;
    percent_available = (float)te->available_blocks / (float)(te->available_blocks + te->unavailable_blocks); 

    LOG(0, ("\tPriority=%d: Available=%d Unavailable=%d PercentAvailable=%f Unknown=%d\n",
	   te->priority, te->available_blocks, te->unavailable_blocks, 
	   percent_available, te->incomplete));
  }
}

#ifdef TESTING

int main(int argc, char *argv[]) {
  InitTally();

  Tally(1000, 2660, 10, TSavailable);
  Tally(1000, 2660, 20, TSavailable);
  Tally(1000, 2660, 1000, TSavailable);

  Tally(900, 2660, 20, TSunavailable);
  Tally(900, 2660, 30, TSavailable);
  Tally(900, 2660, 40, TSavailable);
  Tally(900, 2660, 20, TSunavailable);

  Tally(825, 2660, 50, TSunavailable);
  Tally(850, 2660, 50, TSavailable);
  Tally(875, 2660, 30, TSunknown);

  Tally(1000, 2208, 400, TSavailable);
  Tally(1000, 2208, 400, TSunavailable);

  Tally(700, 2208, 234, TSavailable);
  Tally(700, 2208, 345, TSavailable);
  Tally(700, 2208, 567, TSunavailable);
  

  Print(2660);

  Print(2208);
}

#endif /* TESTING */
