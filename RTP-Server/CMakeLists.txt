cmake_minimum_required(VERSION 3.10)
project(RTPServer)

set(CMAKE_C_STANDARD 99)

add_executable(RTPServer main.c RTPEnc.c RTPEnc.h Utils.c Utils.h AVC.c AVC.h Network.c Network.h)