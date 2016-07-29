#!/bin/sh -x
g++ -pthread -o Demo.out Demo.cpp -L../lib -lrt -lTDFAPI_v2.7 -I../include
export LD_LIBRARY_PATH=../lib/
./Demo.out 115.238.56.208 10000 zxzqhz3 76117
