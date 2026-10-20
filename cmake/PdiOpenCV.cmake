# OpenCV discovery for normal and portable environments.
# Priority:
# 1. -DPDI_OPENCV_DIR=<directory containing OpenCVConfig.cmake>
# 2. environment variable PDI_OPENCV_DIR
# 3. normal CMake discovery (MSYS2/system installation)

set(PDI_OPENCV_DIR "" CACHE PATH "Directory containing OpenCVConfig.cmake")

if(NOT PDI_OPENCV_DIR AND DEFINED ENV{PDI_OPENCV_DIR})
    file(TO_CMAKE_PATH "$ENV{PDI_OPENCV_DIR}" PDI_OPENCV_DIR)
endif()

if(PDI_OPENCV_DIR)
    set(OpenCV_DIR "${PDI_OPENCV_DIR}" CACHE PATH "OpenCV config directory" FORCE)
endif()

find_package(OpenCV QUIET COMPONENTS core imgcodecs imgproc)

if(NOT OpenCV_FOUND)
    message(FATAL_ERROR
        "OpenCV nao foi encontrado.\n"
        "Instale o pacote do ambiente ou informe uma distribuicao portatil:\n"
        "  cmake ... -DPDI_OPENCV_DIR=/caminho/para/lib/cmake/opencv4\n"
        "ou:\n"
        "  export PDI_OPENCV_DIR=/caminho/para/lib/cmake/opencv4\n"
        "O diretorio informado deve conter OpenCVConfig.cmake."
    )
endif()

message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "OpenCV config: ${OpenCV_DIR}")
