CMAKE_MINIMUM_REQUIRED(VERSION 3.4.0)
PROJECT(common)

OPTION(ENABLE_TEST "ENable Utest" ON)

#默认使用c++ 11 标准
set(CMAKE_C_FLAGS_DEBUG     "-Os -ggdb -fno-exceptions -fvisibility=hidden")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fno-rtti -std=c++11")

# for zip in linux
ADD_COMPILE_OPTIONS(-DZIP_STD)

SET(INC_PATHS
    ${PROJECT_SOURCE_DIR}/
    ${PROJECT_SOURCE_DIR}/zip/
    ${PROJECT_SOURCE_DIR}/src/
    ${PROJECT_SOURCE_DIR}/include/
    ${PROJECT_SOURCE_DIR}/encrypt/
)

SET(SRC_FILES
    ${PROJECT_SOURCE_DIR}/include/singleton.hpp
    ${PROJECT_SOURCE_DIR}/src/utils.cpp
    ${PROJECT_SOURCE_DIR}/src/log_file.cpp
    ${PROJECT_SOURCE_DIR}/zip/zip.cpp
    ${PROJECT_SOURCE_DIR}/zip/unzip.cpp
    ${PROJECT_SOURCE_DIR}/encrypt/blowfish.cpp
    ${PROJECT_SOURCE_DIR}/encrypt/xor.cpp
)

ADD_LIBRARY(common STATIC ${SRC_FILES})
TARGET_LINK_LIBRARIES(common PUBLIC pthread)
TARGET_INCLUDE_DIRECTORIES(common PUBLIC ${INC_PATHS})


SET(TEST_FILES
    ${PROJECT_SOURCE_DIR}/../test/main.cpp
)

if(ENABLE_TEST)
    ADD_EXECUTABLE(test ${TEST_FILES} ${SRC_FILES} )
    TARGET_LINK_LIBRARIES(test PUBLIC common)
endif()