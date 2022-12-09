# compile all required files
g++ Serial.cpp -o serial
g++ -fopenmp MultiCPU.cpp -o multicpu
nvcc -c MultiGPU.cu -o multigpu.o
nvcc multigpu.o -o multigpu -lcudart
mpicc DistCPU.c -o distcpu
mpicc DistGPU -o distgpu.o
mpicc distgpu.o multigpu.o -o distgpu -lcudart

# run all binaries
./serial srtm_14_04_6000x6000_short16.raw
./multicpu srtm_14_04_6000x6000_short16.raw 16
srun ./multigpu
mpiexec -n 6 ./distcpu
mpiexec -n 6 ./distgpu

# output file names
let serial = "srtm_14_04_6000x6000_int32_serial_10.raw"
let multicpu = "srtm_14_04_6000x6000_int32_cpu_10.raw"
let multigpu = "srtm_14_04_6000x6000_int32_gpu_10.raw"
let distcpu = "srtm_14_04_6000x6000_int32_distCPU_10.raw"
let distgpu = "srtm_14_04_6000x6000_int32_distGPU_10.raw"


# check all files to validate that they are all the same
let error = 0
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
