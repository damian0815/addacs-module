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

static t_class *addacs_in_class;

typedef struct _addacs_in {
	t_object x_obj;
	int mm_fd;
	t_int channel;
	IPCTestStruct* sharedData;

} t_addacs_in;


void addacs_in_openSharedData( t_addacs_in* x )
{
	// open shared memory with address '/ipc_test', readwrite, with a+rw permissions
	x->mm_fd = shm_open( SHM_NAME, O_RDWR, 0 );
	if ( x->mm_fd < 0 )
	{
		fprintf(stderr, "shm_open failed: err %i %s\n", errno, strerror(errno));
	}
	else
	{
		// attempt to memory map mm_fd 
		x->sharedData = (IPCTestStruct*)mmap(NULL, sizeof(IPCTestStruct), PROT_READ|PROT_WRITE, MAP_SHARED, x->mm_fd, 0);
		if ( x->sharedData == NULL )
		{
			fprintf(stderr, "mmap failed: err %i %s\n",  errno, strerror(errno));
			close( x->mm_fd );
			x->mm_fd = -1;
		}
	}

}

void addacs_in_bang(t_addacs_in* x)
{
	if ( x->mm_fd < 0 ) 
		addacs_in_openSharedData( x );
	else
	{
		x->sharedData->bShouldRead = 1;
		outlet_float(x->x_obj.ob_outlet, (float)x->sharedData->inputs[x->channel] );
	}
}


void *addacs_in_new(t_floatarg channel )
{
	t_addacs_in* x = (t_addacs_in*)pd_new( addacs_in_class );

	x->mm_fd = -1;
	x->sharedData = NULL;
	if ( channel > 4 || channel < 0 )
	{
		error( "[addacs_in] invalid channel %i: should be [0-4], defaulting to 0", (int)channel );
		x->channel = 0;
	}
	else
	{
		post( "[addacs_in] reading from channel %i", (int)channel );
		x->channel = channel;
	}

	outlet_new(&x->x_obj, &s_float);

	return (void*)x;
}

void addacs_in_delete( t_addacs_in* x )
{
	if ( x->sharedData )
		munmap( x->sharedData, sizeof(IPCTestStruct ));
	x->sharedData = NULL;
	if ( x->mm_fd > 0 )
		close(x->mm_fd);
	x->mm_fd = -1;


}


void addacs_in_setup(void)
{
	addacs_in_class = class_new( gensym("addacs_in"), 
			(t_newmethod)addacs_in_new,
			(t_method)addacs_in_delete,
			sizeof( t_addacs_in ),
			CLASS_DEFAULT, 
			A_DEFFLOAT,
			0 );

	class_addbang( addacs_in_class, addacs_in_bang );

}


