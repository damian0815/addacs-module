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
// adc has slave address 0x64 = 0b0110110
#define ADC_ADDRESS 0b0110100

void interrupt()
{
	shouldStop = 1;
}

// i2c line handling
int i2c_open()
{
	// open the i2c bus
	char * filename = "/dev/i2c-2";
	if ( (i2c_fd=open(filename, O_RDWR))<0 )
	{	
		fprintf(stderr, "error opening i2c-2: %d %s\n", errno, strerror(errno) );
		return 1;
	}
	else
		return 0;

}

void i2c_close()
{
	close( i2c_fd );
}


// adc
int adc_slaveAssign()
{
	if ( ioctl(i2c_fd, I2C_SLAVE, ADC_ADDRESS )<0 )
	{
		perror("addacs_daemon: couldn't assign adc address");
		return 1;
	}
	return 0;
}

int adc_setup()
{
	if ( 0 != adc_slaveAssign() )
		return 2;

	uint8_t buf[3];
	buf[0] = 0b10000000; // setup byte

	if( write( i2c_fd, buf, 1 ) != 1 )
	{
		perror("addacs-daemon: failed to write adc setup byte" );
		return 1;
	}
	return 0;
}


// read adc, channel 0-3
// returns -1 on channel select failure, 
// returns -2 on read failure
// returns -3 on failure to assign ADC as I2C slave
int32_t adc_read( uint8_t channel )
{
	if ( adc_slaveAssign() != 0 )
		return -3;
	// according to max11612-7 datasheet, 
	// cs[3-0] select the channel
	// we are always differentional, and
	// we always have first chan +, second - so cs0 = 0 
	// + we are diffirential so SGL = 0
	// now cs3, cs2, cs1 select the channel
	// everything else is 0
	uint8_t buf[2];
	buf[0] = 0b00000000; 
	buf[0] |= (channel << 2);
	if( write( i2c_fd, buf, 1 ) != 1 )
	{
		fprintf(stderr,"addacs-daemon: failed to write adc chan %i selection byte: err %i %s", channel, errno, strerror(errno) );
		return -1;
	}
	// now read
	if ( read( i2c_fd, buf, 2 ) != 2 )
	{
		fprintf(stderr,"addacs-daemon: failed to read 2 bytes from adc chan %i: err %i %s", channel, errno, strerror(errno) );
		return -2;
	}
	// clear first 4 bits, as they are sent high
	uint16_t result = (buf[0] & (0x0f));
	// shift 4 bits to MSB of result
	result <<= 8;
	// OR in the last 8 bits
	result |= buf[1];
	return result;

}

// nunchuck

int nunchuck_slaveAssign()
{
	// nunchuck address
	int addr=0x52; 
	if ( ioctl(i2c_fd, I2C_SLAVE, addr) < 0 )
	{
		fprintf(stderr, "error assigning I2C slave address %x: %d %s", addr, errno, strerror(errno) );
		return 1;
	}
	return 0;
}



int nunchuck_setup()
{
	if ( nunchuck_slaveAssign()!= 0 )
		return 1;
	
	// handshake with nunchuck
	uint8_t buf[2];
	buf[0] = 0x40;
	buf[1] = 0x00;
	if( write( i2c_fd, buf, 2 ) != 2 )
	{
		fprintf(stderr, "write to i2c failed: err %i %s\n", errno, strerror(errno) );
		return 2;
	}

	return 0;
}

int nunchuck_read( uint8_t *buf )
{
	if ( 0 != nunchuck_slaveAssign() )
		return 1;

	buf[0] = 0;
	if ( 1 != write( i2c_fd, buf, 1 ) )
	{
		perror( "nunchuck_read: write failed" );
		return 2;
	}

	if( 6 != read( i2c_fd, buf, 6 ) )
	{
		perror( "nunchuck read: read failed" );
		return 3;	
	}
	return 0;
}	

// application
void cleanup()
{

	// cleanup
	if ( i2c_fd )
		i2c_close();
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

	
	if ( 0 != i2c_open() )
	{
		return 1;
	}


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
		return 1;
	}

	// attempt to memory map mm_fd 
	sharedData = (IPCTestStruct*)mmap(NULL, sizeof(IPCTestStruct), PROT_READ|PROT_WRITE, MAP_SHARED, mm_fd, 0);
	if ( sharedData == NULL )
	{
		fprintf(stderr,"mmap failed: err %i %s\n", errno, strerror(errno));
		return 1;
	}

	// setup the nunchuck
	if ( 0 != nunchuck_setup() )
	{
		return 1;
	}

	// setup the adc
	if ( 0 != adc_setup() )
	{
		return 1;
	}
	

	
	int readCount = 0;
	uint8_t buf[128];
	
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

			nunchuck_read( buf );
			for ( int i=0; i<3; i++ )
				sharedData->inputs[i] = buf[i];

//			uint32_t value = adc_read(0);
//			if ( value >= 0 )
//				sharedData->inputs[3] = value;

			sharedData->readSequenceId++;
			readCount--;
		}

		usleep( 300);
	}
	
}

