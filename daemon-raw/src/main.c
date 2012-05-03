#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "IPCTestStruct.h"


int mm_fd;
IPCTestStruct* sharedData = 0;
int shouldStop = 0;

void cleanup()
{

	// cleanup
	munmap( sharedData, sizeof(IPCTestStruct ));
	close(mm_fd);
	shm_unlink( SHM_NAME );
	printf("cleaned up\n");
}

void interrupt()
{
	shouldStop = 1;
}

int main( int argc, char**argv )
{
	int foreground = 0;
	for ( int i=1; i<argc; i++ )
		if ( strcmp(argv[i],"-f")==0 )
			foreground = 1;

	if ( !foreground )
	{
		if ( fork() != 0 ) 
			return 0;
	}

	atexit( cleanup );
 	signal(SIGINT, interrupt );
	signal(SIGTERM, interrupt );
	signal(SIGSEGV, interrupt );

	printf("deamon running, shared memory address is %s\n", SHM_NAME );
	// open shared memory with address '/ipc_test', readwrite, with a+rw permissions

	mm_fd = shm_open( SHM_NAME, O_RDWR|O_CREAT, S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH );
	if ( mm_fd < 0 )
	{
		fprintf(stderr,"shm_open failed: err %i %s\n", errno, strerror(errno));
		return 1;
	}
	
	// resize the fd to make space for the data
	int result = ftruncate( mm_fd, sizeof(IPCTestStruct) );
	if ( result < 0 )
	{
		fprintf(stderr,"ftruncate failed: err %i %s\n", errno, strerror(errno));
		close( mm_fd );
		shm_unlink( SHM_NAME );
		return 1;
	}

	// attempt to memory map mm_fd 
	sharedData = (IPCTestStruct*)mmap(NULL, sizeof(IPCTestStruct), PROT_READ|PROT_WRITE, MAP_SHARED, mm_fd, 0);
	if ( sharedData == NULL )
	{
		fprintf(stderr,"mmap failed: err %i %s\n", errno, strerror(errno));
		close( mm_fd );
		shm_unlink( SHM_NAME );
		return 1;
	}

	
	int readCount = 0;
	
	// main loop
	while ( !shouldStop )
	{
		if ( sharedData->bShouldRead )
		{
			readCount = 1024;
			sharedData->bShouldRead = 0;
		}
		if ( readCount > 0 )
		{
			for ( int i=0; i<4; i++ )
				sharedData->inputs[i] = sharedData->readSequenceId;
			sharedData->readSequenceId++;
			readCount--;
		}
		
		usleep( 1*1000 );
	}
	
	}

