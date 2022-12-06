#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* size of computed section per process */
#define SLICE 1000


int main(int argc, char *argv[])
{
  /* parameters for calculation */
  int radius = 100; /* max distance of point for visibility */
  int width = 6000; /* width of file */
  int height = 6000; /* height of file */

  /* memory allocation for calculation */
  short *data; /* input data collection */
  uint32_t *output; /* output memory for data */

  data = (short *)malloc(width * height * sizeof(short));
  output = (uint32_t *)malloc(width * height * sizeof(uint32_t));

  /* read in input file */
  FILE *f = fopen("srtm_14_04_6000x6000_short16.raw", "rb");
  fread(data, sizeof(short), width * height, f);
  fclose(f);

  /* MPI parameters */
  int num_proc; /* number of processes */
  int proc_rank; /* process rank */

  MPI_Init(NULL, NULL);

  MPI_Comm_size(MPI_COMM_WORLD,
                &num_proc);

  MPI_Comm_rank(MPI_COMM_WORLD,
                &proc_rank);

  // set up a new type to concatenate results to each other

  MPI_Datatype slice;
  MPI_Type_contiguous( SLICE * SLICE, MPI_UNSIGNED, &slice);
  MPI_TYPE_commit(&slice);

  // instead of scattering, just give each process a 1000 x 1000 chunk (SLICE x SLICE) of the output array 
  short *localData;
  uint32_t localViewshed[SLICE * SLICE];

  for (int y=0; y<height/SLICE; y++)
    {
      for (int x=0; x<width/SLICE; x++)
        {
          int rank = (y * (height/SLICE) + x) % num_proc;


          int x0 = x == 0 ? 0 : (x*SLICE)-radius;
          int x1 = x == (width/SLICE) - 1 ? width : (x*SLICE)+radius;
          int y0 = y == 0 ? 0 : (y*SLICE)-radius;
          int y1 = y == (height/SLICE) - 1 ? height : (y*SLICE)+radius;


          if (proc_rank == 0)
          {

            short buffer[(x1-x0) * (y1-y0)];

            int index = 0;
            for (int i=y0; i<y1; i++)
              {
                for (int j=x0; j<x1; j++)
                  {
                    buffer[index++] = data[i * height + j];
                  }
              }

            MPI_Isend(&buffer, (x1-x0) * (y1-y0), MPI_SHORT, rank, 0, MPI_COMM_WORLD);

          }


          if (proc_rank == rank)
          {
            MPI_Irecv(&localData, (x1-x0) * (y1-y0), MPI_SHORT, 0, 0, MPI_COMM_WORLD);
            
            startKernel(localData, localViewshed, radius, SLICE, SLICE);

            MPI_Send(&localViewshed, 1, slice, 0, 0, MPI_COMM_WORLD);
          }

          if (proc_rank == 0)
          {
            // add results of local viewshed to global viewshed
            MPI_recv(&output[(y*SLICE) * height + (x*SLICE)], 1, slice, rank, 0, MPI_COMM_WORLD);
          }
        }
    }


  
  if (proc_rank == 0)
  {
    f = fopen("out_distgpu.raw", "wb");
    fwrite(output, sizeof(uint32_t), width * height, f);
    fclose(f);
  }

  MPI_Finalize();

  return 0;
}
