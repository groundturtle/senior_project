#include <vector>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <sys/personality.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <string>

#define GL_SILENCE_DEPRECATION // 忽略弃用特性的警告

#include <GLFW/glfw3.h>

#include "debugger.h"
#include "UI.h"

using namespace minidbg;


int main(int argc, char *argv[])
{

    debugger dbg;
    UI ui(dbg);

    if (argc < 2)
    {
        std::cerr << "Program name not specified";
        return -1;
    }

    auto prog = argv[1];

    auto pid = fork();
    if (pid == 0)
    {
        personality(ADDR_NO_RANDOMIZE); // 关闭随机内存地址的分配
        ptrace(PTRACE_TRACEME, 0, nullptr, nullptr);        // PTRACE_TRACEME 将当前进程标记为跟踪目标, 等待父进程发出信号(ptrace)来控制其执行
        execl(prog, prog, nullptr);
    }
    else if (pid >= 1)
    {
        // parent
        std::cout << "start debugging process " << pid << "\n";

        dbg.initDbg(prog, pid);
        dbg.break_execution("main");
        dbg.continue_execution();
        ui.buildWindows();
    }
}
