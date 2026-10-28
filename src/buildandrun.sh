#!/bin/bash

LIB_FMODSTUDIO=vendor/FMOD/lib/
LIB_FMODCORE=vendor/FMOD/lib

INC_FMODSTUDIO=vendor/FMOD/inc/
INC_FMODCORE=vendor/FMOD/inc

g++ -std=c++17 -o bin/game main.cpp common.cpp common_platform.cpp -I . -I $INC_FMODSTUDIO -I $INC_FMODCORE -L $LIB_FMODSTUDIO -L $LIB_FMODCORE -lfmod -lfmodstudio 
#-lGL -lm -lpthread -ldl -lrt -lX11 
cp -r assets bin/ 
export LD_LIBRARY_PATH=$LIB_FMODSTUDIO:$LIB_FMODCORE:$LD_LIBRARY_PATH
./bin/game