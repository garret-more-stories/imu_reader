cmake_minimum_required(VERSION 3.10)
project(MyProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE LIB_SOURCES CONFIGURE_DEPENDS source/*.cpp)
file(GLOB_RECURSE LIB_HEADERS CONFIGURE_DEPENDS include/*.hpp)

find_package(SDL3 REQUIRED)


add_library(ImuReader SHARED
    ${LIB_SOURCES}
    ${LIB_HEADERS}
)

target_include_directories(ImuReader PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)


target_link_libraries(ImuReader PRIVATE SDL3::SDL3)

if(WIN32)
    target_compile_definitions(ImuReader PRIVATE MYLIB_EXPORTS)
endif()


add_executable(MyApp source/main.cpp)
target_link_libraries(MyApp PRIVATE ImuReader)

set_target_properties(ImuReader PROPERTIES
    OUTPUT_NAME "imu_reader"
)

if(APPLE)
    set_target_properties(ImuReader PROPERTIES
        BUILD_RPATH "@loader_path"
        INSTALL_RPATH "@loader_path"
    )
elseif(UNIX)
    set_target_properties(ImuReader PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN"
    )
endif()

if(WIN32)
    add_custom_command(
        TARGET ImuReader POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy
        $<TARGET_FILE:SDL3::SDL3-shared>
        $<TARGET_FILE_DIR:ImuReader>
        VERBATIM
    )
else()
    add_custom_command(
        TARGET ImuReader POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy
        $<TARGET_SONAME_FILE:SDL3::SDL3-shared>
        $<TARGET_FILE_DIR:ImuReader>
        VERBATIM
    )
endif()


