cmake_minimum_required (VERSION 2.6)

project (QrssPiG)
project (IQGen)

set (QrssPiG_Name QrssPiG)
set (QrssPiG_VERSION_MAJOR 0)
set (QrssPiG_VERSION_MINOR 2)
set (QrssPiG_VERSION_PATCH 0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

subdirs(src)
