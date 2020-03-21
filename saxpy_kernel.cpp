/*
SAXPY -> Y = alpha*X + Y 
This is an example of a one-on-one mapping: each thread will work on ONE SINGLE data item,
corresponding to its own position index in the global index space (aka global_id)
*/

__kernel void saxpy(__global float *X, __global float *Y, float alpha, unsigned int N)
{
	int index = get_global_id(0);
	if (index < N)  //not necessary if #work_items == array_sizes.
		Y[index] = alpha*X[index] + Y[index]; 
}

