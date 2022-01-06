cmake_minimum_required(VERSION 3.14)
project(unwindpmp CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(LibElf REQUIRED)
list(APPEND CMAKE_REQUIRED_INCLUDES ${LIBELF_INCLUDE_DIRS})

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
FetchContent_Declare(
        json
        GIT_REPOSITORY "https://github.com/ArthurSonzogni/nlohmann_json_cmake_fetchcontent"
        GIT_TAG "v3.10.2"
)
FetchContent_GetProperties(json)
if(NOT json_POPULATED)
  FetchContent_Populate(json)
  add_subdirectory(${json_SOURCE_DIR} ${json_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
FetchContent_MakeAvailable(cxxopts fmt json)

ExternalProject_Add(
	libunwind
 GIT_REPOSITORY https://github.com/libunwind/libunwind.git 
	GIT_TAG "v1.5"
	CONFIGURE_COMMAND
	cd <SOURCE_DIR> && autoreconf -i && ./configure --disable-minidebuginfo --disable-shared --enable-static
	BUILD_COMMAND
	cd <SOURCE_DIR> && make -s -j${BUILD_JOBS_NUM}
	INSTALL_COMMAND ""
	BUILD_IN_SOURCE TRUE
)
ExternalProject_Get_Property(libunwind SOURCE_DIR)
set(libunwind_SOURCE_DIR ${SOURCE_DIR})
set(libunwind_LIBS_DIR ${CMAKE_CURRENT_BINARY_DIR}/${libunwind_SOURCE_DIR}/src/.libs)
set(libunwind_INCLUDE_DIR ${libunwind_SOURCE_DIR}/include)

#ExternalProject_Add(
#  elfutils
#  GIT_REPOSITORY git://sourceware.org/git/elfutils.git
#  GIT_TAG "elfutils-0.185"
#  CONFIGURE_COMMAND
#  cd <SOURCE_DIR> && autoreconf -i -f && ./configure --enable-maintainer-mode --disable-debuginfod
#  BUILD_COMMAND
#  cd <SOURCE_DIR> && make -s -j${BUILD_JOBS_NUM}
#  INSTALL_COMMAND ""
#  BUILD_IN_SOURCE TRUE
#)
#ExternalProject_Get_Property(elfutils SOURCE_DIR)
#set(elfutils_SOURCE_DIR ${SOURCE_DIR})
#set(elf_LIBS_DIR ${CMAKE_CURRENT_BINARY_DIR}/${elfutils_SOURCE_DIR}/libelf)
#set(elf_INCLUDE_DIR ${elfutils_SOURCE_DIR}/libelf)
#set(ebl_LIBS_DIR ${CMAKE_CURRENT_BINARY_DIR}/${elfutils_SOURCE_DIR}/libebl)
#set(ebl_INCLUDE_DIR ${elfutils_SOURCE_DIR}/libebl)
#set(dw_LIBS_DIR ${CMAKE_CURRENT_BINARY_DIR}/${elfutils_SOURCE_DIR}/libdw)
#set(dw_INCLUDE_DIR ${elfutils_SOURCE_DIR}/libdw)
#set(dwfl_LIBS_DIR ${CMAKE_CURRENT_BINARY_DIR}/${elfutils_SOURCE_DIR}/libdwfl)
#set(dwfl_INCLUDE_DIR ${elfutils_SOURCE_DIR}/libdwfl)
#target_link_libraries(${dwfl_LIBS_DIR}/libdwfl.a ${elf_LIBS_DIR}/libelf.a)

add_executable(${PROJECT_NAME} src/common.cc
                               src/unwindpmp.cc
                               src/uwpmp_types.cc 
                               src/tracer/uwpmp_tracer.cc
                               src/tracer/unwind_tracer.cc
                               src/tracer/dw_tracer.cc
)
target_include_directories(${PROJECT_NAME} PRIVATE src src/tracer ${libunwind_INCLUDE_DIR})
#${ebl_INCLUDE_DIR} ${elf_INCLUDE_DIR} ${dw_INCLUDE_DIR} ${dwfl_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} stdc++ dw cxxopts nlohmann_json::nlohmann_json fmt)
target_link_libraries(${PROJECT_NAME} ${libunwind_LIBS_DIR}/libunwind-ptrace.a)
target_link_libraries(${PROJECT_NAME} ${libunwind_LIBS_DIR}/libunwind-x86_64.a)
target_link_libraries(${PROJECT_NAME} ${libunwind_LIBS_DIR}/libunwind.a)


#target_link_libraries(${PROJECT_NAME} ${elf_LIBS_DIR}/libelf.a ${dwfl_LIBS_DIR}/libdwfl.a)
#target_link_libraries(${PROJECT_NAME} ${ebl_LIBS_DIR}/libebl.a)
#target_link_libraries(${PROJECT_NAME} ${dwfl_LIBS_DIR}/libdwfl.a)
#target_link_libraries(${PROJECT_NAME} ${dw_LIBS_DIR}/libdw.a)
add_dependencies(${PROJECT_NAME} libunwind)
