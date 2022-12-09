#include <mpi.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
  /* parameters for calculation */
  int radius = 10; /* max distance of point for visibility */
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

  for (int y=0; y<height; y++)
    {
      for (int x=0; x<width; x++)
        {
          int index = y * height + x;
          int rank = index % num_proc;

          /* root process generates buffer to send to computing process */
          if (proc_rank == 0)
          {
            short buffer[(radius*2 + 1)*(radius*2 + 1)];
            int buffer_index = 0;
            
            uint16_t leftX   = (x - radius) < 0 ? 0 : (x0 - radius) ;
            uint16_t topY    = (y - radius) < 0 ? 0 : (y0 - radius) ;
            uint16_t rightX  = (x + radius) >= width  ? width - 1  : (x0 + radius) ;
            uint16_t bottomY = (y + radius) >= height ? height - 1 : (y0 + radius) ;

            for (int y0=topY; y0<bottomY; y0++)
              {
                for (int x0=leftX; x0<rightX; x0++)
                  {
                    buffer[buffer_index++] = data[y0 * height + x0];
                  }
              }

            MPI_Isend(&buffer, (radius*2 + 1)*(radius*2 + 1), MPI_SHORT, rank, 0, MPI_COMM_WORLD);
          }

          short shared_buffer[(radius*2 + 1)*(radius*2 + 1)];

          MPI_Irecv(&shared_buffer, (radius*2 + 1)*(radius*2 + 1), MPI_SHORT, 0, 0, MPI_COMM_WORLD);

          if (proc_rank == rank)
          {
            output[index] = getVisibilityInAreaOfInterest(x, y, radius, shared_buffer, width, height);
          }
        }
    }
  
  if (proc_rank == 0)
  {
    f = fopen("srtm_14_04_6000x6000_int32_distCPU_10.raw", "wb");
    fwrite(output, sizeof(uint32_t), width * height, f);
    fclose(f);
  }


  MPI_Finalize();

  return 0;
}
