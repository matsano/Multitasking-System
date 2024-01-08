#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <pthread.h>
#include "displayManager.h"
#include "iDisplay.h"
#include "iAcquisitionManager.h"
#include "iMessageAdder.h"
#include "msg.h"
#include "multitaskingAccumulator.h"
#include "debug.h"

// get rid of gettid warning
pid_t gettid(void);

// DisplayManager thread.
pthread_t displayThread;

/**
 * Display manager entry point.
 * */
static void *display( void *parameters );


void displayManagerInit(void){
	pthread_create(&displayThread, NULL, display, NULL);
}

void displayManagerJoin(void){
	pthread_join(displayThread, NULL);
} 

static void *display( void *parameters )
{
	D(printf("[displayManager]Thread created for display with id %d\n", gettid()));
	unsigned int diffCount = 0;
	while(diffCount < DISPLAY_LOOP_LIMIT){
		diffCount++;
		sleep(DISPLAY_SLEEP_TIME);
		producedCountLock();
		consumedCountLock();
		MSG_BLOCK currentSum = getCurrentSum();
		// If valid msg, prints
		if (messageCheck(&currentSum))
			messageDisplay(&currentSum);
		print(getProducedCount(), getConsumedCount());
		producedCountUnlock();
		consumedCountUnlock();
	}
	printf("[displayManager] %d termination\n", gettid());
    pthread_exit(NULL);
}