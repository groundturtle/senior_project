cmake_minimum_required (VERSION 3.0)
cmake_policy(SET CMP0072 NEW)

project (MiniDbg)

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

add_compile_options(-std=c++14 -g)

include_directories(${GTK3_INCLUDE_DIRS} ext/libelfin include imgui)
link_directories(${GTK3_LIBRARY_DIRS})
add_definitions(${GTK3_CFLAGS_OTHER})

add_executable(minidbg
   src/minidbg.cpp 
   imgui/imgui.cpp
   imgui/imgui_widgets.cpp
   imgui/imgui_tables.cpp
   imgui/imgui_impl_opengl3.cpp
   imgui/imgui_impl_glfw.cpp
   imgui/imgui_draw.cpp
   imgui/imgui_demo.cpp
)

add_executable(hello examples/hello.cpp)
set_target_properties(hello
                      PROPERTIES COMPILE_FLAGS "-g -O0")

add_executable(variable examples/variable.cpp)
set_target_properties(variable
                      PROPERTIES COMPILE_FLAGS "-gdwarf-2 -O0")

add_executable(unwinding examples/stack_unwinding.cpp)
set_target_properties(unwinding
                      PROPERTIES COMPILE_FLAGS "-g -O0")


add_custom_target(
   libelfin
   COMMAND make
   WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/ext/libelfin
)
target_link_libraries(minidbg
                      ${GTK3_LIBRARIES}
                      ${PROJECT_SOURCE_DIR}/ext/libelfin/dwarf/libdwarf++.so
                      ${PROJECT_SOURCE_DIR}/ext/libelfin/elf/libelf++.so
                      GL glfw -ldl)
add_dependencies(minidbg libelfin)
