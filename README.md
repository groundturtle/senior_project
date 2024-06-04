https://github.com/TartanLlama/minidbg/tree/master              mindbg原型
https://github.com/ocornut/imgui                                前端显示ui
https://blog.tartanllama.xyz/writing-a-linux-debugger-setup/    教程

### 1. 环境准备

运行环境：ubuntu20.04 x86架构

```
# glfw3库
sudo apt-get install libglfw3-dev

# OpenGL库
sudo apt-get install libgl1-mesa-dev

# CMake 编译工具
sudo apt-get install cmake

# g++
sudo apt-get install g++

# objdump 
sudo apt-get install binutils

# python3
sudo apt-get install python3

# gtk 窗口打开文件使用
sudo apt-get install libgtk-3-dev
```

### 2. 编译
```
# 编译以来库 提示没有目标，可以忽略
cd ./ext/libelfbin
make

# 编译程序
cd ../../       # 回到项目根目录
mkdir build
cd build
cmake ..
make

# 复制保存GUI状态的文件（窗口大小、位置···
cd ../          # 回到项目根目录
cp imgui.ini build/
```

### 3. 运行

`./mindbg fileYouWantToDbg`

`fileYouWantToDbg` 需要以-g选项编译

### 4. 使用

- 设置断点：在命令输入框，通过输入文件名和行号、地址、函数名三种方式设置断点。
- 运行：单击菜单栏相应的按钮。
- 变量监视：在窗口中输入变量名，点击add。目前只支持主函数中的基本数据类型。
- 寄存器读写：输入命令，形式为 `register write [reg] [val]` 和 `register read [reg]`
- 窗口控制：
    - 点击菜单栏 `View` -> `Element` ，其中显示所有窗口的名称，右侧有 `√` 为已打开，单击即可切换状态。
    - 窗口大小和位置通过鼠标拖动修改，相关状态自动保存在文件 build/Imgui.ini 中。
- 切换被调试程序：点击菜单栏 `file` 按钮，在弹出的窗口中选择可执行文件（注意不是源代码）。

### 5. todo
1. 鼠标点击打断点、删除断点；
2. 将正在运行的代码行居中（目前只有高亮）；
3. 将输出重定向到窗口。


### 项目结构

项目中的主要文件如下所示，其中最重要的是 debugger 类，整合了各部分功能；每个文件中都有详细注释。底层的控制和数据读写操作主要通过系统调用`ptrace()`实现，

```bash
miniGdb
    ├─examples          # 被调试程序示例
    ├─ext               
    │  └─libelfin       # 依赖库
    │      ├─dwarf         # dwarf 信息解析
    │      └─elf           # elf 文件读取
    ├─html              # doxygen 生成的文档
    ├─imgui             # 依赖库：用户界面库
    ├─include           # 项目头文件
    │    asmparaser.h           ## 解析汇编文件，将信息存储于结构体数组中。
    │    breakpoint.h           ## 断点设置和清除，负责修改指令和保存记录状态；一个实例对应一个断点。
    │    ptrace_expr_context.h
    │    registers.h            ## 寄存器类型定义和读写实现。
    │    symboltype.h           ## 符号类型定义和符号查找，暂时没用上。
    │    utility.hpp            ## 通用函数
    │    debugger.h             ## 核心逻辑，负责断点管理、执行控制等，并提供API给UI类调用。
    │    UI.h                   ## 用户界面，包括建立窗口、设置按钮等。
    └─src
        asmparaser.cpp      
        breakpoint.cpp
        debugger.cpp
        main.cpp
        ptrace_expr_context.cpp
        registers.cpp
        symboltype.cpp
        UI.cpp
```

程序入口如下所示，在完成初始化后，就在UI的一个循环中执行直至结束。
```bash
开始 --> 调用fork() --(父进程)--> 初始化debugger --> 子程序入口设置断点并启动子程序 --> 初始化UI类和窗口 --> 循环渲染画面并处理交互事件 --> 结束
            \
              --(子进程)--> 关闭随机地址分配 --> 设置允许被控制 --> exec加载目标程序 --> 根据控制信号运行 --> 结束
```

每次向被调试程序发送一个控制指令后，都要回到`wait_for_signal()`函数中等待信号并进行处理；断点触发的SIGTRAP信号有两种不同情况，`debugger::handle_sigtrap(siginfo_t)`函数用于处理 SIGTRAP 信号，根据 info.si_code 判断具体的触发原因并处理。详见代码和注释。