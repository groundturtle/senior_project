cmake_minimum_required (VERSION 3.0)
cmake_policy(SET CMP0072 NEW)

project (MiniDbg)

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

add_compile_options(-std=c++14 -g)     # 增加C++14标准支持和调试信息     

# 包含头文件目录
include_directories(${GTK3_INCLUDE_DIRS} ext/libelfin include imgui)

# 链接库目录
link_directories(${GTK3_LIBRARY_DIRS})
add_definitions(${GTK3_CFLAGS_OTHER})

add_executable(minidbg
   src/main.cpp 
   src/debugger.cpp        
   src/asmparaser.cpp
   src/breakpoint.cpp
   src/ptrace_expr_context.cpp
   src/registers.cpp
   src/symboltype.cpp
   src/UI.cpp
   ## add source file here.
   imgui/imgui.cpp
   imgui/imgui_widgets.cpp
   imgui/imgui_tables.cpp
   imgui/imgui_impl_opengl3.cpp
   imgui/imgui_impl_glfw.cpp
   imgui/imgui_draw.cpp
   imgui/imgui_demo.cpp
)


# 示例程序
add_executable(hello examples/hello.cpp)
set_target_properties(hello
                      PROPERTIES COMPILE_FLAGS "-g -O0")

add_executable(variable examples/variable.cpp)
set_target_properties(variable
                      PROPERTIES COMPILE_FLAGS "-gdwarf-2 -O0")

add_executable(unwinding examples/stack_unwinding.cpp)
set_target_properties(unwinding
                      PROPERTIES COMPILE_FLAGS "-g -O0")

# 编译libelfin库
add_custom_target(
   libelfin
   COMMAND make
   WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/ext/libelfin
)

# 链接库
target_link_libraries(minidbg
                      ${GTK3_LIBRARIES}
                      ${PROJECT_SOURCE_DIR}/ext/libelfin/dwarf/libdwarf++.so
                      ${PROJECT_SOURCE_DIR}/ext/libelfin/elf/libelf++.so
                      GL glfw -ldl)

# 确保minidbg在libelfin之后编译
add_dependencies(minidbg libelfin)
