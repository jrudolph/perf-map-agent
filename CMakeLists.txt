cmake_minimum_required (VERSION 2.8.6)
project (perf-map-agent)

# uncomment to make a debug build (including source positions and symbols)
# set(CMAKE_BUILD_TYPE DEBUG)

# Binaries (agent libperfmap.so and attach-main.jar) will end up in ./out
set(OUTDIR ${PROJECT_BINARY_DIR}/out)
set(LIBRARY_OUTPUT_PATH ${OUTDIR})

find_package(JNI)
if (JNI_FOUND)
    message (STATUS "JNI_INCLUDE_DIRS=${JNI_INCLUDE_DIRS}")
    message (STATUS "JNI_LIBRARIES=${JNI_LIBRARIES}")
    message (STATUS "JAVA_INCLUDE_PATH=${JAVA_INCLUDE_PATH}")
    message (STATUS "JAVA_INCLUDE_PATH2=${JAVA_INCLUDE_PATH2}")
endif()
INCLUDE_DIRECTORIES(${JAVA_INCLUDE_PATH})
INCLUDE_DIRECTORIES(${JAVA_INCLUDE_PATH2})

add_library(perfmap SHARED src/c/perf-map-agent.c src/c/perf-map-file.c)

find_package(Java COMPONENTS Development)
if (Java_Development_FOUND)
    message("Java development tools found.")
else()
    message(FATAL_ERROR "Java development tools not found.")
endif()
include(UseJava)

set(CMAKE_JAVA_INCLUDE_PATH ${JAVA_INCLUDE_PATH}/../lib/tools.jar)
set(CMAKE_JAVA_TARGET_OUTPUT_DIR ${OUTDIR})

#message(STATUS "LIBS: ${Java_JAR_EXECUTABLE} ${Java_INCLUDE_DIRS} incl: ${CMAKE_JAVA_INCLUDE_PATH} output_dir: ${CMAKE_JAVA_TARGET_OUTPUT_DIR}")
add_jar(attach-main src/java/AttachOnce.java ENTRY_POINT net/virtualvoid/perf/AttachOnce OUTPUT_DIR ${OUTDIR})
