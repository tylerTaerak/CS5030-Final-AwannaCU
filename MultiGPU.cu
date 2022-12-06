#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#define MASK_SIZE 40401
#define SUB_WIDTH 201

__device__
void getNormalVisibility(
    int8_t sign,
    uint16_t x0,
    uint16_t y0,
    uint16_t x1,
    uint16_t y1,
    uint16_t observationElevation,
    short *data,
    char *visiblePoints,
    int16_t leftX,
    int16_t topY,
    uint16_t width
) {
    int tdx = threadIdx.x + blockIdx.x * blockDim.x;
    int tdy = threadIdx.y + blockIdx.y * blockDim.y;
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
        
        double elevation = data[y * width + x] - observationElevation;
        double distance = sqrt((double)((x - x0) * (x - x0) + (y - y0) * (y - y0)));
        double slope = elevation / distance;
        
        if (slope > maxSlope) {
            visiblePoints[(y - topY) * SUB_WIDTH + (x - leftX)] = 1;
            maxSlope = slope;
        }
        
        if (error > 0) {
            y += incrementY;
            error -= 2 * deltaX;
        }
        
        error += 2 * deltaY;
        
        x += sign;
    }
}

__device__
void getInverseVisibility(
    int8_t sign,
    uint16_t x0,
    uint16_t y0,
    uint16_t x1,
    uint16_t y1,
    uint16_t observationElevation,
    short *data,
    char *visiblePoints,
    int16_t leftX,
    int16_t topY,
    uint16_t width
) {
    int tdx = threadIdx.x + blockIdx.x * blockDim.x;
    int tdy = threadIdx.y + blockIdx.y * blockDim.y;
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
        
        double elevation = data[y * width + x] - observationElevation;
        double distance = sqrt((double)((x - x0) * (x - x0) + (y - y0) * (y - y0)));
        double slope = elevation / distance;
        
        if (slope > maxSlope) {
            visiblePoints[(y - topY) * SUB_WIDTH + (x - leftX)] = 1;
            maxSlope = slope;
        }
        
        if (error > 0) {
            x += incrementX;
            error -= 2 * deltaY;
        }
        
        error += 2 * deltaX;
        
        y += sign;
    }
}

__device__
void getVisibility(
    uint16_t x0,
    uint16_t y0,
    uint16_t x1,
    uint16_t y1,
    short *data,
    char *visiblePoints,
    int16_t leftX,
    int16_t topY,
    uint16_t width
) {
    uint16_t observationElevation = data[y0 * width + x0];
    
    if (abs(y1 - y0) < abs(x1 - x0)) {
        if (x0 > x1) {
            getNormalVisibility(-1, x0, y0, x1, y1, observationElevation, data, visiblePoints, leftX, topY, width);
        }
        else {
            getNormalVisibility(1, x0, y0, x1, y1, observationElevation, data, visiblePoints, leftX, topY, width);
        }
    }
    else {
        if (y0 > y1) {
            getInverseVisibility(-1, x0, y0, x1, y1, observationElevation, data, visiblePoints, leftX, topY, width);
        }
        else {
            getInverseVisibility(1, x0, y0, x1, y1, observationElevation, data, visiblePoints, leftX, topY, width);
        }
    }
}

__device__
uint32_t getVisibilityInAreaOfInterest(
    uint16_t x0,
    uint16_t y0,
    uint8_t radius,
    short *data,
    char *visiblePoints,
    uint16_t width,
    uint16_t height
) {
    uint32_t totalVisiblePoints = 0;
    
    int16_t leftX   = (x0 - radius) < 0 ? 0 : (x0 - radius) ;
    int16_t topY    = (y0 - radius) < 0 ? 0 : (y0 - radius) ;
    int16_t rightX  = (x0 + radius) >= width  ? width - 1  : (x0 + radius) ;
    int16_t bottomY = (y0 + radius) >= height ? height - 1 : (y0 + radius) ;
    
    uint16_t x = leftX;
    uint16_t y = topY;
    
    for (; x < rightX; x++) {
        getVisibility(x0, y0, x, y, data, visiblePoints, leftX, topY, width);
    }
    getVisibility(x0, y0, x, y, data, visiblePoints, leftX, topY, width);
    
    for (y += 1; y < bottomY; y++) {
        getVisibility(x0, y0, x, y, data, visiblePoints, leftX, topY, width);
    }
    getVisibility(x0, y0, x, y, data, visiblePoints, leftX, topY, width);
    
    for (x -= 1; x > leftX; x--) {
        getVisibility(x0, y0, x, y, data, visiblePoints, leftX, topY, width);
    }
    getVisibility(x0, y0, x, y, data, visiblePoints, leftX, topY, width);
    
    for (y -= 1; y > topY; y--) {
        getVisibility(x0, y0, x, y, data, visiblePoints, leftX, topY, width);
    }
    
    for (int i = 0; i < (SUB_WIDTH * SUB_WIDTH); i++) {
        totalVisiblePoints += visiblePoints[i];
    }
    
    return totalVisiblePoints;
}

__global__
void calcViewshed(short *data, uint32_t *viewshed, uint8_t radius, uint16_t width, uint16_t height) {
    char visiblePoints[MASK_SIZE];
    
    int x = threadIdx.x + blockIdx.x * blockDim.x;
    int y = threadIdx.y + blockIdx.y * blockDim.y;
    
    for (int i = 0; i < MASK_SIZE; i++) {
        visiblePoints[i] = 0;
    }
    
    if (x < width && y < height) {
        viewshed[y * width + x] = getVisibilityInAreaOfInterest(x, y, radius, data, visiblePoints, width, height);
    }
}

/* This function is utilized by the distributed GPU implementation */
extern "C" void startKernel(short *data, uint32_t *viewshed, uint8_t radius, uint16_t width, uint16_t height)
{
  short *data_d;
  uint32_t *viewshed_d;

  cudaMalloc((void**) &data_d, width * height * sizeof(short));
  cudaMalloc((void**) &viewshed_d, width * height * sizeof(uint32_t));

  cudaMemcpy(data_d, data, width * height * sizeof(short), cudaMemcpyHostToDevice);

  /* dimensions used to calculate a 1000 x 1000 space */
  dim3 DimGrid(100, 100, 1); 
  dim3 DimBlock(10, 10, 1);

  calcViewshed<<<DimGrid, DimBlock>>>(data_d, viewshed_d, radius, width, height);

  cudaMemcpy(viewshed, viewshed_d, width * height * sizeof(uint32_t), cudaMemcpyDeviceToHost);
}


int main() {
    short *data_h;
    uint32_t *viewshed_h;
    short *data_d;
    uint32_t *viewshed_d;
    
    // cudaEvent_t startEvent, stopEvent;    // CUDA events used to compute the elapsed time of the kernal functions.
    // cudaEventCreate(&startEvent);
    // cudaEventCreate(&stopEvent);
    
    // float elapsedTime = 0;                // Actual elapsed time of the kernal functions.
    
    uint8_t radius = 100;
    
    uint32_t size;
    
    uint16_t width = 6000;
    uint16_t height = 6000;
    
    // FILE* f = fopen("test.raw", "rb");
    FILE* f = fopen("srtm_14_04_6000x6000_short16.raw", "rb");
    
    if (f == NULL) {
        printf("Error: input file srtm_14_04_6000x6000_short16.raw could not be opened.\n");
        return 1;
    }
    
    fseek(f, 0, SEEK_END);
    size = ftell(f) / sizeof(short);
    fseek(f, 0, SEEK_SET);
    
    cudaMalloc((void**) &data_d, size * sizeof(short));
    cudaMalloc((void**) &viewshed_d, size * sizeof(uint32_t));
    
    data_h = (short*) malloc(sizeof(short) * size);
    
    fread(data_h, sizeof(short), size, f);
    
    fclose(f);
    
    cudaMemcpy(data_d, data_h, size * sizeof(short), cudaMemcpyHostToDevice);
    
    dim3 DimGrid(375,375,1);
    dim3 DimBlock(16,16,1);
    
    calcViewshed<<<DimGrid, DimBlock>>>(data_d, viewshed_d, radius, width, height);
    
    viewshed_h = (uint32_t*) malloc(sizeof(uint32_t) * size);
    
    cudaMemcpy(viewshed_h, viewshed_d, size * sizeof(uint32_t), cudaMemcpyDeviceToHost);
    
    f = fopen("srtm_14_04_6000x6000_int32_gpu_100.raw", "wb");
    
    fwrite(viewshed_h, sizeof(uint32_t), size, f);
    
    fclose(f);
    
    free(data_h);
    free(viewshed_h);
    
    cudaFree(data_d);
    cudaFree(viewshed_d);
    
    return 0;
}
