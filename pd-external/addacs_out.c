#include "m_pd.h"
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
#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))

static t_class *addacs_out_class;

typedef struct _addacs_out {
	t_object x_obj;
	int mm_fd;
	t_int channel;
	IPCTestStruct* sharedData;

} t_addacs_out;


void addacs_out_openSharedData( t_addacs_out* x )
{
	// open shared memory with address '/ipc_test', readwrite, with a+rw permissions
	x->mm_fd = shm_open( SHM_NAME, O_RDWR, 0 );
	if ( x->mm_fd < 0 )
	{
		error( "[addacs_out] shm_open failed: err %i %s\n", errno, strerror(errno));
	}
	else
	{
		// attempt to memory map mm_fd 
		x->sharedData = (IPCTestStruct*)mmap(NULL, sizeof(IPCTestStruct), PROT_READ|PROT_WRITE, MAP_SHARED, x->mm_fd, 0);
		if ( x->sharedData == NULL )
		{
			error( "[addacs_out] mmap failed: err %i %s\n",  errno, strerror(errno));
			close( x->mm_fd );
			x->mm_fd = -1;
		}
	}

}

// called on float arg
void addacs_out_float(t_addacs_out* x, t_floatarg f )
{
	if ( x->mm_fd < 0 )
		addacs_out_openSharedData( x );

	if ( x->mm_fd >= 0 )
	{
		x->sharedData->bShouldWrite = 1;
		// clamp f to 0..1
		f = MIN(1,MAX(f,0));
		// for now we map 0..1 to  INT_MIN..INT_MAX
		x->sharedData->outputs[x->channel] = (uint16_t)(f*65536);
	}

}

void *addacs_out_new(t_floatarg channel )
{
	t_addacs_out* x = (t_addacs_out*)pd_new( addacs_out_class );

	x->mm_fd = -1;
	x->sharedData = NULL;
	if ( channel > 4 || channel < 0 )
	{
		error( "[addacs_out] invalid channel %i: should be [0-4], defaulting to 0", (int)channel );
		x->channel = 0;
	}
	else
	{
		post( "[addacs_out] reading from channel %i", (int)channel );
		x->channel = channel;
	}
	addacs_out_openSharedData( x );


	return (void*)x;
}

void addacs_out_delete( t_addacs_out* x )
{
	if ( x->sharedData )
		munmap( x->sharedData, sizeof(IPCTestStruct ));
	x->sharedData = NULL;
	if ( x->mm_fd > 0 )
		close(x->mm_fd);
	x->mm_fd = -1;


}


void addacs_out_setup(void)
{
	addacs_out_class = class_new( gensym("addacs_out"), 
			(t_newmethod)addacs_out_new,
			(t_method)addacs_out_delete,
			sizeof( t_addacs_out ),
			CLASS_DEFAULT, 
			A_DEFFLOAT,
			0 );

	class_addfloat( addacs_out_class, addacs_out_float );

}


