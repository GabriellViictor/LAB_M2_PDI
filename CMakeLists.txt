# Versao minima do CMake usada pelo projeto-base.
cmake_minimum_required(VERSION 3.24)

# Declara o projeto C++ e sua versao (M1 + M2 consolidados).
project(pdi_lab VERSION 0.3.0 LANGUAGES CXX)

# O projeto utiliza C++20 de forma portavel.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Gera compile_commands.json, util para editores e ferramentas de analise.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Habilita a infraestrutura de testes do CMake por meio do CTest.
include(CTest)

# Centraliza a descoberta do OpenCV (core, imgcodecs, imgproc).
include(cmake/PdiOpenCV.cmake)

# Biblioteca principal pdi_core contendo a infraestrutura e os modulos M1 e M2.
add_library(pdi_core
    src/cli.cpp
    src/contract.cpp
    src/image_io.cpp
    src/kernel.cpp
    src/operations.cpp
    src/parameters.cpp
    src/result_io.cpp
    src/intensity.cpp
    src/representation.cpp
    src/filtering.cpp
    src/segmentation.cpp
    src/morphology.cpp
    src/components.cpp
)

add_library(pdi::core ALIAS pdi_core)

target_include_directories(
    pdi_core
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pdi_core PUBLIC ${OpenCV_LIBS})

if(MSVC)
    target_compile_options(pdi_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(
        pdi_core
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
            -Wconversion
            -Wsign-conversion
    )
endif()

# Executavel principal usado pela interface de linha de comando.
add_executable(pdi_lab src/main.cpp)
target_link_libraries(pdi_lab PRIVATE pdi::core)

# No Windows, copia as DLLs do OpenCV e do MinGW para a pasta de build permitindo
# execucao imediata pelo terminal ou CLion sem necessidade de configurar PATH no sistema.
if(WIN32)
    file(GLOB OPENCV_DLLS "${OpenCV_DIR}/x64/mingw/bin/*.dll")
    if(OPENCV_DLLS)
        file(COPY ${OPENCV_DLLS} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    endif()
    get_filename_component(COMPILER_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
    foreach(DLL_NAME libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll)
        if(EXISTS "${COMPILER_DIR}/${DLL_NAME}")
            file(COPY "${COMPILER_DIR}/${DLL_NAME}" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
        endif()
    endforeach()
endif()

# Testes automatizados (M1 + M2.1 + M2.2).
if(BUILD_TESTING)
    add_executable(pdi_smoke_test tests/smoke_test.cpp)
    target_link_libraries(pdi_smoke_test PRIVATE pdi::core)
    add_test(NAME pdi_smoke_test COMMAND pdi_smoke_test)

    add_executable(pdi_infrastructure_test tests/infrastructure_test.cpp)
    target_link_libraries(pdi_infrastructure_test PRIVATE pdi::core)
    add_test(NAME pdi_infrastructure_test COMMAND pdi_infrastructure_test)

    add_executable(pdi_m2_test tests/m2_test.cpp)
    target_link_libraries(pdi_m2_test PRIVATE pdi::core)
    add_test(NAME pdi_m2_test COMMAND pdi_m2_test)

    # Configura PATH do ambiente de teste para carregar automaticamente as DLLs do OpenCV e do compilador
    get_filename_component(COMPILER_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)

    set_tests_properties(
        pdi_smoke_test
        pdi_infrastructure_test
        pdi_m2_test
        PROPERTIES
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${OpenCV_DIR}/x64/mingw/bin;PATH=path_list_prepend:${COMPILER_DIR}"
    )
endif()
