#include "m_pd.h"

static t_class *addacs_in_class;

typedef struct _addacs_in {
	t_object x_obj;
} t_addacs_in;


void addacs_in_bang(t_addacs_in* x)
{
	post( "would read i2c, instead outputting 5" );
	outlet_float(x->x_obj.ob_outlet, 5.0f);
}

void *addacs_in_new(void)
{
	t_addacs_in* x = (t_addacs_in*)pd_new( addacs_in_class );

	outlet_new(&x->x_obj, &s_float);

	return (void*)x;
}


void addacs_in_setup(void)
{
	addacs_in_class = class_new( gensym("addacs_in"), 
			(t_newmethod)addacs_in_new,
			0,
			sizeof( t_addacs_in ),
			CLASS_DEFAULT, 
			A_DEFFLOAT,
			0 );

	class_addbang( addacs_in_class, addacs_in_bang );

}


