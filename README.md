系统ubuntu20.04，需要是x86 linux
作者：
    https://github.com/jianKangS1/mindbgWithUI 作者
    https://github.com/TartanLlama/minidbg/tree/master  mindbg原型
    https://github.com/ocornut/imgui  前端显示ui

1、安装依赖
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

2、编译程序
```
make
mkdir build && cd build 
cmake ..
make clean
make
```

3. 运行
./mindbg fileYouWantToDbg

fileYouWantToDbg需要以-g选项编译


|    Name    |   Address   |   Parameters    |   Return Type      |
|------------|-------------|-----------------|--------------------|
|   func(int)|  0x123456   |    int          |       int          |
|func(int, int)|  0x789ABC  |    int, int     |       int          |
|  func(char) |  0xDEADBEEF |    char         |       int          |
