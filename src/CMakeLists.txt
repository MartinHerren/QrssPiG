# Generate and include Config.h
configure_file ("Config.h.in" "${PROJECT_BINARY_DIR}/Config.h")
include_directories("${PROJECT_BINARY_DIR}")

# Set compiler options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wextra")
add_definitions(-std=c++11)

add_executable (QrssPiG main.cpp QrssPiG.cpp QGFft.cpp QGImage.cpp QGUploader.cpp QGLocalUploader.cpp QGSCPUploader.cpp)
target_link_libraries(QrssPiG pthread)
target_link_libraries(QrssPiG boost_program_options)
target_link_libraries(QrssPiG fftw3)
target_link_libraries(QrssPiG gd)
target_link_libraries(QrssPiG ssh)
target_link_libraries(QrssPiG yaml-cpp)

add_executable (IQGen IQGen.cpp)
target_link_libraries(IQGen boost_program_options)
