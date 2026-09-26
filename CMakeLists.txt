# 1. 设置项目名称和所需的 CMake 版本
cmake_minimum_required(VERSION 3.10)
project(MointorTranslate C CXX)

# 2. 设置输出目录
# 将所有生成的文件（DLL, EXE）放在 build/bin 目录下
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 自动查找 windres 工具（MinGW 必备）
if(MINGW)
    set(CMAKE_RC_COMPILER_INIT windres)
    enable_language(RC)
    set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <FLAGS> <DEFINES> -i <SOURCE> -o <OBJECT>")
    set(RESOURCE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
    set_source_files_properties(src/resources.rc PROPERTIES 
        COMPILE_FLAGS "-I${RESOURCE_INCLUDE_DIR}"
    )
endif()

# --- 目标 1: 创建动态链接库 (DLL) ---
# 3. 添加 DLL 目标
add_library(MointorTranslate SHARED
    src/MointorTranslate.cpp
    src/OptionsDialog.c
    src/resources.rc
    src/CommModule.cpp
)
target_link_libraries(MointorTranslate PRIVATE user32 gdi32 comctl32 ws2_32)

# 4. 配置 DLL 导出
# 定义宏 MYLIBRARY_EXPORTS，告诉编译器现在我们正在编译 DLL 本身，
# 所以需要使用 __declspec(dllexport)。
target_compile_definitions(MointorTranslate PUBLIC MYLIBRARY_EXPORTS)

# 5. 设置头文件路径
# 确保其他目标可以找到 MointorTranslate.h
target_include_directories(MointorTranslate PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# --- 目标 2: 创建测试程序 (EXE) ---
# 6. 添加可执行文件目标
add_executable(MyApp main.c)

# 7. 链接 DLL 到可执行文件
# 告诉 MyApp 需要链接到 MointorTranslate
target_link_libraries(MyApp PRIVATE MointorTranslate)

# 8. 设置可执行文件使用的头文件路径
target_include_directories(MyApp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)