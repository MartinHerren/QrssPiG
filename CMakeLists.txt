cmake_minimum_required (VERSION 3.0.2)

project (qrsspig)

set (QRSSPIG_NAME QrssPiG)
set (QRSSPIG_VERSION_MAJOR 0)
set (QRSSPIG_VERSION_MINOR 4)
set (QRSSPIG_VERSION_PATCH 0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

subdirs(src)
