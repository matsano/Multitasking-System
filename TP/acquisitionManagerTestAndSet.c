#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <fcntl.h>
#include <stdatomic.h>
#include "acquisitionManager.h"
#include "msg.h"
#include "iSensor.h"
#include "multitaskingAccumulator.h"
#include "iAcquisitionManager.h"
#include "debug.h"

// get rid of gettid warning
pid_t gettid(void);

#define SEM_FULL_NAME "/aquisitionManager_semFull"
#define SEM_EMPTY_NAME "/aquisitionManager_semEmpty"

//producer count storage
volatile unsigned int produceCount = 0;

//FIFO storage
volatile MSG_BLOCK buf[BUFFER_SIZE];
//FIFO indices for indice storage
volatile unsigned ilibre = 0;
volatile unsigned iplein = 0;
volatile unsigned jlibre = 0;
volatile unsigned jplein = 0;
//FIOF indice storage
volatile unsigned itabLibre[BUFFER_SIZE];
volatile unsigned itabPlein[BUFFER_SIZE];

pthread_t producers[4];

static void *produce(void *params);

/**
* Semaphores and Mutex
*/
volatile unsigned int mutex_prod_cnt = 1;
sem_t *sem_nFull;
sem_t *sem_nEmpty;
pthread_mutex_t mutex_ilibre = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_iplein = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_jlibre = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_jplein = PTHREAD_MUTEX_INITIALIZER;

/*
* Creates the synchronization elements.
* @return ERROR_SUCCESS if the init is ok, ERROR_INIT otherwise
*/
static unsigned int createSynchronizationObjects(void);

static unsigned int createSynchronizationObjects(void)
{
	sem_unlink(SEM_FULL_NAME);
    sem_nFull = sem_open(SEM_FULL_NAME, O_CREAT, 0644, 0);
    if (sem_nFull == SEM_FAILED)
    {
        perror("[sem_open");
        return ERROR_INIT;
    }
	sem_unlink(SEM_EMPTY_NAME);
    sem_nEmpty = sem_open(SEM_EMPTY_NAME, O_CREAT, 0644, BUFFER_SIZE);
    if (sem_nEmpty == SEM_FAILED)
    {
        perror("[sem_open");
        return ERROR_INIT;
    }
	printf("[acquisitionManager]Semaphores created\n");
	return ERROR_SUCCESS;
}

/**
* Increments the produce count.
*/
static void incrementProducedCount(void);

static void incrementProducedCount(void)
{
	pCountLockTake();
	produceCount++;
	pCountLockRelease();
}

unsigned int getProducedCount(void)
{
	unsigned int p;
	p = produceCount;
	return p;
}

MSG_BLOCK getMessage(void){
	MSG_BLOCK tmp;
	unsigned iloc;
	sem_wait(sem_nFull);
	pthread_mutex_lock(&mutex_jplein);
		iloc = itabPlein[jplein];
		jplein = (jplein+1)%BUFFER_SIZE;
	pthread_mutex_unlock(&mutex_jplein);
	tmp = buf[iloc];
	pthread_mutex_lock(&mutex_jlibre);
		itabLibre[jlibre] = iloc;
		jlibre = (jlibre+1)%BUFFER_SIZE;
	pthread_mutex_unlock(&mutex_jlibre);
	sem_post(sem_nEmpty);
	return tmp;
}

unsigned int producedCountLock(void)
{
	return pCountLockTake();
}

unsigned int producedCountUnlock(void)
{
	return pCountLockRelease();
}

int pCountLockTake(){
	unsigned result = 0;
	while (1)
	{
		unsigned expected = mutex_prod_cnt;
		if(expected == 1) // mutex seems avaliable
			result = atomic_compare_exchange_weak(&mutex_prod_cnt, &expected, 0);
			if(result)
				break;
				// got mutex
	}
	return ERROR_SUCCESS;
}

int pCountLockRelease(){
	unsigned result = 0;
	while (1)
	{
		unsigned expected = mutex_prod_cnt;
		if(expected == 0){ // mutex seems taken
			result = atomic_compare_exchange_weak(&mutex_prod_cnt, &expected, 1);
			if(result)
				break;
				// got mutex
		}else{
			return ERROR_MUTEX_RELEASE;
		}
	}
	return ERROR_SUCCESS;
}

unsigned int acquisitionManagerInit(void)
{
	unsigned int i;
	
	printf("[acquisitionManager]Synchronization initialization in progress...\n");
	fflush( stdout );
	if (createSynchronizationObjects() == ERROR_INIT)
		return ERROR_INIT;
	
	printf("[acquisitionManager]Synchronization initialization done.\n");

	for (i = 0; i < PRODUCER_COUNT; i++)
	{
		pthread_create(&producers[i], NULL, produce, NULL);
	}

	return ERROR_SUCCESS;
}

void acquisitionManagerJoin(void)
{
	unsigned int i;
	for (i = 0; i < PRODUCER_COUNT; i++)
	{
		pthread_join(producers[i], NULL);
	}

	sem_destroy(sem_nEmpty);
	sem_destroy(sem_nFull);
	printf("[acquisitionManager]Semaphore cleaned\n");
}

void *produce(void* params)
{
	D(printf("[acquisitionManager]Producer created with id %d\n", gettid()));
	unsigned int i = 0;
	while (i < PRODUCER_LOOP_LIMIT)
	{
		i++;
		sleep(PRODUCER_SLEEP_TIME+(rand() % 5));
		unsigned iloc;
		sem_wait(sem_nEmpty);
		pthread_mutex_lock(&mutex_ilibre);
			iloc = itabLibre[ilibre];
			ilibre = (ilibre+1)%BUFFER_SIZE;
		pthread_mutex_unlock(&mutex_ilibre);
		getInput(gettid(),&buf[iloc]);
		if (messageCheck(&buf[iloc]))
		{
			pthread_mutex_lock(&mutex_iplein);
				itabPlein[iplein] = iloc;
				iplein = (iplein+1)%BUFFER_SIZE;
			pthread_mutex_unlock(&mutex_iplein);
			incrementProducedCount();
			sem_post(sem_nFull);
		}
		else{
			// Discard falty message
		}
	}
	printf("[acquisitionManager] %d termination\n", gettid());
	pthread_exit(NULL);
}