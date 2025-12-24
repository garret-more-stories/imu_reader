cmake_minimum_required(VERSION 3.10)
project(MyProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE LIB_SOURCES CONFIGURE_DEPENDS source/*.cpp)
file(GLOB_RECURSE LIB_HEADERS CONFIGURE_DEPENDS include/*.hpp)

include(FetchContent)

find_package(SDL3 QUIET)

if(NOT SDL3_FOUND)
    FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG  7f3ae3d57459e59943a4ecfefc8f6277ec6bf540
    )
    
    set(SDL_SHARED ON CACHE BOOL "" FORCE)
    set(SDL_STATIC OFF CACHE BOOL "" FORCE)

    if (MSVC)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL" CACHE STRING "" FORCE)
    endif()

    FetchContent_MakeAvailable(SDL3)
endif()

add_library(ImuReader SHARED
    ${LIB_SOURCES}
    ${LIB_HEADERS}
)

target_include_directories(ImuReader PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)


target_link_libraries(ImuReader PRIVATE SDL3::SDL3)

if(WIN32)
    target_compile_definitions(ImuReader PRIVATE IMU_LIB_EXPORTS)
endif()


add_executable(MyApp source/main.cpp)
target_link_libraries(MyApp PRIVATE ImuReader)

set_target_properties(ImuReader PROPERTIES
    OUTPUT_NAME "imu_reader"
)

if (WIN32)
    set_target_properties(ImuReader PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>"
    )

    set_target_properties(MyApp PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/MyApp"
)

elseif(APPLE)
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

    add_custom_command(
        TARGET MyApp POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy
        $<TARGET_FILE:SDL3::SDL3-shared>
        $<TARGET_FILE:ImuReader>
        $<TARGET_FILE_DIR:MyApp>
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

    add_custom_command(
        TARGET MyApp POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy
        $<TARGET_SONAME_FILE:SDL3::SDL3-shared>
        $<TARGET_FILE:ImuReader>
        $<TARGET_FILE_DIR:MyApp>
        VERBATIM
    )
endif()


