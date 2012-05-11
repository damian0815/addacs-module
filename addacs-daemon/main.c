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
#include <time.h>
#include <sys/time.h>
#include <linux/spi/spidev.h>
#include <linux/types.h>
#include <stdint.h>
#include <sys/ioctl.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))


//#define NUNCHUCK

int nunchuck_fd = -1;
int adc_fd = -1;
int mm_fd = -1;
int dac_fd = -1;
IPCTestStruct* sharedData = 0;
uint8_t shouldStop = 0;
uint8_t useTimers = 0;
timer_t timer;
// adc has slave address 0x34 = binary 0110100
#define ADC_ADDRESS 0x33


#define SPI_SPEED 100000
#define SPI_BITS_PER_WORD 8


void cleanup();

void interrupt()
{
	shouldStop = 1;
}


// adc
int adc_slaveAssign( )
{
	if ( ioctl(adc_fd, I2C_SLAVE, ADC_ADDRESS)<0 )
	{
		perror("addacs_daemon: couldn't assign adc address");
		return 1;
	}
	return 0;
}

int adc_setup()
{
	// open the i2c bus
	char * filename = "/dev/i2c-2";
	if ( (adc_fd=open(filename, O_RDWR))<0 )
	{	
		fprintf(stderr, "error opening i2c-2: %d %s\n", errno, strerror(errno) );
		return 3;
	}
	if ( 0 != adc_slaveAssign() )
		return 2;

	uint8_t buf[3];
	// setup byte, bipolar, not RST, external reference
	buf[0] = (1<<7)|(1<<2)|(1<<1)|(0x2<<4); 
	if( write( adc_fd, buf, 1 ) != 1 )
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
	// according to max11612-7 datasheet, 
	// cs[3-0] select the channel
	// we are always differentional, and
	// we always have first chan +, second - so cs0 = 0 
	// + we are diffirential so SGL = 0
	// now cs3, cs2, cs1 select the channel
	// everything else is 0
	uint8_t buf[2];
	buf[0] = 0; 
	buf[0] |= (channel << 1);
	buf[0] |= 1; // single-ended
	if( write( adc_fd, buf, 1 ) != 1 )
	{
		fprintf(stderr,"addacs-daemon: failed to write adc chan %i selection byte: err %i %s", channel, errno, strerror(errno) );
		return -1;
	}
	// now read
	if ( read( adc_fd, buf, 2 ) != 2 )
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

// adc 
// AD5664 on SPI

int dac_setup()
{

	dac_fd = open("/dev/spidev4.0", O_RDWR);
	if ( dac_fd < 0 )
	{
		fprintf(stderr, "couldn't open /dev/spidev4.0: %i %s\n", errno, strerror(errno) );
		return 1;

	}

	int bits = SPI_BITS_PER_WORD;
   int ret = ioctl( dac_fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
   if (ret == -1)
   {
	   fprintf(stderr, "couldn't set SPI bits per word to %i: %i %s\n", bits, errno, strerror(errno) );
	   return 2;
   }

   int speed = SPI_SPEED;
   ret = ioctl( dac_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed );
   if ( ret == -1 )
   {
	   fprintf(stderr, "couldn't set SPI max write speed to %i: %i %s\n", speed );
	   return 3;
   }

	return 0;
}

int dac_write( uint8_t channel, uint16_t value )
{
	char buf[3];
	buf[0] = 0;
	buf[0] |= 0x3<<2; // c2-c0 011 = update channel n
	buf[0] |= (channel&0x04); // a2-a0 000 = channel
	buf[1] = value >> 8; // MSB
	buf[2] = value & 0x00ff; // LSB

	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)buf,
		.rx_buf = 0,
		.len = ARRAY_SIZE(buf),
		.delay_usecs = 0,
		.speed_hz = SPI_SPEED,
		.bits_per_word = SPI_BITS_PER_WORD,
	};

	int ret = ioctl( dac_fd, SPI_IOC_MESSAGE(1), &tr );
	if ( ret <1 )
	{
		fprintf( stderr, "error writing to DAC: %i %s\n", errno, strerror(errno) );
		return 1;
	}
	
	return 0;

}




// nunchuck

int nunchuck_slaveAssign()
{
	// nunchuck address
	int addr=0x52; 
	if ( ioctl(nunchuck_fd, I2C_SLAVE, addr) < 0 )
	{
		fprintf(stderr, "nunchuck_slaveAssign: error assigning I2C slave address %x: %d %s", addr, errno, strerror(errno) );
		return 1;
	}
	return 0;
}



int nunchuck_setup()
{
	// open the i2c bus
	char * filename = "/dev/i2c-2";
	if ( (nunchuck_fd=open(filename, O_RDWR))<0 )
	{	
		fprintf(stderr, "error opening i2c-2: %d %s\n", errno, strerror(errno) );
		return 3;
	}
	if ( nunchuck_slaveAssign()!= 0 )
		return 1;
	
	// handshake with nunchuck
	uint8_t buf[2];
	buf[0] = 0x40;
	buf[1] = 0x00;
	if( write( nunchuck_fd, buf, 2 ) != 2 )
	{
		fprintf(stderr, "write to i2c failed: err %i %s\n", errno, strerror(errno) );
		return 2;
	}

	return 0;
}

int nunchuck_read( uint8_t *buf )
{
	buf[0] = 0;
	if ( 1 != write( nunchuck_fd, buf, 1 ) )
	{
		perror( "nunchuck_read: write failed" );
		return 2;
	}

	if( 6 != read( nunchuck_fd, buf, 6 ) )
	{
		perror( "nunchuck read: read failed" );
		return 3;	
	}
	return 0;
}	

// application
void cleanup()
{

	if ( useTimers )
		timer_delete( timer );

	// cleanup
	if ( nunchuck_fd != -1 )
		close( nunchuck_fd );
	if ( adc_fd != -1  )
		close( adc_fd );
	if ( dac_fd != -1 )
		close( dac_fd );
	if ( sharedData )
		munmap( sharedData, sizeof(IPCTestStruct ));
	if ( mm_fd )
	{
		close(mm_fd);
		shm_unlink( SHM_NAME );
	}
	nunchuck_fd = -1;
	adc_fd = -1;
	mm_fd = -1;
	sharedData = 0;

	printf("cleaned up\n");
}

unsigned long callCount = 0;
unsigned long overrunCount = 0;
void periodicFunction()
{

#ifdef NUNCHUCK
	uint8_t buf[128];
	nunchuck_read( buf );
	for ( int i=0; i<3; i++ )
		sharedData->inputs[i] = buf[i];
#endif
	
	for ( int i=0; i<2; i++ )
		sharedData->inputs[i] = adc_read(i);
	
	for ( int i=0; i<2; i++ )
		dac_write(i, sharedData->outputs[i] );

	callCount++;
	
	/*
	overrunCount += useTimers*timer_getoverrun(timer);
	*/
}

void printOverrunStats()
{
	printf("addacs-daemon: periodicFunction called %lu times with %lu overruns\n", callCount, overrunCount );
	printf("addacs-daemon: (%f overruns/call)\n",(float)overrunCount/callCount);
	printf("addacs-daemon: - now resetting counters\n");
	callCount = 0;
	overrunCount = 0;
}

void dump_clockres()
{

	clockid_t clockids[] = { CLOCK_REALTIME, 
		CLOCK_MONOTONIC, 
		CLOCK_MONOTONIC_RAW,
		CLOCK_PROCESS_CPUTIME_ID,
		CLOCK_THREAD_CPUTIME_ID };
	for ( int i=0; i<5 ;i++ )
	{
		struct timespec ts;
		int result = clock_getres( clockids[i], &ts );
		if ( result < 0 )
			fprintf(stderr, "clock_getres() failed with clock %i: err %i %s\n", clockids[i], errno, strerror(errno) );
		else
			printf("clock %i: res %lu:%lu (%f ms)\n", clockids[i], ts.tv_sec, ts.tv_nsec, (float)ts.tv_sec*1000+(float)ts.tv_nsec/(1000*1000) );
	}


}

int main( int argc, char**argv )
{
	uint8_t foreground = 0;
	for ( int i=1; i<argc; i++ )
	{
		if ( strcmp(argv[i],"-f")==0 )
			foreground = 1;
		else if ( strcmp(argv[i], "-r")==0 )
		{
			dump_clockres();
			exit(0);
		}
		else if ( strcmp(argv[i], "-t")==0 )
		{
			useTimers = 1;
		}
	}
		
	if ( !foreground )
	{
		if ( fork() != 0 ) 
			return 0;
	}

	atexit( cleanup );
 	signal(SIGINT, interrupt );
	signal(SIGTERM, interrupt );
	signal(SIGSEGV, interrupt );


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

#ifdef NUNCHUCK
	// setup the nunchuck
	if ( 0 != nunchuck_setup() )
	{
		return 3;
	}
#endif

	// setup the dac
	if ( 0 != dac_setup() )
	{
		return 2;
	}
		// setup the adc
	if ( 0 != adc_setup() )
	{
		return 1;
	}

	printf("deamon running, shared memory address is %s\n", SHM_NAME );

	if ( useTimers )
	{
		printf("using timers\n");
		signal(SIGUSR1, printOverrunStats );
	
		// register signal handler
		struct sigaction act;
		act.sa_handler = periodicFunction;
		sigemptyset( &act.sa_mask );
		act.sa_flags = 0;
		sigaction( SIGRTMIN, &act, NULL );

		// create the timer
		struct sigevent event;
		event.sigev_notify = SIGEV_SIGNAL;
		event.sigev_signo = SIGRTMIN;
		if ( 0 != timer_create( CLOCK_REALTIME, &event, &timer ) )
		{
			fprintf(stderr, "timer_create failed: %i %s\n", errno, strerror(errno) );
			return 2;
		}
		struct itimerspec timerspec;
		// 1ms intervals
		timerspec.it_interval.tv_sec = 0;
		timerspec.it_interval.tv_nsec = 10000*1000;
		// 1ms from now
		timerspec.it_value.tv_sec = 0;
		timerspec.it_value.tv_nsec = 1000*1000;
		// record start time
		struct timeval start;
		gettimeofday( &start, NULL );
		// arm the timer
		timer_settime( timer, NULL, &timerspec, NULL );

		// main loop
		while ( !shouldStop )
		{

			sleep( 1 );
		}
		printf("shouldStop != 0, exiting\n"); 
	
	}
	else // ! useTimers
	{
		printf("using clock_nanosleep()\n");
		// use clock_nanosleep()
		struct timespec sleepTime;
		struct timespec remain;
		// delay until 1ms from now
		clock_gettime( CLOCK_MONOTONIC, &sleepTime );	
		// add 1 ms
		int lastErr = 0;

		while( !shouldStop )
		{
			if ( lastErr == EINTR )
				printf("addacs-daemon interrupted\n");
			else if ( lastErr != 0 )
			{
				fprintf(stderr, "clock_nanosleep returned error %i %s\n", lastErr, strerror( lastErr ) );
			}
			else
				periodicFunction();
			
			uint64_t ns = 100*1000*1000;
#ifndef NSEC_PER_SEC
#define NSEC_PER_SEC    1000000000L
#endif
			lldiv_t div_result = lldiv( sleepTime.tv_nsec+ns, (uint64_t)NSEC_PER_SEC );
			sleepTime.tv_sec += div_result.quot;
			sleepTime.tv_nsec = div_result.rem;
			/*
			struct timespec now;
			clock_gettime( CLOCK_MONOTONIC, &now );

			printf("sleepTime is %lu:%lu, now is %lu:%lu\n", sleepTime.tv_sec, sleepTime.tv_nsec, now.tv_sec, now.tv_nsec );
			*/

			lastErr = clock_nanosleep( CLOCK_MONOTONIC,
				TIMER_ABSTIME, &sleepTime, &remain );	

		}

		printf("shouldStop != 0, exiting\n"); 
	}

	

}

