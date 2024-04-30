运行环境：ubuntu20.04 x86架构

    https://github.com/TartanLlama/minidbg/tree/master  mindbg原型
    https://github.com/ocornut/imgui  前端显示ui

1. 安装依赖
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

2. 编译
```
# 编译库 提示没有目标，可以忽略
cd ./ext/libelfbin
make

# 编译程序
cd ../../       # 回到项目根目录
mkdir build
cd build
cmake ..
make

# 保存GUI状态的文件（窗口大小、位置等）
cp imgui.ini build/
```

3. 运行
./mindbg fileYouWantToDbg

fileYouWantToDbg需要以-g选项编译

4. 使用

- 设置断点：在命令输入框，通过输入文件名和行号，或函数名设置断点。
- 运行：单击**next**按钮逐行执行程序代码，支持**step into**、**step over**、**continue**、**si**等操作。
- 变量监视：在窗口中输入变量名，可以实时查看变量值。目前只支持在主函数中监视基本数据类型。
- 查看调用栈：在窗口可以查看当前程序的调用栈信息，直观看到函数调用关系。
- 查看寄存器和内存状态：用户可以查看目标程序的寄存器状态，了解程序运行时的底层信息。
- 点击菜单栏的按钮，可以收起或打开各窗口。
- 点击**File**按钮，选择可执行文件，可以调试新程序。

5. todo
    1. 删除断点
    2. 完成变量监视器，能够监视函数内的局部变量；-> 能够删除变量；




- 集成汇编代码、寄存器、调用栈等多种功能，一次可见，同时比对，不用反复查找翻看；
- 与其他调试器比较（给出数据），内存和cpu占用少，小平台也可以流畅运行；
- 代码全部开放可见，从底层开始实现，可用于教学、可进行功能扩展和定制，如条件断点、远程可视化调试；