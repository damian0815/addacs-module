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
uint8_t copyInToOut = 0;

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



int main( int argc, char**argv )
{

	for ( int i=1; i<argc; i++ )
	{
		if ( strcmp(argv[i],"-c")==0 )
			copyInToOut = 1;
		else 
		{
			fprintf(stderr,"bad argument: %s", argv[i]);
			exit(1);
		}
	}
	

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
		for ( int invisibl=0; invisibl<10; invisibl++ ) {
			sharedData->bShouldRead = 1;
			for ( int i=0; i<4 ;i++ )
			{
				sharedData->outputs[i]=((sharedData->outputs[i]+1)*!copyInToOut)
					+ sharedData->inputs[i]*copyInToOut;
			}

			usleep(1*10);
		}
		
		printf("in: ");
		for ( int i=0; i<4 ;i ++ )
		{
			printf("  %04x", sharedData->inputs[i] );
		}
		printf(" out: ");
		for ( int i=0; i<4 ;i++ )
		{
			printf("  %04x", sharedData->outputs[i] );
		}

		printf("\r");
		
	}
	
}


