# this file is here just for faster development with my editor of choice CLion which supports CMake projects

cmake_minimum_required(VERSION 3.15)
project(stpyv8)

# there is no easy way how I could pass this to CLion ENV => hardcode it here
set(V8_HOME ~/lab/v8_ws/v8)
set(STPYV8_V8_GIT_TAG 8.3.104)
set(STPYV8_DEBUG 1)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(Boost_USE_MULTITHREADED ON)
set(CMAKE_FIND_FRAMEWORK NEVER)

find_package(Python3)
find_package(Boost COMPONENTS python${PYTHON_VERSION_MAJOR}${PYTHON_VERSION_MINOR})
find_package(PythonInterp 3)
find_package(PythonLibs 3 REQUIRED)

if (DEFINED ENV{STPYV8_DEBUG})
    set(STPYV8_DEBUG $ENV{STPYV8_DEBUG})
endif ()

if (NOT DEFINED STPYV8_DEBUG)
    set(STPYV8_DEBUG 0)
endif ()

if (DEFINED ENV{V8_HOME})
    set(V8_HOME $ENV{V8_HOME})
endif ()

if (NOT DEFINED V8_HOME)
    message(FATAL_ERROR "You must set V8_HOME variable")
endif ()

if (DEFINED ENV{STPYV8_V8_GIT_TAG})
    set(STPYV8_V8_GIT_TAG $ENV{STPYV8_V8_GIT_TAG})
endif ()

if (NOT DEFINED STPYV8_V8_GIT_TAG)
    message(FATAL_ERROR "You must set STPYV8_V8_GIT_TAG variable")
endif ()

if (${STPYV8_DEBUG} EQUAL 1)
    set(STPYV8_CONFIG_NAME debug)
else ()
    set(STPYV8_CONFIG_NAME release)
endif ()


message(STATUS "PYTHON_LIBRARIES = ${PYTHON_LIBRARIES}")
message(STATUS "PYTHON_EXECUTABLE = ${PYTHON_EXECUTABLE}")
message(STATUS "PYTHON_INCLUDE_DIRS = ${PYTHON_INCLUDE_DIRS}")
message(STATUS "Boost_LIBRARIES = ${Boost_LIBRARIES}")
message(STATUS "V8_HOME = ${V8_HOME}")
message(STATUS "STPYV8_V8_GIT_TAG = ${STPYV8_V8_GIT_TAG}")
message(STATUS "STPYV8_DEBUG = ${STPYV8_DEBUG}")

include_directories(${Boost_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS})
link_libraries(${Boost_LIBRARIES} ${PYTHON_LIBRARIES})

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE HEADERS src/*.h)
file(GLOB_RECURSE SOURCES src/*.cpp)

include_directories(${V8_HOME}/include)
link_directories(${V8_HOME}/out.gn-${STPYV8_V8_GIT_TAG}/x64.${STPYV8_CONFIG_NAME}.stpyv8/obj)
link_libraries(v8_monolith)

add_library(stpyv8 MODULE ${SOURCES})

# here we mimic output to match what python dist.tools would do
set_target_properties(stpyv8
        PROPERTIES
        PREFIX ""
        OUTPUT_NAME "_STPyV8.cpython-37m-darwin"
        LIBRARY_OUTPUT_DIRECTORY "/Users/darwin/lab/stpyv8xxx/.venv/lib/python3.7/site-packages"
        )

#clang++ -bundle -undefined dynamic_lookup
#-isysroot /Library/Developer/CommandLineTools/SDKs/MacOSX10.15.sdk
#build/temp.macosx-10.15-x86_64-3.7/src/*.o
#-L/Users/darwin/lab/v8_ws/v8/out.gn/x64.debug.sample/obj/
#-L/usr/local/lib
#-L/usr/local/opt/openssl@1.1/lib
#-L/usr/local/opt/sqlite/lib
#-lboost_system
#-lv8_monolith
#-lboost_python37
#-o build/lib.macosx-10.15-x86_64-3.7/_STPyV8.cpython-37m-darwin.so