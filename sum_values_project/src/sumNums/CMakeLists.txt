set(sources sumNums.cpp)
set(headers ../../include/JC/util.h ../../include/JC/openCLUtil.hpp)
set(resources ../all_kernels.ocl)

set(my_include_dirs ${CMAKE_CURRENT_SOURCE_DIR}/../../include)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release")

add_executable(sumNums ${sources} ${headers} ${resources})


# this does not work
set(RESOURCE_FILES
  simpleKernels.ocl
)

target_include_directories(sumNums PRIVATE 
    ${OpenCL_INCLUDE_DIRS}
    ${my_include_dirs})

target_link_libraries(sumNums ${OpenCL_LIBRARIES})
			
add_custom_command(TARGET sumNums
	COMMAND POST_BUILD COMMAND  ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../all_kernels.ocl ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/all_kernels.ocl
	COMMENT "Copying OpenCL files to Debug directory")

add_custom_command(TARGET sumNums
	COMMAND POST_BUILD COMMAND  ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../all_kernels.ocl ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/all_kernels.ocl
	COMMENT "Copying OpenCL files to Release directory")

add_custom_command(TARGET sumNums
	COMMAND POST_BUILD COMMAND  ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../all_kernels.ocl ${CMAKE_CURRENT_BINARY_DIR}
	COMMENT "Copying OpenCL files to build directory")

add_custom_command(TARGET sumNums
	COMMAND POST_BUILD COMMAND  ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../all_kernels.ocl ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/all_kernels.ocl
	COMMENT "Copying OpenCL files to bin directory")
