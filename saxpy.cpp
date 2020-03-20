/* The process to follow is:
1) Get the Platforms
2) Get the Devices
3) Create a Context 
4) Create a CommandQueue
5) Create Sources object and push_back the sources
6) Build a Program (connecting sources, context, devices) 
7) Create input / output buffers
8) Create a Kernel and set its arguments
9) Send input data
10) Create NDRanges
11) Execute Kernels 
12) Collect Results

*/


void saxpy(float* x, float* y, int n, float a)
{
	std::vector<cl::Platform> platforms; //declares a vector of platforms 
	
	cl::Platform::get(&platforms); //gets the platforms and writes them in the vector of platforms 	
	
	std::vector<cl::Device> devices; //declares a vector of devices (GPUs, but not only) 
	
	platforms[0].getDevices(CL_DEVICE_TYPE_GPU, &devices); //gets the devices for platform 0 
	/*
	CL_DEVICE_TYPE_GPU --> An OpenCL device that is a GPU. By this we mean that the device can also be used to accelerate a 3D API such as OpenGL or DirectX.
	*/

	cl::Context context(devices); //creates the context object for all the devices
	
	cl::CommandQueue commandQueue(context, devices[0]) //creates the command Queue object. It is associated with device number 0. 
	/*
	One of the many constructors for CommandQueue. Don't forget CommandQueue is an interface for the cl_command_queue. [Wrapper!] 
	CommandQueue (const Context &context, const Device &device, cl_command_queue_properties properties=0, cl_int *err=NULL)
		Constructs a CommandQueue for a passed device and context 
		Will return an CL_INVALID_QUEUE_PROPERTIES error if CL_QUEUE_ON_DEVICE is specified.
	*/
		
	std::string sourceText = fileToString("saxpy.ocl") //reads the OpenCL source code 
	
	std::pair<const char*, size_t> source(sourceText.c_str(), sourceText.size());  //creates a pair object, coupling the string (array of char) with its size 
	/*c_str() function
			Returns a pointer to an array that contains a null-terminated sequence of characters (i.e., a C-string) representing the current value of the string object. This array includes the same sequence of characters that make up the value of the string object plus an additional terminating null-character ('\0') at the end.
	*/
	
	cl::Program::Sources sources; //collects the OpenCL code-sources 
	
	sources.push_back(source); //adds the saxpy code source to the list of sources 
	
	cl::Program program = cl::Program(context, sources); //creates the OpenCL program related to a context and the sources 
	
	program.build(devices); //"builds" the program against the specified devices
	
	//##########################################################//
	
	cl::Buffer xBuffer(context, CL_MEM_READ_ONLY, sizeof(float)*n);
	cl::Buffer yBuffer(context, CL_MEM_READ_WRITE, sizeof(float)*n); 
	/*
	Class interface for Buffer Memory Objects 
	Buffer (const Context &context, cl_mem_flags flags, size_type size, void *host_ptr=NULL, cl_int *err=NULL)
	Constructs a Buffer in a specified context.
	*/
	
	cl::Kernel kernel(program, "saxpy"); 
	kernel.setArg<cl::Buffer>(0, xBuffer); 
	kernel.setArg<cl::Buffer>(1, yBuffer); 
	kernel.setArg<float>(2,a); 
	
	/*
	Kernel creation and arguments setting
	Kernel (const Program &program, const char *name, cl_int *err=NULL)
	*/
	
	commandQueue.enqueueWriteBuffer(xBuffer, CL_TRUE, 0, sizeof(float)*n, x); 
	commandQueue.enqueueWriteBuffer(yBuffer, CL_TRUE, 0, sizeof(float)*n, y); 
	/*
	Sending input data to the device
	cl_int clEnqueueWriteBuffer (	cl_command_queue command_queue,
									cl_mem buffer,
									cl_bool blocking_write,
									size_t offset,
									size_t cb,
									const void *ptr,
									cl_uint num_events_in_wait_list,
									const cl_event *event_wait_list,
									cl_event *event)

	*/
	
	cl::NDRange global(n);
	cl::NDRange local(128);
	/*
	Specifying NDRange values.
	*/
		
	commandQueue.enqueueNDRangekernel(kernel, cl::NullRange, global, local);  //Kernel execution!
	/*
	cl_int clEnqueueNDRangeKernel (	cl_command_queue command_queue,
									cl_kernel kernel,
									cl_uint work_dim,
									const size_t *global_work_offset,
									const size_t *global_work_size,
									const size_t *local_work_size,
									cl_uint num_events_in_wait_list,
									const cl_event *event_wait_list,
									cl_event *event)
	*/
	
	commandQueue.enqueueReadBuffer(yBuffer,CL_TRUE,0, sizeof(float)*n,y); //getting results
	/*
	
	cl_int clEnqueueReadBuffer (    cl_command_queue command_queue,
									cl_mem buffer,
									cl_bool blocking_read,
									size_t offset,
									size_t cb,
									void *ptr,
									cl_uint num_events_in_wait_list,
									const cl_event *event_wait_list,
									cl_event *event)
	*/
	
	
	