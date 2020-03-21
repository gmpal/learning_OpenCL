/*
A microbenchmark is a program designed to measure one specific characteristic of the hardware.
If we want to measure GPU performance: 
	operations per second = instructions / runtime 
	
	
Example: 
	a = b * c (with a, b and c floating point variables) 

!!Pay attention to the following issues: 
		1. minimize overhead (eg memory transfer) so that execution time is really measured
		2. "Fool the compiler": make sure that EACH instruction is executed
*/

#define N 1000
__kernel void fmul (__global float* destination, float factor, int flag)
{
	float product = 1; 
	#pragma unroll N
	for (int i = 0; i < N; ++i)
		product = product * factor; 
	
	if (flag * product)
		destination[ get_global_id(0)] = product;
}

/*
How does this kernel solve the 2 problems above? 
	1. Overhead is minimized in many ways.
		a. data on which the instruction operates is passed as a simple argument (factor) 
		b. 1000 multiplications -> contribution of other instructions is negligible 
		c. no loop overhead (loop is unrolled) 
		d. Iterations of the loop are dependent (no Instruction Level Parallelism) 
	2. Writing the result to global memory -> depends on flag (passed as argument, will be 0) and on product that is not known. 
		The compiler cannot optimize it away.
		(Without this last "writing back" the compiler can see that product is not used anymore after the loop, making it useless. 
		At the same time we do not want the compiler to write it back for real, because it will cause overhead.

*/


/* What's wrong with this code? --> The loop is COMPLETELY UNROLLED. 
This can cause performance degradation because the size of the code becomes too big (especially new GPUs that have an instruction buffer between the *instructions cache* and the *schedulers*.  
Solution? --> PARTIALLY UNROLL THE LOOP 
*/ 

// SOLUTION ONE: pragma unroll X (#pragma unroll 64 --> make sure that 64 is a whole divisor of 1024)
#define N 1024 
__kernel void fmul (__global float* destination, float factor, int flag) 
{
	float product = 1; 
	#pragma unroll 64
	for (int i = 0; i < N; ++i)
		product = product * factor; 
	
	if (flag * product)
		destination[ get_global_id(0)] = product;
}


//or, identically: 
#define N 1024 
__kernel void fmul (__global float* destination, float factor, int flag) 
{
	float product = 1; 
	
	for (int i = 0; i < N/64; ++i) {
		#pragma unroll 64
		for (int i = 0; i < 64; ++i)
			product = product * factor;
	}
	
	if (flag * product)
		destination[ get_global_id(0)] = product;
}


