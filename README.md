# uwpmp - A libunwind Based Wallclock Profiler

### Build
```
git clone https://github.com/markhpc/uwpmp.git
cd uwpmp
mkdir build
cd build
cmake ..
make
```

### Build on OpenShift Container Platform ( OCP )
```
- enter in debug mode on targeted Openshift worker node 
# oc debug node/<node>
# chroot /host

- run toolbox container 
# toolbox

- install git / wget 

# dnf install git wget 

- either subscribe machine to RHEL channels, or get Centos 8 Stream repository from 
https://centos.pkgs.org/8-stream/centos-baseos-x86_64/ 

# dnf install http://mirror.centos.org/centos/8-stream/BaseOS/aarch64/os/Packages/centos-gpg-keys-8-6.el8.noarch.rpm
# dnf install http://mirror.centos.org/centos/8-stream/BaseOS/aarch64/os/Packages/centos-stream-repos-8-6.el8.noarch.rpm

Install remaining packages necessary for build 
# dnf install clang cmake gcc-c++ gcc elfutils-libelf-devel autoconf automake libtool gcc-toolset-11-elfutils-libelf-devel elfutils-devel 

- run build commands from Build section 

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

