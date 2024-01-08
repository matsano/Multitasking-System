#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h> 
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include "messageAdder.h"
#include "msg.h"
#include "iMessageAdder.h"
#include "multitaskingAccumulator.h"
#include "iAcquisitionManager.h"
#include "debug.h"

// get rid of gettid warning
pid_t gettid(void);

//consumer thread
pthread_t consumer;
//Message computed
volatile MSG_BLOCK out;
//Consumer count storage
volatile unsigned int consumeCount = 0;

/**
* Semaphores and Mutex
*/
pthread_mutex_t mutex_cons_cnt = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

/**
 * Increments the consume count.
 */
static void incrementConsumeCount(void);

/**
 * Consumer entry point.
 */
static void *sum( void *parameters );


MSG_BLOCK getCurrentSum(){
	return out;
}

unsigned int getConsumedCount(){
	unsigned tmp;
	pthread_mutex_lock(&mutex_cons_cnt);
	tmp = consumeCount;
	pthread_mutex_unlock(&mutex_cons_cnt);
	return tmp;
}

static void incrementConsumeCount(void){
	pthread_mutex_lock(&mutex_cons_cnt);
	consumeCount++;
	pthread_mutex_unlock(&mutex_cons_cnt);
}

void messageAdderInit(void){
	out.checksum = 0;
	for (size_t i = 0; i < DATA_SIZE; i++)
	{
		out.mData[i] = 0;
	}
	pthread_create(&consumer, NULL, sum, NULL);
}

void messageAdderJoin(void){
	pthread_join(consumer, NULL);
}

unsigned int consumedCountLock(void)
{
	return pthread_mutex_lock(&mutex_cons_cnt);
}

unsigned int consumedCountUnlock(void)
{
	return pthread_mutex_unlock(&mutex_cons_cnt);
}

static void *sum( void *parameters )
{
	D(printf("[messageAdder]Thread created for sum with id %d\n", gettid()));
	unsigned int i = 0;
	while(i<ADDER_LOOP_LIMIT){
		i++;
		sleep(ADDER_SLEEP_TIME);
		MSG_BLOCK newMessage = getMessage();
		messageAdd(&out, &newMessage);
		incrementConsumeCount();
	}
	printf("[messageAdder] %d termination\n", gettid());
	pthread_exit(NULL);
}


