/*
OpenCL language: C and some extra keywords
Defines kernels and device functions. 
	Kernels can be launched from CPU. 
	Device Functions are launched from GPU from a kernel or another device function. 
	Differentiation: __kernel keyword
					void return type for kernels

Work items: laid out in a global index space (1D/2D/3D)
	Threads are numbered [0,...,size-1] in each dimension 
	get_global_id(int dimension): retrieves the index 
	get_global_size(int dimension): retrieves the size 	
*/



/////////////
///MAPPING///
/////////////

/*
Assigning different parts of the data to each thread: mapping work items to data items
NDRange describes the space of work-items 
*/

int W = 24; //Array size and number of work items 

/*Start 1-to-1 mapping  |0|1|2|3|4|5|6|7|8|...|*/
	/*start host code*/ 
unsigned int data_index[W]; //will contain the work item indexes 
cl::NDRange global(W);  //builds an ND-range descriptor with global_work_size = W 
	/*end host code*/

	//data index is passed to the kernel as arguemnt 

	/*start device code*/
unsigned int index = get_global_id(0) //each thread gets its own ID 
data_index[index] = index;
	/*end device code*/
/*End 1-to-1 mapping*/


/*Start 1-to-N mapping --> contiguous |0|0|1|1|2|2|3|3|4|4|5|5|...|*/
	/*start host code*/ 
unsigned int data_index[W]; //will contain the work item indexes 
cl::NDRange global(W/N);  //builds an ND-range descriptor with global_work_size = W/N 
	/*end host code*/

	//data index is passed to the kernel as argument 

	/*start device code*/
unsigned int index = get_global_id(0) //each thread gets its own ID 
//each thread will have a part of data_index of size N 
for (int i = 0; i < N; ++i)
	data_index[i] = N * index + i;
	//if this thread is index 3 and N = 8
	//data_index[0] = 8*3+0 = 24
	//data_index[1] = 8*3+1 = 25 [???]
	/*end device code*/
/*End 1-to-N mapping  --> contiguous*/

/*Start 1-to-N mapping --> not contigous |0|1|2|3|4|0|1|2|3|4|...|*/
	/*start host code*/ 
unsigned int data_index[W]; //will contain the work item indexes 
cl::NDRange global(W/N);  //builds an ND-range descriptor with global_work_size = W/N 
	/*end host code*/

	//data index is passed to the kernel as argument 

	/*start device code*/
unsigned int index = get_global_id(0): //each thread gets its own ID 
unsigned int size = get_global_size(0);  //each thread gets the work item size
//each thread will have a part of data_index of size N 
for (int i = 0; i < N; ++i)
	data_index[i] = index + i*size; 
	//data_index[0] = 3 + 0*8 = 3 
	//data_index[1] = 3 + 1*8 = 11 
	//data_index[2] = 3 + 2*8 = 19 [??] 


	
/*End 1-to-N mapping  -->  not contigous*/ 


