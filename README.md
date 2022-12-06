# CS5030-Final-AwannaCU

## To compile MPI-Cuda mixed code:

```sh
$ module load mpi cuda
$ mpicc -c main.c -o main.o
$ nvcc -c multiply.cu -o multiply.o
$ mpicc main.o multiply.o -lcudart
```
