系统ubuntu20.04，需要是x86 linux
    https://github.com/TartanLlama/minidbg/tree/master  mindbg原型
    https://github.com/ocornut/imgui  前端显示ui

1. 安装依赖
```
# glfw3库
sudo apt-get install libglfw3-dev
# OpenGL库
sudo apt-get install libgl1-mesa-dev
# CMake
sudo apt-get install cmake
# g++
sudo apt-get install g++
# objdump
sudo apt-get install binutils
# python3
sudo apt-get install python3
# gtk
sudo apt-get install libgtk-3-dev
```

2. 编译程序
```
```

3. 运行
./mindbg fileYouWantToDbg

fileYouWantToDbg需要以-g选项编译

4. todo
    1. 把标准输出全部导向一个新增的窗口；
    2. 完成变量监视器，能够监视函数内的局部变量；-> 能够删除变量；
    3. 去除菜单栏上没用的按钮；
    4. 整理debugger类，把声明与实现分离；对应修改CMakeLists.txt
