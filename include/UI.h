#ifndef UI_H
#define UI_H

#include <vector>
#include <string>
#include <unordered_map>
#include "debugger.h"
#include <gtk/gtk.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <sys/personality.h>
#include <stdio.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "breakpoint.h"
#include "registers.h"
#include "asmparaser.h"

namespace minidbg {

/**
 * @brief 弹出文件选择对话框，返回选中文件的路径
 * 
 * @return char* 选中文件的路径
 */
char* openFileDialog();

/**
 * @brief GLFW发生错误时的回调函数
 * 
 * @param error 错误码
 * @param description 错误描述
 */
static void glfw_error_callback(int error, const char *description);

class UI {
public:
    /**
     * @brief 构造UI类
     * 
     * @param dbg 调试器实例的引用
     */
    explicit UI(debugger& dbg);

    /**
     * @brief 构建和显示窗口
     * 
     * @return int 状态码
     */
    int buildWindows();

    /**
     * @brief 调用现有函数渲染UI
     * 
     */
    void render();

private:
    debugger& dbg; ///< 调试器实例的引用

    char commandInput[256];        ///< 命令行输入缓冲区
    char newVariableName[256];     ///< 输入变量名缓冲区
    std::unordered_map<std::string, std::string> watchedVariables;  ///< 存储变量名和对应的值

    // UI窗口显示控制
    static bool show_program;
    static bool show_stack;
    static bool show_src;
    static bool show_global_stack;
    static bool show_ram;
    static bool show_option_bar;
    static bool show_call_stack;
    static bool show_command_inputBar;
    static bool show_demo_window;
    static bool show_watcher;
    static int windows_status;

    // 私有成员函数，用于显示不同的窗口和组件
    void showProgram(bool* p_open);
    void showStack(bool* p_open);
    void showSrc(bool* p_open);
    void showGlobalStack(bool* p_open);
    void showRam(bool* p_open);
    void showCallStack(bool* p_open);
    void showOptionBar(bool* p_open);
    void showOptionMainMenuBar();
    void showCommandInputBar();
    void showVariableWatcher();
    void updateWatchedVariables();
};

} // namespace minidbg

#endif // UI_H
