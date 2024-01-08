#ifndef ACQUISITION_MANAGER_H
#define ACQUISITION_MANAGER_H

/**
* Initializes the acquisitions
*/
unsigned int acquisitionManagerInit(void);

/**
* Waits that acquisitions terminate
*/
void acquisitionManagerJoin(void);

/**
* Returns number os produced mensages
*/
unsigned int getProducedCount(void);

/**
* Locks ProducedCount Mutex
*/
unsigned int producedCountLock(void);

/**
* Unlocks ProducedCount Mutex
*/
unsigned int producedCountUnlock(void);

#endif