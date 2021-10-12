cmake_minimum_required(VERSION 3.14)
project(unwindpmp CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(ExternalProject)
include(FetchContent)
FetchContent_Declare(
        cxxopts 
        GIT_REPOSITORY "https://github.com/jarro2783/cxxopts"
        GIT_TAG "v2.2.1"
)
FetchContent_Declare(
        fmt
        GIT_REPOSITORY "https://github.com/fmtlib/fmt"
        GIT_TAG "8.0.1"
)
#FetchContent_Declare(
#        libunwind 
#        GIT_REPOSITORY "https://github.com/libunwind/libunwind"
#        GIT_TAG "v1.5"
#)
FetchContent_Declare(
        json
        GIT_REPOSITORY "https://github.com/ArthurSonzogni/nlohmann_json_cmake_fetchcontent"
        GIT_TAG "v3.10.2"
)
#FetchContent_GetProperties(libunwind)
#if(NOT libunwind_POPULATED)
#  FetchContent_Populate(libunwind)
#  add_subdirectory(${libunwind_SOURCE_DIR} ${libunwind_BINARY_DIR} EXCLUDE_FROM_ALL)
#endif()
FetchContent_GetProperties(json)
if(NOT json_POPULATED)
  FetchContent_Populate(json)
  add_subdirectory(${json_SOURCE_DIR} ${json_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
FetchContent_MakeAvailable(cxxopts fmt json)


set(name libunwind)
set(source_dir ${CMAKE_CURRENT_BINARY_DIR}/${name}/source)
ExternalProject_Add(
    ${name}
    URL https://github.com/libunwind/libunwind/releases/download/v1.5/libunwind-1.5.0.tar.gz
#    URL_HASH MD5=06ba9e60d92fd6f55cd9dadb084df19e
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/${name}
    TMP_DIR ${BUILD_INFO_DIR}
    STAMP_DIR ${BUILD_INFO_DIR}
    DOWNLOAD_DIR ${DOWNLOAD_DIR}
    SOURCE_DIR ${source_dir}
    CONFIGURE_COMMAND
        ${common_configure_envs}
        ./configure ${common_configure_args}
                    --disable-minidebuginfo
                    --disable-shared --enable-static
    BUILD_COMMAND make -s -j${BUILDING_JOBS_NUM}
    BUILD_IN_SOURCE 1
    INSTALL_COMMAND make -s install -j${BUILDING_JOBS_NUM}
    LOG_CONFIGURE TRUE
    LOG_BUILD TRUE
    LOG_INSTALL TRUE
)

ExternalProject_Add_Step(${name} clean
    EXCLUDE_FROM_MAIN TRUE
    ALWAYS TRUE
    DEPENDEES configure
    COMMAND make clean -j
    COMMAND rm -f ${BUILD_INFO_DIR}/${name}-build
    WORKING_DIRECTORY ${source_dir}
)

ExternalProject_Add_StepTargets(${name} clean)

add_executable(${PROJECT_NAME} src/unwindpmp.cc src/uwpmp_types.cc)
target_include_directories(${PROJECT_NAME} PRIVATE src )
#target_link_libraries(${PROJECT_NAME} cxxopts nlohmann_json::nlohmann_json fmt unwind-ptrace unwind-generic stdc++)
target_link_libraries(${PROJECT_NAME} cxxopts nlohmann_json::nlohmann_json fmt unwind-ptrace unwind-generic unwind stdc++)
