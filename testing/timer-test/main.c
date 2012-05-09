#include <time.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <sys/time.h>

int alarmCount = 0;

void alarm( int signum )
{
	alarmCount++;
}

int main()
{
	// register signal handler
	struct sigaction act;
	act.sa_handler = alarm;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction( SIGRTMIN, &act, NULL );
	

	// create the timer
	timer_t timer;
	struct sigevent event;
	event.sigev_notify = SIGEV_SIGNAL;
	event.sigev_signo = SIGRTMIN;
	if( 0 != timer_create( CLOCK_REALTIME, &event, &timer ) )
	{
		fprintf(stderr,"timer_create failed: %i %s\n", errno, strerror(errno) );
		exit(0);
	}

	struct itimerspec timerspec;
	// 1ms intervals
	timerspec.it_interval.tv_sec = 0;
	timerspec.it_interval.tv_nsec = 1*1000;
	// 1ms from now
	timerspec.it_value.tv_sec = 0;
	timerspec.it_value.tv_nsec = 1000*1000;

	// record start time
	struct timeval start;
	gettimeofday( &start, NULL );

	// arm the timer
	timer_settime( timer, NULL, &timerspec, NULL );
			
	struct timeval t;
	gettimeofday( &t, NULL );
	int seconds = 0;
	while( seconds < 30 )
	{
		struct timeval now;
		gettimeofday( &now, NULL );
		
		struct timeval delta;
		timersub( &now, &t, &delta );
		if ( delta.tv_sec > 0 )
		{

			// calculate time since start in millis
			timersub( &now, &start, &delta );
			unsigned long millisSinceStart = delta.tv_sec*1000 + delta.tv_usec/(1000);

			printf("alarms: %8i interval: %4fms\n", alarmCount, (float)millisSinceStart/alarmCount );
			t = now;
			seconds++;
		}
		usleep(100*1000);
	}


	return 0;
}



