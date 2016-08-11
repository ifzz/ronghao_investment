#!/bin/bash

runtime=/data/runtime/linux

# 动态库的路径
export LD_LIBRARY_PATH=$runtime/so/:$runtime/ctp:$runtime/hh_api

