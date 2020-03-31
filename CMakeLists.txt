# this file is here just for faster development with my editor of choice CLion which supports CMake projects
# this won't produce correct python extension, you must go through `setup.py stpyv8` as documented

cmake_minimum_required(VERSION 3.15)
project(stpyv8)

set(Boost_USE_MULTITHREADED ON)
set(CMAKE_FIND_FRAMEWORK NEVER)

find_package(Python3)
find_package(Boost COMPONENTS python${PYTHON_VERSION_MAJOR}${PYTHON_VERSION_MINOR})
find_package(PythonInterp 3)
find_package(PythonLibs 3 REQUIRED)

set(V8_HOME "v8")
if (DEFINED ENV{V8_HOME})
    set(V8_HOME $ENV{V8_HOME})
endif ()

message(STATUS "PYTHON_LIBRARIES = ${PYTHON_LIBRARIES}")
message(STATUS "PYTHON_EXECUTABLE = ${PYTHON_EXECUTABLE}")
message(STATUS "PYTHON_INCLUDE_DIRS = ${PYTHON_INCLUDE_DIRS}")
message(STATUS "Boost_LIBRARIES = ${Boost_LIBRARIES}")
message(STATUS "V8_HOME = ${V8_HOME}")

include_directories(${Boost_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS})
link_libraries(${Boost_LIBRARIES} ${PYTHON_LIBRARIES}) # Deprecated but so convenient!

set(CMAKE_CXX_STANDARD 11)

file(GLOB_RECURSE HEADERS src/*.h)
file(GLOB_RECURSE SOURCES src/*.cpp)

include_directories(${V8_HOME}/include)
link_directories(out.gn/x64.release.sample/obj)

add_library(stpyv8 ${SOURCES} ${HEADERS})