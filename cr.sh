g++ -shared -std=c++11 -pthread -fPIC -g -o libhoard.so malloc-intercept.cpp allocator.cpp tracing.cpp
LD_PRELOAD=./libhoard.so qtcreator
