#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <chrono>
#include <omp.h>

int thread_count = omp_get_max_threads();


std::vector<unsigned short int> readInFile(std::string fileName);
void visiblePoints(int numRows, int numColumns, uint8_t radius, unsigned short* data, uint32_t *out);

void getNormalVisibility(int8_t sign, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t *observationElevation, unsigned short *data, char *visiblePoints, int16_t *leftX, int16_t *topY, uint16_t *visiblePointsWidth, int width);
void getInverseVisibility(int8_t sign, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t *observationElevation, unsigned short *data, char *visiblePoints, int16_t *leftX, int16_t *topY, uint16_t *visiblePointsWidth, int width);
void getVisibility(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, unsigned short *data, char *visiblePoints, int16_t *leftX, int16_t *topY, uint16_t *visiblePointsWidth, int width);
uint32_t getVisibilityInAreaOfInterest(uint16_t x0, uint16_t y0, uint8_t radius, unsigned short *data, int width, int height);

int main(int argc, char* argv[]) {
  std::string fileName;
  std::vector<unsigned short int> srtm_grid;
  const int NUM_COLUMNS = 6000;
  const int NUM_ROWS = 6000;
  uint8_t radius = 10;


  // Store the file name from the command line.
  if (argc >= 2) {
    fileName = argv[1]; 
  } else {
    printf("Invalid Arguments: Must supply name of input file");
    return -1;
  }

  if (argc == 3)
  {
    thread_count = std::atoi(argv[2]);
  }

  // Store the topological data from the input file.
  srtm_grid = readInFile(fileName);

  uint32_t *out = (uint32_t *)malloc(NUM_COLUMNS * NUM_ROWS * sizeof(uint32_t));
  memset(out, 0, NUM_COLUMNS * NUM_ROWS * sizeof(uint32_t));

  // Calculate the viewshed and write to file. 
  std::chrono::time_point<std::chrono::system_clock> start_time = std::chrono::system_clock::now();
  visiblePoints(NUM_ROWS, NUM_COLUMNS, radius, srtm_grid.data(), out);
  std::chrono::time_point<std::chrono::system_clock> end_time = std::chrono::system_clock::now();

  std::chrono::duration<double> difference = end_time - start_time;

  printf("Operation Completed.\n Time taken: %f s \n", difference.count());

  return 0;
}


// Reads input file into a vector
// Input: Name of the file
// Output: A vector containing the entire file.
std::vector<unsigned short int> readInFile(std::string fileName) {
  // Open a binary file stream.
  std::ifstream srtm(fileName, std::ios::in | std::ios::binary | std::ios::ate);
  // Get the size of the file.
  long size;
  srtm.seekg(0, std::ios::end);
  size = srtm.tellg();
  srtm.seekg(0L, std::ios::beg);

  if (size % sizeof(unsigned short int) == 0) {
    std::vector<unsigned short int> file_data(size/sizeof(unsigned short int));

    // Read the file into the vector.
    srtm.read((char*) file_data.data(), size);
    srtm.close();

    return file_data;
  } else {
    // Exit the program if there is more than just unsigned ints.
    printf("Error: Incompatible File Size");
    exit(EXIT_FAILURE);
  }
}

// Determines the number of visible pixels within a certain radius from a given center. 
void visiblePoints(int numRows, int numColumns, uint8_t radius, unsigned short* data, uint32_t *out) {
  FILE* outFile = fopen("srtm_14_04_6000x6000_int32_serial_10.raw", "wb");
    #pragma omp parallel for num_threads(thread_count) collapse(2)
  for (int y = 0; y < numRows; y++) {
      for (int x = 0; x < numColumns; x++) {
          uint32_t visiblePoints = getVisibilityInAreaOfInterest(x, y, radius, data, numColumns, numRows); // Calculate number of visible points.
          out[y * numRows + x] = visiblePoints;
      }
  }

  fclose(outFile);
}

void getNormalVisibility(
    int8_t sign,
    uint16_t x0,
    uint16_t y0,
    uint16_t x1,
    uint16_t y1,
    uint16_t *observationElevation,
    unsigned short *data,
    char *visiblePoints,
    int16_t *leftX,
    int16_t *topY,
    uint16_t *visiblePointsWidth,
    int width
) {
    char finished = 0;
    int16_t deltaX = abs(x1 - x0);
    int16_t deltaY = y1 - y0;
    
    double maxSlope = -45;
    
    char incrementY = 1;
    
    if (deltaY < 0) {
        incrementY = -1;
        deltaY = -deltaY;
    }
    
    int error = 2 * deltaY - deltaX;
    
    uint16_t x = x0;
    uint16_t y = y0;
    
    while(!finished) {
        if (x == x1) {
            finished = 1;
        }
        
        double elevation = data[y * width + x] - *observationElevation;
        double distance = sqrt((x - x0) * (x - x0) + (y - y0) * (y - y0));
        double slope = elevation / distance;
        
        if (slope > maxSlope) {
            visiblePoints[(y - *topY) * *visiblePointsWidth + (x - *leftX)] = 1;
            maxSlope = slope;
        }
        
        // printf("%f %f %f (%d, %d) %d %d\n", elevation, distance, slope, x, y, data[(y * width + x)], visiblePoints[(y - *topY) * *visiblePointsWidth + (x - *leftX)]);
        
        if (error > 0) {
            y += incrementY;
            error -= 2 * deltaX;
        }
        
        error += 2 * deltaY;
        
        x += sign;
    }
}

void getInverseVisibility(
    int8_t sign,
    uint16_t x0,
    uint16_t y0,
    uint16_t x1,
    uint16_t y1,
    uint16_t *observationElevation,
    unsigned short *data,
    char *visiblePoints,
    int16_t *leftX,
    int16_t *topY,
    uint16_t *visiblePointsWidth,
    int width
) {
    char finished = 0;
    int16_t deltaX = x1 - x0;
    int16_t deltaY = abs(y1 - y0);
    
    double maxSlope = -45;
    
    char incrementX = 1;
    
    if (deltaX < 0) {
        incrementX = -1;
        deltaX = -deltaX;
    }
    
    int error = 2 * deltaX - deltaY;
    
    uint16_t x = x0;
    uint16_t y = y0;
    
    while (!finished) {
        if (y == y1) {
            finished = 1;
        }
        
        double elevation = data[y * width + x] - *observationElevation;
        double distance = sqrt((x - x0) * (x - x0) + (y - y0) * (y - y0));
        double slope = elevation / distance;
        
        if (slope > maxSlope) {
            visiblePoints[(y - *topY) * *visiblePointsWidth + (x - *leftX)] = 1;
            maxSlope = slope;
        }
        
        // printf("%f %f %f (%d, %d) %d %d\n", elevation, distance, slope, x, y, data[(y * width + x)], visiblePoints[(y - *topY) * *visiblePointsWidth + (x - *leftX)]);
        
        if (error > 0) {
            x += incrementX;
            error -= 2 * deltaY;
        }
        
        error += 2 * deltaX;
        
        y += sign;
    }
}

void getVisibility(
    uint16_t x0,
    uint16_t y0,
    uint16_t x1,
    uint16_t y1,
    unsigned short *data,
    char *visiblePoints,
    int16_t *leftX,
    int16_t *topY,
    uint16_t *visiblePointsWidth,
    int width
) {
    uint16_t observationElevation = data[y0 * width + x0];
    
    if (abs(y1 - y0) < abs(x1 - x0)) {
        if (x0 > x1) {
            getNormalVisibility(-1, x0, y0, x1, y1, &observationElevation, data, visiblePoints, leftX, topY, visiblePointsWidth, width);
        }
        else {
            getNormalVisibility(1, x0, y0, x1, y1, &observationElevation, data, visiblePoints, leftX, topY, visiblePointsWidth, width);
        }
    }
    else {
        if (y0 > y1) {
            getInverseVisibility(-1, x0, y0, x1, y1, &observationElevation, data, visiblePoints, leftX, topY, visiblePointsWidth, width);
        }
        else {
            getInverseVisibility(1, x0, y0, x1, y1, &observationElevation, data, visiblePoints, leftX, topY, visiblePointsWidth, width);
        }
    }
}

uint32_t getVisibilityInAreaOfInterest(
    uint16_t x0,
    uint16_t y0,
    uint8_t radius,
    unsigned short *data,
    int width,
    int height
) {
    char *visiblePoints;
    uint32_t totalVisiblePoints = 0;
    uint16_t visiblePointsWidth = radius * 2 + 1;
    
    int16_t leftX   = (x0 - radius) < 0 ? 0 : (x0 - radius) ;
    int16_t topY    = (y0 - radius) < 0 ? 0 : (y0 - radius) ;
    int16_t rightX  = (x0 + radius) >= width  ? width - 1  : (x0 + radius) ;
    int16_t bottomY = (y0 + radius) >= height ? height - 1 : (y0 + radius) ;
    
    uint16_t x = leftX;
    uint16_t y = topY;
    
    // printf("%d %d %d %d\n", leftX, topY, rightX, bottomY);
    
    visiblePoints = (char*) calloc(visiblePointsWidth * visiblePointsWidth, sizeof(char));
    
    for (; x < rightX; x++) {
        getVisibility(x0, y0, x, y, data, visiblePoints, &leftX, &topY, &visiblePointsWidth, width);
    }
    getVisibility(x0, y0, x, y, data, visiblePoints, &leftX, &topY, &visiblePointsWidth, width);
    
    for (y += 1; y < bottomY; y++) {
        getVisibility(x0, y0, x, y, data, visiblePoints, &leftX, &topY, &visiblePointsWidth, width);
    }
    getVisibility(x0, y0, x, y, data, visiblePoints, &leftX, &topY, &visiblePointsWidth, width);
    
    for (x -= 1; x > leftX; x--) {
        getVisibility(x0, y0, x, y, data, visiblePoints, &leftX, &topY, &visiblePointsWidth, width);
    }
    getVisibility(x0, y0, x, y, data, visiblePoints, &leftX, &topY, &visiblePointsWidth, width);
    
    for (y -= 1; y > topY; y--) {
        getVisibility(x0, y0, x, y, data, visiblePoints, &leftX, &topY, &visiblePointsWidth, width);
    }
    
    for (int i = 0; i < (visiblePointsWidth * visiblePointsWidth); i++) {
        totalVisiblePoints += visiblePoints[i];
    }
    
    free(visiblePoints);
    
    return totalVisiblePoints;
}
