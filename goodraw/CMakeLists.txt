cmake_minimum_required(VERSION 3.22)
project(GoodRAW)

set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Custom OpenColorIO installation:
list(APPEND CMAKE_PREFIX_PATH "/opt/ocio/")

find_package(OpenColorIO REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGL OpenGLWidgets)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBRAW REQUIRED libraw)

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
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${LIBRAW_INCLUDE_DIRS}
    ${OCIO_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include
    /usr/include/Imath
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets
    ${LIBRAW_LIBRARIES}
    ${OCIO_LIBRARIES}
    OpenGL::GL
    Imath
)
