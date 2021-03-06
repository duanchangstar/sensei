cmake_minimum_required(VERSION 3.8)
project(shiftreg_gpio)

#######################
#  Cross Compilation  #
#######################

set(XENOMAI_BASE_DIR "/usr/xenomai" CACHE STRING "xenomai base dir path")

if(NOT "$ENV{CMAKE_SYSROOT}" STREQUAL "")
    set(CMAKE_SYSROOT "$ENV{CMAKE_SYSROOT}")
    message("ENV_CMAKE_SYSROOT = " $ENV{CMAKE_SYSROOT})
endif()
if(NOT "${CMAKE_SYSROOT}" STREQUAL "")
    set(XENOMAI_BASE_DIR "${CMAKE_SYSROOT}/usr/xenomai")
    message("XENOMAI_BASE_DIR is " ${XENOMAI_BASE_DIR})
endif()

#######################
#  Utility functions  #
#######################

# Set Xenomai build options at local scope
function(add_xenomai_to_target target)
    set(XENOMAI_C_FLAGS "-D_GNU_SOURCE -D_REENTRANT -D__COBALT__ -D__COBALT_WRAP__")

    set(XENOMAI_INCLUDE_DIRS ${XENOMAI_BASE_DIR}/include
                             ${XENOMAI_BASE_DIR}/include/cobalt)

    find_library(COBALT_LIB cobalt HINTS ${XENOMAI_BASE_DIR}/lib)
    target_compile_options(${target} PRIVATE ${XENOMAI_C_FLAGS})
    target_include_directories(${target} PRIVATE ${XENOMAI_INCLUDE_DIRS})
    target_link_libraries(${target} PRIVATE ${COBALT_LIB} rt m)
endfunction()

#######################
# Include Directories #
#######################
set(SHIFTREG_GPIO_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/include"
                               "${PROJECT_SOURCE_DIR}/src"
                               "${CMAKE_SOURCE_DIR}/src"
                               "${CMAKE_SOURCE_DIR}/third-party/spdlog/include")

####################
# Lib Source Files #
####################
set(SHIFTREG_GPIO_LIB_SOURCES src/shiftreg_gpio.cpp)

###########
# Targets #
###########
if (${WITH_GPIO_LOGIC})
    # third party fifo library
    add_subdirectory(${CMAKE_SOURCE_DIR}/third-party/fifo [EXCLUDE_FROM_ALL])

    # build gpio protocol library
    set(GPIO_PROTOCOL_BUILD_CLIENT_LIB ON CACHE BOOL "" FORCE)

    add_library(shiftreg_gpio STATIC ${SHIFTREG_GPIO_LIB_SOURCES})
    add_xenomai_to_target(shiftreg_gpio)
    target_include_directories(shiftreg_gpio PRIVATE ${SHIFTREG_GPIO_INCLUDE_DIRS})
    target_link_libraries(shiftreg_gpio PRIVATE pthread)
    target_link_libraries(shiftreg_gpio PUBLIC fifo gpio_protocol gpio_protocol_client)
    target_compile_definitions(shiftreg_gpio PUBLIC -DWITH_GPIO_LOGIC)

else()

    set(GPIO_PROTOCOL_BUILD_CLIENT_LIB OFF CACHE BOOL "" FORCE)

    # just export header to sensei
    add_library(shiftreg_gpio INTERFACE)
    target_include_directories(shiftreg_gpio INTERFACE ${PROJECT_SOURCE_DIR}/include)
endif()