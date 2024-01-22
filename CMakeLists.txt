cmake_minimum_required(VERSION 3.21)
project(MaaAssistantArknights)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif ()


#将一个路径追加到CMAKE_MODULE_PATH变量中。这通常用于指定CMake在查找模块文件时应该搜索的路径,${CMAKE_CURRENT_LIST_DIR}/是系统预定的变量,表示cmakelist所在的路径
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")


#option命令用于定义一个构建选项，并指定其默认值
#BUILD_TEST 是构建选项的变量名，"build a demo" 是选项的描述，OFF 是默认值。
option(BUILD_TEST "build a demo" OFF)
option(BUILD_XCFRAMEWORK "build xcframework for macOS app" OFF)
option(BUILD_UNIVERSAL "build both arm64 and x86_64 on macOS" OFF)
option(INSTALL_PYTHON "install python ffi" OFF)
option(INSTALL_RESOURCE "install resource" OFF)
option(INSTALL_DEVEL "install development files" OFF)
option(INSTALL_THIRD_LIBS "install third party libraries" ON)
option(USE_MAADEPS "use third-party libraries built by MaaDeps" ON)
option(WITH_THRIFT "build with thrift" OFF)


#${PROJECT_SOURCE_DIR} 是一个CMake预定变量，表示CMake项目的顶级目录（源代码目录）的路径。
#这个变量用于表示CMake项目的顶级目录，也就是源代码目录的路径
include(${PROJECT_SOURCE_DIR}/cmake/utils.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/thrift-gen.cmake)

if(USE_MAADEPS)
    include(${PROJECT_SOURCE_DIR}/MaaDeps/maadeps.cmake)
endif()

if (MSVC)
    add_compile_options("/utf-8")
    add_compile_options("/MP")
    add_compile_options("/W4;/WX")
    add_compile_options("/wd4127")  # conditional expression is constant
    add_compile_options("/Wv:19.35.32217")  # disable warning introduced after this version

    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
else ()
#-Wall: 启用常见的警告信息。
# -Werror: 将所有警告视为错误，中止编译。
# -Wextra: 启用额外的警告。
# -Wpedantic: 严格遵循C++标准。
# -Wno-missing-field-initializers: 禁止关于结构或类初始化时缺少字段的警告。
    add_compile_options("-Wall;-Werror;-Wextra;-Wpedantic;-Wno-missing-field-initializers")


    #这个条件语句检查编译器标识是否为GNU，并且检查编译器版本是否小于13。
#如果条件满足，就会添加一个额外的编译选项 -Wno-restrict。
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 13)
        add_compile_options("-Wno-restrict")
    endif()
endif ()


# 这行代码创建一个名为 header_only_libraries 的头文件库（header-only library）。
# INTERFACE 关键字表示这个库没有编译产物（二进制文件），而是仅包含头文件。这样的库通常用于包含一些头文件，而不需要编译源文件。
# target_include_directories(header_only_libraries INTERFACE 3rdparty/include)：
add_library(header_only_libraries INTERFACE)
# 这行代码用于将目录 3rdparty/include 添加到 header_only_libraries 的头文件搜索路径中。
#target_include_directories 命令用于为目标（这里是 header_only_libraries）设置头文件搜索路径。通过将 INTERFACE 关键字传递给 target_include_directories，这个路径将被添加到库的接口（包括依赖该库的其他目标）中，确保它们也能够找到这个路径。
target_include_directories(header_only_libraries INTERFACE 3rdparty/include)


#这行代码使用 file 命令和 GLOB_RECURSE 选项，查找指定目录（src/MaaCore/）下所有的 .cpp 文件，并将它们的路径保存在变量 maa_src 中。
#GLOB_RECURSE 表示递归地查找匹配指定模式的文件。这通常用于将一个目录中的所有源文件列出，并将它们作为源文件列表添加到一个目标中，例如，一个库或可执行文件。
#如果不要递归查找可以填写GLOB
file(GLOB_RECURSE maa_src src/MaaCore/*.cpp)


# add_library 是用于定义库的CMake命令。
# MaaCore 是库的名称，你可以根据项目的需要自定义。
# SHARED 表示创建一个共享库，也就是动态链接库（DLL）或共享对象（SO）。
# ${maa_src} 包含了之前通过 file(GLOB ...) 查找到的源文件列表。
add_library(MaaCore SHARED ${maa_src})

if (WIN32)
    #注意：相比VS版本缺少了 -D_CONSOLE -D_WINDLL 两项
    target_compile_definitions(MaaCore PRIVATE ASST_DLL_EXPORTS _UNICODE UNICODE)
endif ()

# target_include_directories 命令用于设置目标的头文件搜索路径。
# MaaCore 是目标的名称，即之前通过 add_library 定义的共享库。
# PUBLIC, PRIVATE 是指定头文件搜索路径的关键字。它们用于控制这些路径对于库的用户（使用者）和库的实现（定义者）之间的可见性。
# include 是一个公共的头文件路径，对于库的用户和实现都是可见的。
# src/MaaCore 是一个私有的头文件路径，只对库的实现可见，而对库的用户是不可见的。
target_include_directories(MaaCore PUBLIC include PRIVATE src/MaaCore)
set(MaaCore_PUBLIC_HEADERS include/AsstCaller.h include/AsstPort.h)
# target_sources 命令用于为一个目标（这里是 MaaCore）添加源文件或头文件。
# PUBLIC 关键字表示这些文件是公共的，即它们对于库的用户是可见的。
# ${MaaCore_PUBLIC_HEADERS} 包含了之前定义的公共头文件的路径，这样它们将被添加到 MaaCore 目标中。
target_sources(MaaCore PUBLIC ${MaaCore_PUBLIC_HEADERS})


# set_target_properties 命令用于设置目标的一些属性。
# CXX_STANDARD 20 设置了C++标准为C++20。
# CXX_STANDARD_REQUIRED ON 表示C++标准是必需的。
# PUBLIC_HEADER "${MaaCore_PUBLIC_HEADERS}" 将之前定义的公共头文件指定为库的公共头文件。这对于安装库时将头文件包含到安装位置是有用的。
set_target_properties(MaaCore PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    PUBLIC_HEADER "${MaaCore_PUBLIC_HEADERS}"
)

if(APPLE)
    set_target_properties(MaaCore PROPERTIES INSTALL_RPATH "@loader_path/")
elseif(UNIX)
# INSTALL_RPATH 属性用于指定目标被安装时运行时查找库的路径。在这里，$ORIGIN/ 是一个相对路径，指的是运行时目标的位置。
# 这个设置的含义是告诉系统在运行时在目标的相同目录中查找依赖库。$ORIGIN/ 是一个特殊的变量，代表了目标文件所在的目录。
    set_target_properties(MaaCore PROPERTIES INSTALL_RPATH "$ORIGIN/")//有待测试INSTALL_PATH的用法
endif()

if (BUILD_TEST)
    add_executable(test src/Cpp/main.cpp)
    set_target_properties(test PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
    )
    target_link_libraries(test MaaCore)
endif (BUILD_TEST)

find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs videoio)
find_package(ZLIB REQUIRED)
find_package(MaaDerpLearning REQUIRED)
find_package(asio REQUIRED)
find_package(ONNXRuntime)
find_package(cpr CONFIG REQUIRED)
if(WITH_THRIFT)
    find_package(Thrift CONFIG REQUIRED)
endif(WITH_THRIFT)

target_link_libraries(MaaCore ${OpenCV_LIBS} MaaDerpLearning asio::asio ZLIB::ZLIB ONNXRuntime::ONNXRuntime cpr::cpr header_only_libraries)

if(WITH_THRIFT)
    add_subdirectory(src/MaaThriftController)
    target_link_libraries(MaaCore MaaThriftController)
endif(WITH_THRIFT)

if(WIN32)
    target_link_libraries(MaaCore ws2_32)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
    find_package(range-v3 REQUIRED)
    target_link_libraries(MaaCore range-v3::range-v3)
endif ()

if(INSTALL_DEVEL)
    set(MaaCore_install_extra_args PUBLIC_HEADER DESTINATION devel/include ARCHIVE DESTINATION devel/lib)
endif()

install(TARGETS MaaCore
    RUNTIME DESTINATION .
    LIBRARY DESTINATION .
    PUBLIC_HEADER DESTINATION .
    ${MaaCore_install_extra_args}
)

if(INSTALL_THIRD_LIBS AND USE_MAADEPS)
    maadeps_install()
endif()

if (INSTALL_PYTHON)
    install(DIRECTORY src/Python DESTINATION .)
endif (INSTALL_PYTHON)
if (INSTALL_RESOURCE)
    install(DIRECTORY resource DESTINATION .)
endif (INSTALL_RESOURCE)


if (APPLE)
    include(${PROJECT_SOURCE_DIR}/cmake/macos.cmake)
endif (APPLE)

# define MAA_VERSION from git
if (NOT DEFINED MAA_VERSION)
    find_package(Git)
endif ()
if (NOT DEFINED MAA_VERSION AND GIT_FOUND)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE err
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if (result EQUAL 0)
        set(MAA_VERSION "${output}")
    else ()
        message(WARNING "git rev-parse returning ${result}, output:\n${err}")
    endif ()
endif ()
if (NOT MAA_VERSION)
    set(MAA_VERSION "DEBUG VERSION")
endif ()
message(STATUS "MAA_VERSION=${MAA_VERSION}")
add_compile_definitions(MAA_VERSION="${MAA_VERSION}")
