cmake_minimum_required(VERSION 3.22)
project(GoodRAW VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

#----------------------------------
# Dependencies
#----------------------------------

# Qt
find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGL OpenGLWidgets)

# OpenGL
find_package(OpenGL REQUIRED)

# OpenColorIO
find_package(OpenColorIO REQUIRED)

# Imath
find_package(Imath REQUIRED)

# OpenMP for parallel color transforms
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    message(STATUS "OpenMP found - enabling parallel color transforms")
else()
    message(STATUS "OpenMP not found - using serial color transforms")
endif()

# LibRaw
if(WIN32 AND DEFINED CMAKE_TOOLCHAIN_FILE)
    # Windows: vcpkg imported target
    find_package(LibRaw CONFIG REQUIRED)
else()
    # Linux or system install
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBRAW REQUIRED libraw)
endif()

#----------------------------------
# Executable
#----------------------------------

add_executable(${PROJECT_NAME}
    include/mainwindow.h 
    include/glimagewidget.h
    include/halfimage.h
    include/colortransform.h
    include/rawdecoder.h 
    include/glsl_shaders.h 
    src/main.cpp
    src/mainwindow.cpp
    src/glimagewidget.cpp
    src/rawdecoder.cpp
    src/ocio_transform.cpp
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

if(WIN32 AND DEFINED CMAKE_TOOLCHAIN_FILE)
    # Windows: vcpkg handles includes automatically
else()
    # Linux: add system libraw includes
    target_include_directories(${PROJECT_NAME} PRIVATE ${LIBRAW_INCLUDE_DIRS})
endif()

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    OpenGL::GL
    OpenColorIO::OpenColorIO
    Imath::Imath
)

# Link OpenMP if available
if(OpenMP_CXX_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenMP::OpenMP_CXX)
endif()

if(WIN32 AND DEFINED CMAKE_TOOLCHAIN_FILE)
    target_link_libraries(${PROJECT_NAME} PRIVATE libraw::raw)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBRAW_LIBRARIES})
endif()

#----------------------------------
#----------------------------------
# Installation (optional)
#----------------------------------
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
