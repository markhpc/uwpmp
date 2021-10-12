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

add_executable(${PROJECT_NAME} src/unwindpmp.cc src/uwpmp_types.cc)
target_include_directories(${PROJECT_NAME} PRIVATE src ${libunwind_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} stdc++ cxxopts nlohmann_json::nlohmann_json fmt)
target_link_libraries(${PROJECT_NAME} ${libunwind_LIBS_DIR}/libunwind-ptrace.a)
target_link_libraries(${PROJECT_NAME} ${libunwind_LIBS_DIR}/libunwind-x86_64.a)
target_link_libraries(${PROJECT_NAME} ${libunwind_LIBS_DIR}/libunwind.a)
add_dependencies(${PROJECT_NAME} libunwind)
