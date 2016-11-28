configure_file ("Config.h.in" "${PROJECT_BINARY_DIR}/Config.h")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wextra")

add_definitions(-std=c++14)

include_directories("${PROJECT_BINARY_DIR}")

add_executable (QrssPiG QrssPiG.cpp QGFft.cpp QGImage.cpp)
#target_link_libraries(QrssPiG m)
target_link_libraries(QrssPiG fftw3)
target_link_libraries(QrssPiG gd)

add_executable (IQGen IQGen.cpp)
#target_link_libraries(IQGen m)
target_link_libraries(IQGen boost_program_options)
