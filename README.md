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

### The Scaling

We performed a number of tests with each implementation and how they perform
against one another. These are the results

***Put a table or something explaining the results of our timing and performance experiments***

## The Conclusion

We can see that given a large, 2+ dimensional data set that GPUs perform really well.
We can also see that there is a huge performance gain between a serial implementation
and a parallel implementation.
