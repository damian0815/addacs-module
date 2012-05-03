#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include "IPCTestStruct.h"


int i2c_fd = -1;
int mm_fd = -1;
IPCTestStruct* sharedData = 0;
int shouldStop = 0;

void cleanup()
{

	// cleanup
	if ( i2c_fd )
		close( i2c_fd );
	if ( sharedData )
		munmap( sharedData, sizeof(IPCTestStruct ));
	if ( mm_fd )
	{
		close(mm_fd);
		shm_unlink( SHM_NAME );
	}
	i2c_fd = -1;
	mm_fd = -1;
	sharedData = 0;

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

	
	// open the i2c bus
	char * filename = "/dev/i2c-2";
	if ( (i2c_fd=open(filename, O_RDWR))<0 )
	{	
		fprintf(stderr, "error opening i2c-2: %d %s\n", errno, strerror(errno) );
		return 1;
	}

	// nunchuck address
	int addr=0x52; 
	if ( ioctl(i2c_fd, I2C_SLAVE, addr) < 0 )
	{
		fprintf(stderr, "error assigning I2C slave address %x: %d %s", addr, errno, strerror(errno) );
		cleanup();
		return 1;
	}

	
	// handshake with nunchuck
	char buf[256];
	buf[0] = 0x40;
	buf[1] = 0x00;
	if( write( i2c_fd, buf, 2 ) != 2 )
	{
		fprintf(stderr, "write to i2c failed: err %i %s\n", errno, strerror(errno) );
	}


	int bytesRead =read( i2c_fd, buf, 6 ); 
	printf( "requested %i, got %i, contents:\n", 6, bytesRead );
	for ( int i=0; i<6; i++ )
	{
		printf("  %02x", buf[i] );
	}
	printf("\n");


	// open shared memory with address '/ipc_test', readwrite, with a+rw permissions
	mm_fd = shm_open( SHM_NAME, O_RDWR|O_CREAT, S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH );
	if ( mm_fd < 0 )
	{
		fprintf(stderr,"shm_open failed: err %i %s\n", errno, strerror(errno));
		cleanup();
		return 1;
	}
	
	// resize the fd to make space for the data
	int result = ftruncate( mm_fd, sizeof(IPCTestStruct) );
	if ( result < 0 )
	{
		fprintf(stderr,"ftruncate failed: err %i %s\n", errno, strerror(errno));
		cleanup();
		return 1;
	}

	// attempt to memory map mm_fd 
	sharedData = (IPCTestStruct*)mmap(NULL, sizeof(IPCTestStruct), PROT_READ|PROT_WRITE, MAP_SHARED, mm_fd, 0);
	if ( sharedData == NULL )
	{
		fprintf(stderr,"mmap failed: err %i %s\n", errno, strerror(errno));
		cleanup();
		return 1;
	}

	
	int readCount = 0;
	
	// main loop
	while ( !shouldStop )
	{
		if ( sharedData->bShouldRead )
		{
			readCount = 1;
			sharedData->bShouldRead = 0;
		}
		if ( readCount > 0 )
		{
			buf[0] = 0;
			write( i2c_fd, buf, 1 );
			int bytesRead =read( i2c_fd, buf, 6 ); 
			for ( int i=0; i<4; i++ )
				sharedData->inputs[i] = buf[i];
			sharedData->readSequenceId++;
			readCount--;
		}

		usleep( 300);
	}
	
	cleanup();
}

