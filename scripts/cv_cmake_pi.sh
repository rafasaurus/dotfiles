cmake -DBUILD_TIFF=ON \
  -DBUILD_opencv_java=OFF \
<<<<<<< HEAD
  -DOPENCV_EXTRA_MODULES_PATH=/home/pi/opencv/opencv_contrib-3.4.3/modules \
=======
  -DOPENCV_EXTRA_MODULES_PATH=/home/pi/opencv/opencv_contrib-3.4.2/modules \
>>>>>>> 553ec57ef428dd67c6b55b4e113b2aca6e20da86
  -DWITH_CUDA=OFF \
  -DENABLE_NEON=ON \
  -DWITH_OPENGL=ON \
  -DWITH_OPENCL=ON \
  -DWITH_GTK_2_X=ON \
  -DWITH_IPP=ON \
  -DWITH_TBB=ON \
  -DWITH_EIGEN=ON \
  -DWITH_V4L=ON \
  -DBUILD_TESTS=OFF \
  -DBUILD_PERF_TESTS=OFF \
  -DBUILD_EXAMPLES=OFF \
  -DINSTALL_PYTHON_EXAMPLES=OFF \
  -DINSTALL_C_EXAMPLES=OFF \
  -DWITH_FFMPEG=ON \
  -DWITH_GSTREAMER=ON \
  -DCMAKE_BUILD_TYPE=RELEASE \
  -DCMAKE_INSTALL_PREFIX=$(python3 -c "import sys; print(sys.prefix)") \
  -DPYTHON_EXECUTABLE=$(which python3) \
  -DPYTHON_INCLUDE_DIR=$(python3 -c "from distutils.sysconfig import get_python_inc; print(get_python_inc())") \
  -DPYTHON_PACKAGES_PATH=$(python3 -c "from distutils.sysconfig import get_python_lib; print(get_python_lib())") ..
