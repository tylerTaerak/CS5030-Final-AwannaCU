# compile all required files
g++ Serial.cpp -o serial
g++ -fopenmp MultiCPU.cpp -o multicpu
nvcc MultiGPU -o multigpu
mpicc DistCPU.c -o distcpu
mpicc DistGPU -o distgpu
mpicc distgpu multigpu -lcudart

# run all binaries
./serial
./multicpu
./multigpu
./distcpu
./distgpu

# output file names
serial = "srtm_14_04_6000x6000_int32_serial_10.raw"
multicpu = "srtm_14_04_6000x6000_int32_cpu_10.raw"
multigpu = "srtm_14_04_6000x6000_int32_gpu_10.raw"
distcpu = "srtm_14_04_6000x6000_int32_distCPU_10.raw"
distgpu = "srtm_14_04_6000x6000_int32_distGPU_10.raw"


# check all files to validate that they are all the same
error = 0
if diff $serial $multicpu
  echo "multiCPU output differs from serial"
  error = 1
fi

if diff $serial $multigpu
  echo "multiGPU output differs from serial"
  error = 1
fi

if diff $serial $distcpu
  echo "distributed CPU output differs from serial"
  error = 1
fi

if diff $serial $distgpu
  echo "distributed GPU output differs from serial"
  error = 1
fi

if $error
  exit 1

