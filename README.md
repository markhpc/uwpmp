# uwpmp - A libunwind Based Wallclock Profiler

### Prerequisites
You'll need the following packages installed prior to building the executable.
- git
- cmake
- gcc-c++
- elfutils-libelf-devel
- elfutils-devel
- autoconf
- automake
- libtool
  
For CentOS Stream 8
```
dnf install -y git cmake gcc-c++ elfutils-libelf-devel elfutils-devel autoconf automake libtool
```


### Build 
```
git clone https://github.com/markhpc/uwpmp.git
cd uwpmp
mkdir build
cd build
cmake ..
make
```

### Help
```
Usage:
  ./unwindpmp [OPTION...]

  -h, --help           show this help message and exit
  -p, --pid arg        PID of the process to attach to.
  -s, --sleep arg      The time to sleep between samples in ms.
  -n, --samples arg    The number of samples to collect.
  -t, --threshold arg  Ignore results below the threshold when making the
                       callgraph.
  -v, --invert         Print inverted callgraph.
  -w, --max_width arg  Set the display width (default is terminal width)
  -r, --truncate       Truncate lines to the terminal width
```
