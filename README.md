# CS5030-Final-AwannaCU

This is the final project for Brandon Garrett, Spencer Hall, and Tyler Conley
for the CS5030/6030 class, High Performance Computing.

Our project is our implementation of the AwannaCU option for the final project,
using SRTM data to see which areas of a map can see more area than others. It
utilizes Bresenham's algorithm to identify which pixels are in a given line of
sight from one pixel to another. 

## The Data

The input data for our algorithms is a 6000 x 6000 pixel raw file consisting of
signed short values. The file that we were given has the following SRTM data
(white is higher):

![Here is our input data](images/input.png "Input Data")

The output data is a file of unsigned 32-bit integers for how many visible pixels
are within a defined radius. The project description asked for a 100 pixel radius,
but the computation time for that took far too long. Our programs use a 10 pixel radius,
giving the following data output:

![Here is our output data](images/output.png "Output Data")


## The Implementations

We developed five distinct implementations to calculate the output of this data.
They are:

1. [Serial](Serial.cpp)
2. [Parallel CPU](MultiCPU.cpp)
3. [Parallel GPU](MultiGPU.cu)
4. [Distributed CPU](DistCPU.c)
5. [Distributed GPU](DistGPU.c)

### Descriptions

#### Serial

For the serial approach we used a double for loop that goes from pixel coordinate (0, 0) to (5999, 5999).
For each pixel in this range, it uses a 10 pixel radius to check the slope of each pixel in the current line of sight.
If the slope of the current pixel is greater than the slope of the previous pixel, it can be seen from the observation pixel and the total number of visible pixels is incremented.
Once all pixels within the 10 pixel radius have been checked, the total number of visible pixels is immediately written to the output file.

#### Parallel CPU

For the parallel CPU approach we used a double for loop that goes from pixel coordinate (0, 0) to (5999, 5999).
The outer for loop is run using OpenMP to evenly distribute rows of data across all the available threads.
For each pixel in this range, it uses a 10 pixel radius to check the slope of each pixel in the current line of sight.
If the slope of the current pixel is greater than the slope of the previous pixel, it can be seen from the observation pixel and the total number of visible pixels is incremented.
Once all pixels within the 10 pixel radius have been checked, the total number of visible pixels is saved to an output array.
Once all threads have finished their work and the double for loop is done, the output array is written to the output file.

#### Parallel GPU

For the parallel GPU approach we used a 2D grid size of 375 by 375 blocks and a 2D block size of 16 by 16 threads.
This gave a total of 36,000,000 threads for a 1 to 1 ratio of threads to pixels.
Each thread is assigned a pixel and uses a 10 pixel radius to check the slope of each pixel in the current line of sight.
If the slope of the current pixel is greater than the slope of the previous pixel, it can be seen from the observation pixel and the total number of visible pixels is incremented.
Once all pixels within the 10 pixel radius have been checked, the total number of visible pixels is saved to an output array on the GPU.
Once all threads have finished their work, the output array is transfered to the CPU and written to the output file.

#### Distributed CPU

For the distributed CPU approach we used a double for loop that goes from pixel coordinate (0, 0) to (5999, 5999).
Process 0 is in charge of distributing subarrays of size 21 by 21 pixels from the input data to the other processes.
It checks the slope of each pixel in the subarray and the current line of sight.
If the slope of the current pixel is greater than the slope of the previous pixel, it can be seen from the observation pixel and the total number of visible pixels is incremented.
Once all pixels within the 10 pixel radius have been checked, the total number of visible pixels is saved to an output array.
Once the double for loop is done, process 0 is in charge of writting the output array to the output file.

#### Distributed GPU

For the distributed GPU approach we used a double for loop that goes from pixel coordinate (0, 0) to (5999, 5999).
Process 0 is in charge of distributing subarrays of size 21 by 21 pixels from the input data to the other processes.
Each process then uses a 2D grid size of 100 by 100 blocks and a 2D block size of 10 by 10 threads.
This gives each process a 1,000,000 pixel section of the input data to work with.
For each pixel in this range, it uses a 10 pixel radius to check the slope of each pixel in the current line of sight.
If the slope of the current pixel is greater than the slope of the previous pixel, it can be seen from the observation pixel and the total number of visible pixels is incremented.
Once all pixels within the 10 pixel radius have been checked, the total number of visible pixels is saved to an output array.
Once the double for loop is done, process 0 is in charge of writting the output array to the output file.

### Individual Execution

	g++ Serial.cpp -o serial
	./serial srtm_14_04_6000x6000_short16.raw

	g++ -fopenmp MultiCPU.cpp -o multicpu
	./multicpu srtm_14_04_6000x6000_short16.raw <Number of threads>

	nvcc -c MultiGPU.cu -o multigpu.o
	nvcc multigpu.o -o multigpu -lcudart
	srun ./multigpu

	mpicc DistCPU.c -o distcpu
	mpiexec -n <Number of processes> ./distcpu

	nvcc -c MultiGPU.cu -o multigpu.o
	mpicc DistGPU.c -o distgpu.o
	mpicc distgpu.o multigpu.o -o distgpu -lcudart
	mpiexec -n <Number of processes> ./distgpu

### Scaling Performance

We timed each implementation and here is how they perform
against one another:

|| Serial | Parallel CPU | Parallel GPU |
|:-| :----: | :----------: | :----------: |
| Compute Time (s)| 641 | 391 | 2.247 |
| Bandwidth Performance (KB/s) | 329 | 539 | 93867 |

We were unable to test the scaling of the distributed implementations due to the CHPC being offline on Tuesday Decemebr 6th.

### Verification

Running the following command will build, execute, and validate the results of each implementation.  The runtime of the program should be around 30 minutes to complete.

	./CompileRunValidate.sh

To validate the results of a single execution use the following command.

	diff <file1> <file2>

Running the example command below will compare the serial and parallel GPU implementations.  No output means the files are identical.  The command below will only state if the files differ, not how they may differ.

	diff srtm_14_04_6000x6000_int32_serial_10.raw srtm_14_04_6000x6000_int32_gpu_10.raw

## The Conclusion

We can see that given a large, 2+ dimensional data set that GPUs perform really well.
We can also see that there is a huge performance gain between a serial implementation
and a parallel implementation.

## References

All pseudocode for the Bresenham's line algorithm used in this project was taken and modified from: https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
