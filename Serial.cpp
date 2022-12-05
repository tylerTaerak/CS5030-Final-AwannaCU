#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <string.h>


std::vector<short unsigned int> readInFile(std::string fileName);

int main(int argc, char* argv[]) {
  std::string fileName;
  std::vector<short unsigned int> srtm_grid;

  // Store the file name from the command line.
  if (argc == 2) {
    fileName = argv[1];
  } else {
    printf("Invalid Arguments: Must supply name of input file");
    return -1;
  }

  // Store the topological data from the input file.
  srtm_grid = readInFile(fileName);
  
  return 0;
}


// Reads input file into a vector
// Input: Name of the file
// Output: A vector containing the entire file.
std::vector<short unsigned int> readInFile(std::string fileName) {
  // Open a binary file stream.
  std::ifstream srtm(fileName, std::ios::in | std::ios::binary | std::ios::ate);
  // Get the size of the file.
  long size;
  srtm.seekg(0, std::ios::end);
  size = srtm.tellg();
  srtm.seekg(0L, std::ios::beg);

  if (size % sizeof(short unsigned int) == 0) {
    std::vector<short unsigned int> file_data(size/sizeof(short unsigned int));

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
