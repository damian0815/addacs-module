#include "IPCTestStruct.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include "IPCTestStruct.h"

int mm_fd=-1;
IPCTestStruct* sharedData=NULL;

void cleanup()
{
	// cleanup
	if ( sharedData )
		munmap( sharedData, sizeof(IPCTestStruct ));
	sharedData = NULL;
	if ( mm_fd > 0 )
		close(mm_fd);
	mm_fd = -1;

}

void handleInterrupt()
{
	cleanup();
	exit(2);
}



int main()
{

	atexit( cleanup );
	signal( SIGINT, handleInterrupt );
	signal( SIGTERM, handleInterrupt );
	signal( SIGKILL, handleInterrupt );



	// open shared memory with address '/ipc_test', readwrite, with a+rw permissions
	mm_fd = shm_open( SHM_NAME, O_RDWR, 0 );
	if ( mm_fd < 0 )
	{
		fprintf(stderr, "shm_open failed: err %i %s\n", errno, strerror(errno));
		cleanup();
		return 1;
	}
	else
	{
		// attempt to memory map mm_fd 
		sharedData = (IPCTestStruct*)mmap(NULL, sizeof(IPCTestStruct), PROT_READ|PROT_WRITE, MAP_SHARED, mm_fd, 0);
		if ( sharedData == NULL )
		{
			fprintf(stderr, "mmap failed: err %i %s\n",  errno, strerror(errno));
			cleanup();
			return -1;
		}
	}


	while ( 1 )
	{
		sharedData->bShouldRead = 1;
		for ( int i=0; i<4 ;i ++ )
		{
			printf("  %04x", sharedData->inputs[i] );
		}
		printf("\r");
		usleep(1*1000);
	}
	
}


