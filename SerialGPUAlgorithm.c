#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

int width;
int height;

void getNormalVisibility(
    int8_t sign,
    uint16_t x0,
    uint16_t y0,
    uint16_t x1,
    uint16_t y1,
    uint16_t *observationElevation,
    short *data,
    char *visiblePoints,
    int16_t *leftX,
    int16_t *topY,
    uint16_t *visiblePointsWidth
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
    short *data,
    char *visiblePoints,
    int16_t *leftX,
    int16_t *topY,
    uint16_t *visiblePointsWidth
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
    short *data,
    char *visiblePoints,
    int16_t *leftX,
    int16_t *topY,
    uint16_t *visiblePointsWidth
) {
    uint16_t observationElevation = data[y0 * width + x0];
    
    if (abs(y1 - y0) < abs(x1 - x0)) {
        if (x0 > x1) {
            getNormalVisibility(-1, x0, y0, x1, y1, &observationElevation, data, visiblePoints, leftX, topY, visiblePointsWidth);
        }
        else {
            getNormalVisibility(1, x0, y0, x1, y1, &observationElevation, data, visiblePoints, leftX, topY, visiblePointsWidth);
        }
    }
    else {
        if (y0 > y1) {
            getInverseVisibility(-1, x0, y0, x1, y1, &observationElevation, data, visiblePoints, leftX, topY, visiblePointsWidth);
        }
        else {
            getInverseVisibility(1, x0, y0, x1, y1, &observationElevation, data, visiblePoints, leftX, topY, visiblePointsWidth);
        }
    }
}

uint32_t getVisibilityInAreaOfInterest(
    uint16_t x0,
    uint16_t y0,
    uint8_t radius,
    short *data
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
    
    visiblePoints = calloc(sizeof(char), visiblePointsWidth * visiblePointsWidth);
    
    for (; x < rightX; x++) {
        getVisibility(x0, y0, x, y, data, visiblePoints, &leftX, &topY, &visiblePointsWidth);
    }
    getVisibility(x0, y0, x, y, data, visiblePoints, &leftX, &topY, &visiblePointsWidth);
    
    for (y += 1; y < bottomY; y++) {
        getVisibility(x0, y0, x, y, data, visiblePoints, &leftX, &topY, &visiblePointsWidth);
    }
    getVisibility(x0, y0, x, y, data, visiblePoints, &leftX, &topY, &visiblePointsWidth);
    
    for (x -= 1; x > leftX; x--) {
        getVisibility(x0, y0, x, y, data, visiblePoints, &leftX, &topY, &visiblePointsWidth);
    }
    getVisibility(x0, y0, x, y, data, visiblePoints, &leftX, &topY, &visiblePointsWidth);
    
    for (y -= 1; y > topY; y--) {
        getVisibility(x0, y0, x, y, data, visiblePoints, &leftX, &topY, &visiblePointsWidth);
    }
    
    for (int i = 0; i < (visiblePointsWidth * visiblePointsWidth); i++) {
        totalVisiblePoints += visiblePoints[i];
    }
    
    free(visiblePoints);
    
    return totalVisiblePoints;
}

int main() {
    short *data;
    uint8_t radius = 10;
    uint32_t size;
    
    width = 6000;
    height = 6000;
    
    FILE* f = fopen("srtm_14_04_6000x6000_short16.raw", "rb");
    
    fseek(f, 0, SEEK_END);
    size = ftell(f) / sizeof(short);
    fseek(f, 0, SEEK_SET);
    
    data = malloc(sizeof(short) * size);
    
    fread(data, sizeof(short), size, f);
    
    fclose(f);
    
    f = fopen("srtm_14_04_6000x6000_int32_serial_10.raw", "wb");
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // printf("%d %d\n", x, y);
            uint32_t visiblePoints = getVisibilityInAreaOfInterest(x, y, radius, data);
            fwrite(&visiblePoints, sizeof(uint32_t), 1, f);
        }
    }
    
    fclose(f);
    
    free(data);
    
    return 0;
}
