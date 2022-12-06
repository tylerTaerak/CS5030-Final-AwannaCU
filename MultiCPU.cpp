/**
* This is the file for the Parallel CPU implementation
* of the AwannaCU Project for CS5030. It uses SRTM data
* to create a map of visible area
*/

#include <cmath> /* abs */
#include <fstream> /* fopen, fclose, fread, fwrite */
#include <cstdlib> /* malloc */
#include <cstring> /* memcpy */
#include <deque> /* deque */
#include <vector> /* vector */
#include <iostream>
#include <algorithm> /* count */
#include <omp.h> /* OpenMP Functionality */

#define MASK_SIZE 40401
#define SUB_WIDTH 201

int thread_count = omp_get_max_threads(); /* number of threads for program */

void getVisibility(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, short *data, char *visible, int16_t leftX, int16_t topY, uint16_t width)
{
  short elevation = data[y0 * width + x0];

  int dx = std::abs(x1 - x0);
  int dy = std::abs(y1 - y0);

  int xs, ys;

  if (x1 > x0)
  {
    xs = 1;
  }
  else {
    xs = -1;
  }
  if (y1 > y0)
  {
    ys = 1;
  }
  else {
    ys = -1;
  }

  double maxSlope = -45;

  if (dx >= dy)
  {
    int error = 2 * dy - dx;
    while (x0 != x1)
    {
      short obs = data[y0 * width + x0] - elevation;
      double distance = std::sqrt((double)((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0)));
      double slope = obs / distance;

      if (slope > maxSlope)
      {
        visible[(y0 - topY) * SUB_WIDTH + (x0 - leftX)] = 1;
        maxSlope = slope;
      }

      x0 += xs;
      if (error >= 0)
        {
          y0 += ys;
          error -= 2 * dx;
        }
      error += 2 * dy;
    }
  }

  else {
    int error = 2 * dx - dy;
    while (y0 != y1)
    {
      short obs = data[y0 * width + x0] - elevation;
      double distance = std::sqrt((double)((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0)));
      double slope = obs / distance;

      if (slope > maxSlope)
      {
        visible[(y0 - topY) * SUB_WIDTH + (x0 - leftX)] = 1;
        maxSlope = slope;
      }

      y0 += ys;
      if (error >= 0)
        {
          x0 += xs;
          error -= 2 * dy;
        }
      error += 2 * dx;
    }
  }
}

uint32_t getVisibilityAtPoint(uint16_t x0, uint16_t y0, uint8_t radius, short *data, char *visible, uint16_t width, uint16_t height)
{
  uint32_t totalVisiblePoints = 0;
    
  int16_t leftX   = (x0 - radius) < 0 ? 0 : (x0 - radius) ;
  int16_t topY    = (y0 - radius) < 0 ? 0 : (y0 - radius) ;
  int16_t rightX  = (x0 + radius) >= width  ? width - 1  : (x0 + radius) ;
  int16_t bottomY = (y0 + radius) >= height ? height - 1 : (y0 + radius) ;
  
  uint16_t x = leftX;
  uint16_t y = topY;

  for (; x<rightX; x++)
  {
    getVisibility(x0, y0, x, y, data, visible, leftX, topY, width);
  }
  getVisibility(x0, y0, x, y, data, visible, leftX, topY, width);

  for (y++; y < bottomY; y++)
  {
    getVisibility(x0, y0, x, y, data, visible, leftX, topY, width);
  }
  getVisibility(x0, y0, x, y, data, visible, leftX, topY, width);

  for (x--; x > leftX; x--)
  {
    getVisibility(x0, y0, x, y, data, visible, leftX, topY, width);
  }
  getVisibility(x0, y0, x, y, data, visible, leftX, topY, width);

  for (y--; y > topY; y--)
  {
    getVisibility(x0, y0, x, y, data, visible, leftX, topY, width);
  }

  for (int i=0; i<(SUB_WIDTH * SUB_WIDTH); i++)
  {
    totalVisiblePoints += visible[i];
  }

  return totalVisiblePoints;
}

void calcViewshed(short *data, uint32_t *out, uint8_t radius, uint16_t width, uint16_t height)
{
  char visiblePoints[MASK_SIZE];
  memset(visiblePoints, 0, MASK_SIZE);
  int i, j;

#pragma omp parallel for num_threads(thread_count) collapse(2)
  for (i=0; i<width; i++)
  {
    for (j=0; j<height; j++)
    {
      out[j * width + i] = getVisibilityAtPoint(i, j, radius, data, visiblePoints, width, height);
      if ((j * width + i) % 100000 == 0)
      {
        std::cout << "Pixel " << j << ", " << i << " complete!" << std::endl;
      }
    }
  }

  std::cout << "Pixel analysis complete" << std::endl;
}


int main(int argc, char **argv)
{
  // initialize dimensional variables
  uint16_t width = 6000;
  uint16_t height = 6000;
  uint32_t size = width * height;

  // if program is run without arguments, program will use every available thread
  // otherwise, the first argument defines the number of threads
  if (argc > 1)
  {
    thread_count = std::atoi(argv[1]);
  }

  // initialize memory variables
  short *dataMem = (short *)malloc(size * sizeof(short));
  uint32_t *outMem = (uint32_t *)malloc(size * sizeof(uint32_t));

  // read in file
  FILE *data = fopen("./data/srtm_14_04_6000x6000_short16.raw", "rb");

  fread((char *)dataMem, sizeof(short), width * height, data);

  fclose(data);

  // compute visibility of each pixel of dataset
  calcViewshed(dataMem, outMem, 100, width, height);

  // write out file
  FILE *outData = fopen("./data/out_awannacu.raw", "wb");

  fwrite((char *)outMem, sizeof(uint32_t), width * height, data);

  fclose(data);
  
  return 0;
}
