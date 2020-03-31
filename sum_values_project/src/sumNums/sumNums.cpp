#include <exception>
#include <iostream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <vector>
#include <chrono> // high-precision timing

#define __CL_ENABLE_EXCEPTIONS
#include <CL/cl.hpp> // CL namespace
#include <JC/util.h>
#include <JC/openCLUtil.hpp>  // JC namespace

using namespace std;
bool PRESS_KEY_TO_CLOSE_WINDOW = true; // when running from within visual studio
#define NBR_ALGORITHM_VERSIONS 100
#define NBR_EXPERIMENTS 5
#define N 1024 * 16
#define NUM_INT 15
#define NUM_FLOAT 15.5
#define ITERATIONS_TIMING 100
#define ITERATIONS_KERNELS 1000 //make sure its the same of Kernels! 

//function to do the sum on CPU

cl::Program buildProgram(
	const std::string& file_name,
	const cl::Context& context,
	const cl::Device& device,
	int amount)
{
	vector<cl::Device> devices;
	devices.push_back(device);
	std::string source_code = jc::fileToString(file_name);
	std::pair<const char*, size_t> source(source_code.c_str(), source_code.size());
	cl::Program::Sources sources;
	sources.push_back(source);
	cl::Program program(context, sources);
	std::string options = "-D N=" + std::to_string(amount);
	try {
		program.build(devices, options.c_str());
	}
	catch (cl::Error & e) {
		std::string msg;
		program.getBuildInfo<std::string>(devices[0], CL_PROGRAM_BUILD_LOG, &msg);
		std::cerr << "Your kernel failed to compile" << std::endl;
		std::cerr << "-----------------------------" << std::endl;
		std::cerr << msg;
		throw(e);
	}

	return program;
}



template <class T1, class T2>
int64_t sumOfNums(T1 *dest, T2 num, int flag)
{
	chrono::time_point<chrono::system_clock> start = chrono::system_clock::now();

	T1 result = 0; 

	#pragma unroll N
	for (int i = 0; i < N; ++i)
		result = result + num; 
   

	//std::cout << "Result from CPU: " << result << "." << std::endl;
	
	//fooling the compiler
    if (flag * result)
        dest[0] = result; //won't be executed

	return chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now() - start).count();
}

void print_table_title() {
	cout << "    ***** Version Name *****     | min(us)   | mean(us)  | stddev    | GOp/s     |  GB/s     |   CPI     | speedup   |";
	cout << endl;
}

void print_row(string name, long long time, long long time_mean, long long time_stddev, long long nbrOperations, long long nbrBytes, long long ticksPerMilliSecond, long long referenceTime) {
	const char separator = ' ';
	const int nameWidth = 32;
	const int numWidth = 9;

	if (name.length() > nameWidth)
		name = name.substr(0, nameWidth);
	cout << left << setw(nameWidth) << setfill(separator) << name << " | ";
	cout << right << setw(numWidth) << setfill(separator) << time << " | ";
	cout << right << setw(numWidth) << setfill(separator) << time_mean << " | ";
	cout << right << setw(numWidth - 4) << setfill(separator) << (time_stddev * 100.0 / time_mean) << "% | ";
	cout << right << setw(numWidth) << setfill(separator) << (float)nbrOperations / time / 1000 << " | ";
	cout << right << setw(numWidth) << setfill(separator) << (float)nbrBytes / time / 1000 << " | ";
	float total_nbr_cycles = (float)(time * (ticksPerMilliSecond / 1000));
	cout << right << setw(numWidth) << setfill(separator) << ((float)total_nbr_cycles / nbrOperations) << " | ";
	cout << right << setw(numWidth) << setfill(separator) << (float)referenceTime / time << " | ";
	cout << endl;
}

void analyzePerformance(vector<string> names, long long runtimes[NBR_ALGORITHM_VERSIONS][NBR_EXPERIMENTS], long long totalNbrOfOperations, long long totalNbrOfBytes, long long ticksPerMilliSecond, long long referenceTime) {
	print_table_title();
	for (int v = 0; v < names.size(); v++) {
		long long time = minimalValue(runtimes[v], NBR_EXPERIMENTS);
		long long time_mean = meanValue(runtimes[v], NBR_EXPERIMENTS);
		long long time_stddev = stddev(runtimes[v], NBR_EXPERIMENTS);
		print_row(names[v], time, time_mean, time_stddev, totalNbrOfOperations, totalNbrOfBytes, ticksPerMilliSecond, referenceTime);
	}
}

long long minimalValue(long long* values, int NBR) {
	long long min = values[0];
	for (int i = 1; i < NBR; i++) {
		if (values[i] < min)
			min = values[i];
	}
	return min;
}
long long meanValue(long long* values, int NBR) {
	long long sum = values[0];
	for (int i = 1; i < NBR; i++) {
		sum += values[i];
	}
	return sum / NBR;
}
long long stddev(long long* values, int NBR) {
	long long mean = meanValue(values, NBR);
	long long SSE = 0;
	for (int i = 0; i < NBR; i++) {
		SSE += (values[i] - mean) * (values[i] - mean);
	}
	return sqrt(SSE / NBR);
}


int main(int argc, char *argv[])
{

	if (argsContainsOption('h', argc, argv) || argsContainsUnknownOption("dhps", argc, argv)) {
		cout << "Usage: " << argv[0] << " -p <platform ID> -d <device ID> -s <array size>" << endl;
		return 0;
	}
	if (argc > 1)
		PRESS_KEY_TO_CLOSE_WINDOW = false; // running from terminal

	try {
		// *0* Configuration
		string kernel_file("all_kernels.ocl");

		int PLATFORM_ID = defaultOrViaArgs(1, 'p', argc, argv);
		int DEVICE_ID = defaultOrViaArgs(0, 'd', argc, argv);
		// TODO: array_size is not used deleting it is better?
		unsigned int array_size = defaultOrViaArgs(1000, 's', argc, argv); // DESTINATION array size
		int num_int = NUM_INT; 
		float num_float = NUM_FLOAT;
		int flag = 0;
   		
		// *1* OpenCL initialization
		cl::Device device = jc::getDevice(PLATFORM_ID, DEVICE_ID, PRESS_KEY_TO_CLOSE_WINDOW);

		cl::Context context(device);
		cl::CommandQueue queue(context, device, CL_QUEUE_PROFILING_ENABLE);
		

		// *2* Allocate memory on the host and populate source
        int *gpu_dst = new int[array_size], *cpu_dst = new int[array_size];
		float* gpu_dst_f = new float[array_size], * cpu_dst_f = new float[array_size];
       

		// *3* Allocate memory on the device
        cl::Buffer dest_buffer0(context, CL_MEM_WRITE_ONLY, array_size*sizeof(cl_int));
		cl::Buffer dest_buffer1(context, CL_MEM_WRITE_ONLY, array_size * sizeof(cl_float));
		cl::Buffer dest_buffer2(context, CL_MEM_WRITE_ONLY, array_size * sizeof(cl_float));

		
		float starting_float = 1.1f;
		
		int expected_sum_int = 0;
		float expected_sum_float = 0;
		float expected_sum_mix = starting_float;

		for (int i = 0; i < N; ++i) {
			expected_sum_int = expected_sum_int + NUM_INT;
			expected_sum_float = expected_sum_float + NUM_FLOAT;
			expected_sum_mix = expected_sum_mix + NUM_INT;
		}

        
		

		// *5* execute the code on the device
		cout << "Executing sumNumbers on device '"<< jc::deviceName(device) << endl;
		// local is left to be chosen by OpenCL
		// cl::NDRange local(wg_size);


		long long runtimes[NBR_ALGORITHM_VERSIONS][NBR_EXPERIMENTS];
		vector<string> names;

		int work_group_size = 256;
		int work_items = N / 256;
		for (int i = 0; i < 9; ++i) {
			
			// compile time with N macro equals to work_group_size
			cl::Program program = buildProgram(kernel_file, context, device, work_group_size);

			// Prepare the kernel parameters
			cl::Kernel kernel01(program, "intSum");
			// set the kernel arguments
			kernel01.setArg<cl::Buffer>(0, dest_buffer0);
			kernel01.setArg<cl_uint>(1, num_int);
			kernel01.setArg<cl_uint>(2, flag);
			kernel01.setArg<cl_uint>(3, expected_sum_int);


			cl::Kernel kernel02(program, "floatSum");
			// set the kernel arguments
			kernel02.setArg<cl::Buffer>(0, dest_buffer1);
			kernel02.setArg<cl_float>(1, num_float);
			kernel02.setArg<cl_uint>(2, flag);
			kernel02.setArg<cl_float>(3, expected_sum_float);


			cl::Kernel kernel03(program, "mixSum");
			// set the kernel arguments
			kernel03.setArg<cl::Buffer>(0, dest_buffer2);
			kernel03.setArg<cl_uint>(1, num_int);
			kernel03.setArg<cl_uint>(2, flag);
			kernel03.setArg<cl_float>(3, starting_float);
			kernel03.setArg<cl_float>(4, expected_sum_mix);
			
			std::cout << "WS" << work_group_size << std::endl;
			cl::NDRange global(N);
			cl::NDRange local(work_group_size);

			for (int t = 0; t < NBR_EXPERIMENTS; ++t) {
				int version = 3 * i;
				// transfer source data from the host to the device

				runtimes[version++][t] = jc::runAndTimeKernel(kernel01, queue, global, local);
				if (t == 0) {
						names.push_back("GPU INT+INT ");
						names.back() += "g=";
						names.back() += to_string(work_group_size);
				}
				
				runtimes[version++][t] = jc::runAndTimeKernel(kernel02, queue, global, local); // , local
				if (t == 0) {
					names.push_back("GPU FLOAT+FLOAT");
					names.back() += "g=";
					names.back() += to_string(work_group_size);
				}

				runtimes[version++][t] = jc::runAndTimeKernel(kernel03, queue, global, local); // , local	
				if (t == 0) {
					names.push_back("GPU FLOAT+INT");
					names.back() += "g=";
					names.back() += to_string(work_group_size);
				}
			}
			work_items *= 2;
			work_group_size /= 2;
		}
		long long referenceTime = minimalValue(runtimes[0], NBR_EXPERIMENTS);
		analyzePerformance(names, runtimes, array_size, array_size * sizeof(float), 0, referenceTime);


        // *9* Deallocate memory
		delete[] cpu_dst, gpu_dst, cpu_dst_f, gpu_dst_f;

		if (PRESS_KEY_TO_CLOSE_WINDOW) { cout << endl << "Press ENTER to close window..."; char c = cin.get(); }

        return 0;
    }
    catch (cl::Error& e) {
        cerr << e.what() << ": " << jc::readableStatus(e.err());
		if (PRESS_KEY_TO_CLOSE_WINDOW) { cout << endl << "Press ENTER to close window..."; char c = cin.get(); }
        return 3;
    }
    catch (exception& e) {
        cerr << e.what() << endl;
		if (PRESS_KEY_TO_CLOSE_WINDOW) { cout << endl << "Press ENTER to close window..."; char c = cin.get(); }
        return 2;
    }
    catch (...) {
        cerr << "Unexpected error. Aborting!\n" << endl;
		if (PRESS_KEY_TO_CLOSE_WINDOW) { cout << endl << "Press ENTER to close window..."; char c = cin.get(); }
        return 1;
    }
}
