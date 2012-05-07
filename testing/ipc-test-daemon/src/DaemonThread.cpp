//
//  DaemonThread.cpp
//  ipc-test-daemon
//
//  Created by Damian Stewart on 03.05.12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "DaemonThread.h"



void DaemonThread::threadedFunction()
{
	// open shared memory with address '/ipc_test', readwrite, with a+rw permissions
	int mm_fd = shm_open( SHM_NAME, O_RDWR|O_CREAT, S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH );
	if ( mm_fd < 0 )
	{
		ofLogError("DaemonThread","shm_open failed: err "+ofToString(errno)+" "+strerror(errno));
		return;
	}
	
	// resize the fd to make space for the data
	int result = ftruncate( mm_fd, sizeof(IPCTestStruct) );
	if ( result < 0 )
	{
		ofLogError("DaemonThread","ftruncate failed: err "+ofToString(errno)+" "+strerror(errno));
		close( mm_fd );
		shm_unlink( SHM_NAME );
		return;
	}

	// attempt to memory map mm_fd 
	sharedData = (IPCTestStruct*)mmap(NULL, sizeof(IPCTestStruct), PROT_READ|PROT_WRITE, MAP_SHARED, mm_fd, 0);
	if ( sharedData == NULL )
	{
		ofLogError("DaemonThread","mmap failed: err "+ofToString(errno)+" "+strerror(errno));
		close( mm_fd );
		shm_unlink( SHM_NAME );
		return;
	}

	
	readCount = 0;
	
	// main loop
	while ( isThreadRunning() )
	{
		if ( sharedData->shouldRead )
		{
			readCount = 1024;
			sharedData->shouldRead = false;
		}
		if ( readCount > 0 )
		{
			for ( int i=0; i<4; i++ )
				sharedData->inputs[i] = ofRandom(INT_MIN, INT_MAX);
			sharedData->readSequenceId++;
			readCount--;
		}
		
		ofSleepMillis( 1 );
	}
	
	// cleanup
	munmap( sharedData, sizeof(IPCTestStruct ));
	sharedData = NULL;
	close(mm_fd);
	shm_unlink( SHM_NAME );
	
}


void DaemonThread::draw()
{
	if ( !ofThread::isMainThread() )
	{
		ofLogError("DaemonThread","draw must be run from main thread");
		return;
	}

	if ( sharedData )
	{
		sharedData->draw( 10, 14 );
	}
}

