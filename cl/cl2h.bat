cat cl_utils.cl cl_fft.cl > routines.cl
clcc --add_headers -o routines.o routines.cl
xxd -i routines.cl "../cl_routines.h"
