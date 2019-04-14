git clone https://github.com/mgorny/cpuid2cpuflags
cd cpuid2cpuflags
aclocal
autoconf
autoheader
mkdir -p build-aux
automake --add-missing
./configure
make
sudo cp cpuid2cpuflags /usr/bin/
cd ../
rm -rf cpuid2cpuflags
