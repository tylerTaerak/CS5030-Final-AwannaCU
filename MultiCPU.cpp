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

struct Pixel;

int width; /* width of dataset */ 
int height; /* height of dataset */ 
size_t size; /* full size of dataset */
int thread_count = omp_get_max_threads(); /* number of threads for program */

// container for pixel and correlating elevation at that point in data
struct Pixel 
{
  Pixel() = default;

  Pixel(int x, int y, short z)
{
    this->x = x;
    this->y = y;
    this->z = z;
    }

    bool operator== (const Pixel &other)
{
      return x == other.x && y == other.y;
    }

  int x;
  int y;
  short z;
  std::vector<Pixel> visible;
  };

// returns true if path is free, false if collision detected
// collision in line of sight found
//
// collision happens when data[x * width + y] > pixel.z
//
// this part of the algorithm works as intended
bool checkCollision(std::vector<Pixel> path, short data[])
{
  for (Pixel &p : path)
  {
    if (data[p.x * width + p.y] > p.z)
    {
      return false;
    }
  }
  return true;
}


// Bresenham line algorithm for 3 dimensions (3rd dimension is data elevation)
// drawn from https://www.geeksforgeeks.org/bresenhams-algorithm-for-3-d-line-drawing/
// 
// This part of the algorithm works as intended
std::vector<Pixel> Bresenham(int x1, int y1, int z1, int x2, int y2, int z2)
{
  std::vector<Pixel> pixels;

  pixels.emplace_back(x1, y1, z1);

  int dx = std::abs(x2 - x1);
  int dy = std::abs(y2 - y1);
  int dz = std::abs(z2 - z1);

  int xs, ys, zs;

  if (x2 > x1)
  {
    xs = 1;
  }
  else {
    xs = -1;
  }
  if (y2 > y1)
  {
    ys = 1;
  }
  else {
    ys = -1;
  }
  if (z2 > z1)
  {
    zs = 1;
  }
  else {
    zs = -1;
  }

  if (dx >= dy && dx >= dz)
  {
    int p1 = 2 * dy - dx;
    int p2 = 2 * dz - dx;
    while (x1 != x2)
    {
      x1 += xs;
      if (p1 >= 0)
        {
          y1 += ys;
          p1 -= 2 * dx;
        }
      if (p2 >= 0)
        {
          z1 += zs;
          p2 -= 2 * dx;
        }
      p1 += 2 * dy;
      p2 += 2 * dz;
      pixels.emplace_back(x1, y1, z1);
    }
  }

  else if (dy >= dx && dy >= dz){
    int p1 = 2 * dx - dy;
    int p2 = 2 * dz - dy;
    while (y1 != y2)
    {
      y1 += ys;
      if (p1 >= 0)
        {
          x1 += xs;
          p1 -= 2 * dy;
        }
      if (p2 >= 0)
        {
          z1 += zs;
          p2 -= 2 * dy;
        }
      p1 += 2 * dx;
      p2 += 2 * dz;
      pixels.emplace_back(x1, y1, z1);
    }
  }

  else {
    int p1 = 2 * dx - dz;
    int p2 = 2 * dy - dz;
    while (z1 != z2)
    {
      z1 += zs;
      if (p1 >= 0)
        {
          x1 += xs;
          p1 -= 2 * dz;
        }
      if (p2 >= 0)
        {
          y1 += ys;
          p2 -= 2 * dz;
        }
      p1 += 2 * dx;
      p2 += 2 * dy;
      pixels.emplace_back(x1, y1, z1);
    }
  }

  return pixels;
}

// this is the part that needs to be fixed
uint32_t computeVisibilityAtPoint(int x, int y, short data[])
{

  std::deque<Pixel> q;
  std::vector<Pixel> visited;
  q.emplace_back(x, y, data[x * width + y]);

  Pixel curr;
  for (int i=0; i<100; i++)
  {
      curr = q.front();
      q.pop_front();
      // bfs search until we reach the 100 pixel radius 

      // add adjacent pixels to q if within 0 - width or height
      std::vector<Pixel> next;

      if (!(curr.x+1 > width))
      {
        next.emplace_back(curr.x + 1, curr.y, data[(curr.x+1) * width + curr.y]);
      }
      if (!(curr.x-1 < 0))
      {
        next.emplace_back(curr.x-1, curr.y, data[(curr.x-1) * width + curr.y]);
        if (!(curr.y+1 > height))
        {
          next.emplace_back(curr.x-1, curr.y+1, data[(curr.x-1) * width + curr.y+1]);
        }

        if (!(curr.y-1 < 0))
        {
          next.emplace_back(curr.x-1, curr.y-1, data[(curr.x-1) * width + curr.y-1]);
        }
      }
      if (!(curr.y+1 > height))
      {
        next.emplace_back(curr.x, curr.y+1, data[curr.x * width + curr.y+1]);
      }
      if (!(curr.y-1 < 0))
      {
        next.emplace_back(curr.x, curr.y-1, data[curr.x * width + curr.y-1]);
      }

      for (Pixel &p : next)
      {
        visited.push_back(p);
        q.push_back(p);
      }
  }

  while (!q.empty())
  {
    visited.push_back(q.front());
    q.pop_front();
  }

  int visible_pixels = 0;
  for (Pixel &p : visited)
  {
    if (checkCollision(Bresenham(x, y, data[x * width + y], p.x, p.y, p.z), data))
    {
      visible_pixels++;
    }
  }

  return visible_pixels;
}

void computeVisibility(short in[], short out[] /*size_t row, size_t col*/)
{

  /**
    * this is where the multi-cpu implementation will go
    *
    * Here's how it will work:
    *
    * 1. Send a point to each thread
    * 2. Each thread calculates visibility from point using Bresenham's
    *   line algorithm
    * 3. give results back to main thread in outMem
    */

  // shared variables - to be utilized by each thread
  int i; // indices for data to be passed to point visibility calculation
  int j;
  short *temp = (short *)malloc(size); // temporary memory for calculation

  // calculate visiblitiy for each pixel
#pragma omp parallel for num_threads(thread_count) collapse(2)
  for (i=0; i<width; i++)
    {
      for (j=0; j<height; j++)
      {
        temp[i * width + j] = computeVisibilityAtPoint(i, j, in);
        if ((i * width + j) % 100000 == 0)
        {
          std::cout << "Pixel " << i << ", " << j << " complete!" << std::endl;
        }
      }
    }

  std::cout << "Pixel analysis complete" << std::endl;

  // copy memory to out (parallelized)
  #pragma omp parallel for num_threads(thread_count)
  for (i=0; i<width * height; i++)
    {
      memcpy(&out[i], &temp[i], sizeof(short));
    }
}

int main(int argc, char **argv)
{
  // initialize dimensional variables
  width = 6000;
  height = 6000;
  size = width * height * sizeof(short);

  // if program is run without arguments, program will use every available thread
  // otherwise, the first argument defines the number of threads
  if (argc > 1)
  {
    thread_count = std::atoi(argv[1]);
  }

  // initialize memory variables
  short *dataMem = (short *)malloc(size);
  short *outMem = (short *)malloc(size);

  // read in file
  FILE *data = fopen("./data/srtm_14_04_6000x6000_short16.raw", "rb");

  fread((char *)dataMem, sizeof(short), width * height, data);

  fclose(data);

  // compute visibility of each pixel of dataset
  computeVisibility(dataMem, outMem);

  // write out file
  FILE *outData = fopen("./data/out_awannacu.raw", "wb");

  fwrite((char *)outMem, sizeof(short), width * height, data);

  fclose(data);
  
  return 0;
}
