cmake_minimum_required(VERSION 3.30)
project(vulkan)

set(CMAKE_CXX_STANDARD 17)

find_package(Vulkan REQUIRED)
find_package(PkgConfig REQUIRED)

# GLFW
pkg_check_modules(GLFW REQUIRED glfw3)

add_executable(vulkan_app main.cpp
        System.h
        System.cpp
        Input.h
        Input.cpp
        Graphics.h
        Graphics.cpp
        Vulkan.h
        Vulkan.cpp
        Global.h
        ErrorDialog.h
        ErrorDialog.cpp
        Font.h
        Font.cpp
        Texture.h
        Texture.cpp)


# include
target_include_directories(vulkan_app PRIVATE
        ${GLFW_INCLUDE_DIRS}
)

# link dirs
target_link_directories(vulkan_app PRIVATE
        ${GLFW_LIBRARY_DIRS}
)

# link libs
target_link_libraries(vulkan_app PRIVATE
        Vulkan::Vulkan
        ${GLFW_LIBRARIES}
        ktx
)

# compile flags
target_compile_options(vulkan_app PRIVATE)


