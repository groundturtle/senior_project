#include "UI.h"

namespace minidbg 
{

char* openFileDialog() {
    GtkWidget *dialog;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
    gint res;

    gtk_init(NULL, NULL);

    dialog = gtk_file_chooser_dialog_new("Open File",
                                         NULL,
                                         action,
                                         "_Cancel",
                                         GTK_RESPONSE_CANCEL,
                                         "_Open",
                                         GTK_RESPONSE_ACCEPT,
                                         NULL);

    res = gtk_dialog_run(GTK_DIALOG(dialog));
    char* filename = NULL;
    if (res == GTK_RESPONSE_ACCEPT) {
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
        filename = gtk_file_chooser_get_filename(chooser);
    }

    gtk_widget_destroy(dialog);
    while (g_main_context_iteration(NULL, FALSE));
    return filename;
}

/**
 * @brief glf发生错误时的回调函数
 * 
 * @param error 
 * @param description 
 */
static void glfw_error_callback(int error, const char *description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

UI::UI(debugger& dbg) : dbg(dbg) {
    commandInput[256] = "";        // 命令行输入缓冲区
    newVariableName[256] = "";     // 输入变量名缓冲区
}


// Initialization of static members
bool UI::show_program = true;
bool UI::show_stack = true;
bool UI::show_src = true;
bool UI::show_global_stack = true;
bool UI::show_ram = true;
bool UI::show_option_bar = true;
bool UI::show_call_stack = true;
bool UI::show_command_inputBar = true;
bool UI::show_watcher = true;
bool UI::show_demo_window = false;
int UI::windows_status = (ImGuiWindowFlags_None);


void UI::render()
{
    if (show_program) {
        showProgram(&show_program);
    }
    if (show_stack) {
        showStack(&show_stack);
    }
    if (show_src) {
        showSrc(&show_src);
    }
    if (show_global_stack) {
        showGlobalStack(&show_global_stack);
    }
    if (show_ram) {
        showRam(&show_ram);
    }
    if (show_option_bar) {
        showOptionBar(&show_option_bar);
    }
    if (show_call_stack) {
        showCallStack(&show_call_stack);        
    }
    if (show_demo_window) {
        ImGui::ShowDemoWindow(&show_demo_window);
    }

// 新增窗口
    if (show_command_inputBar){
        showCommandInputBar();
    }
    if (show_watcher){
        showVariableWatcher();
    }
}

void UI::showCommandInputBar()
{
    ImGui::Begin("Command Input");      // 创建一个新的ImGui窗口
    ImGui::SetWindowFontScale(1.5f); // 设置字体放大比例为1.5
    {
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
        ImGui::BeginChild("Command Input", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), false, window_flags);

        ImGui::InputText("Command", commandInput, IM_ARRAYSIZE(commandInput));      // 创建一个文本输入框
        if (ImGui::Button("Submit")) {          // 创建一个提交按钮
            std::string command = commandInput;    
            dbg.handle_command(command);     
            // memset(commandInput, 0, sizeof(commandInput));
            commandInput[0] = '\0';         // 清空命令
        }

        ImGui::EndChild();
    }
    ImGui::End();
}


/**
 * @brief 变量监视窗口，输入变量名、点击Add即可进行监视。
 * 
 */
void UI::showVariableWatcher() {
    ImGui::Begin("Variable Watcher");
    ImGui::SetWindowFontScale(1.5f); // 设置字体放大比例为1.5
    {
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
        ImGui::BeginChild("Variable Watcher Data", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), false, window_flags);

        ImGui::InputText("Variable Name", newVariableName, IM_ARRAYSIZE(newVariableName));
        if (ImGui::Button("Add")) {
            watchedVariables[std::string(newVariableName)] = "Pending...";       // 初始值
            newVariableName[0] = '\0';          // 清空输入框
        }
        updateWatchedVariables(); // 更新变量的值
        for (auto& var : watchedVariables) {
            // std::cout<<var.first<<"  "<<var.second<<std::endl;
            ImGui::Text("%s: %s", var.first.c_str(), var.second.c_str());
        }

        ImGui::EndChild();
    }
    ImGui::End();
}

/**
 * @brief 调用debugger类的方法，刷新被监视的变量值
 * 
 */
void UI::updateWatchedVariables() {
    for (auto& var : watchedVariables) {
        var.second = dbg.read_variable(var.first);
    }
}


/**
 * @brief 在 ImGui 窗口中显示源代码。
 * 
 * @param p_open 指向一个布尔值，指示窗口是否应打开
 */
void UI::showProgram(bool *p_open)
{
    ImGui::Begin("Program", p_open, windows_status);

    ImGui::SetWindowFontScale(1.5f);
    {
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
        ImGui::BeginChild("Program data", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), false, window_flags);
        int i = 0;
        // 获取当前程序计数器所在源代码行
        auto program_line = dbg.get_src_line();
        for (auto &src : dbg.m_src_vct)
        {
            if (i + 1 != program_line)
            {
                ImGui::Text("%d\t%s", ++i, src.c_str());
            }
            else
            {
                char buf[128];
                sprintf(buf, "%d\t%s", ++i, src.c_str());
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));     // 设置按钮文本对齐方式
                ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));     // 设置按钮文本对齐方式
                ImGui::Button(buf, ImVec2(-FLT_MIN, 0.0f));                             // // 创建一个按钮
                ImGui::PopStyleVar();       // 恢复
                ImGui::PopStyleColor();     // 恢复
            }
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

/**
 * @brief 在 ImGui 窗口中显示堆栈信息。显示程序计数器rip、基址指针rbp和栈指针rsp的值。
 * 
 * @details rbp 是栈帧指针，用于标识当前栈帧的起始位置
 * 
 * @param p_open 指向一个布尔值，指示窗口是否打开
 */
void UI::showStack(bool *p_open)
{
    ImGui::Begin("Stack", p_open, windows_status);
    ImGui::SetWindowFontScale(1.5f);

    {
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
        ImGui::BeginChild("Stack info", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), false, window_flags);

        ImGui::Text("rip\t\t%lx", dbg.get_pc());
        ImGui::Text("rbp\t\t%lx", dbg.get_rbp());
        ImGui::Text("rsp\t\t%lx", dbg.get_rsp());

        ImGui::EndChild();
    }
    ImGui::End();
}

/**
 * @brief 显示汇编代码的ImGui
 * 
 * @param p_open 
 */
void UI::showSrc(bool *p_open)
{
    ImGui::Begin("Src", p_open, windows_status);
    ImGui::SetWindowFontScale(1.5f);

    {
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
        ImGui::BeginChild("Src data", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), false, window_flags);

        auto asm_addr = dbg.get_pc();

        for (auto &head : dbg.m_asm_vct)
        {
            // 显示该函数的起始地址和函数名
            ImGui::TextColored(ImVec4(0, 0, 1, 1), "0x%lx\t%s", head.start_addr, head.function_name.c_str());
            for (auto &line : head.asm_entris)
            {
                if (line.addr != asm_addr)
                {
                    ImGui::Text("  0x%lx\t%s", line.addr, line.asm_code.c_str());
                }
                else        // 如果当前汇编指令的地址与程序计数器地址匹配，则将该指令突出显示
                {
                    char buf[128];
                    sprintf(buf, "  0x%lx\t%s", line.addr, line.asm_code.c_str());
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));
                    ImGui::Button(buf, ImVec2(-FLT_MIN, 0.0f));
                    ImGui::PopStyleVar();
                    ImGui::PopStyleColor();
                }
            }
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

/**
 * @brief 显示全局堆栈信息
 * 
 * @param p_open 
 */
void UI::showGlobalStack(bool *p_open)
{
    ImGui::Begin("Global Satck", p_open, windows_status);
    ImGui::SetWindowFontScale(1.5f);

    {
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;       // 水平滚动条
        ImGui::BeginChild("Global Stack info", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), false, window_flags);

        // 获取全局堆栈信息
        auto rsp = dbg.get_rsp();
        auto rbp = dbg.get_rbp();
        std::vector<std::pair<uint64_t, std::vector<uint8_t>>> global_stack_vct = std::move(dbg.get_global_stack_vct(rsp - 512, rbp + 512));

        static ImGuiTableFlags flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_Hideable;
        
        // 开始绘制表格. 一个地址对应一个字长，64位，即16位十六进制数
        if (ImGui::BeginTable("global stack table", 9, flags))
        {
            ImGui::TableSetupColumn("address", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("+0", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("+1", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("+2", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("+3", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("+4", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("+5", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("+6", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("+7", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableHeadersRow();

            // 遍历全局堆栈信息并绘制每一行. 
            // row.first: address,  row.second: data
            for (auto &row : global_stack_vct)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                if (row.first != rsp && row.first != rbp)
                {
                    ImGui::Text("%lx", row.first);
                }
                else        // 如果是栈顶或基址指针，则以按钮的形式显示地址值
                {
                    char buf[128];
                    sprintf(buf, "%lx", row.first);
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));
                    ImGui::Button(buf, ImVec2(-FLT_MIN, 0.0f));
                    ImGui::PopStyleVar();
                    ImGui::PopStyleColor();
                }

                // 填充每行的数据
                u_int32_t column = 0;
                for (auto &col : row.second)
                {
                    ImGui::TableSetColumnIndex(++column);
                    ImGui::Text("%02x", col);
                }
            }

            ImGui::EndTable();
        }

        ImGui::EndChild();
    }
    ImGui::End();
}


/**
 * @brief 显示内存中的数据
 * 
 * @param p_open 
 */
void UI::showRam(bool *p_open)
{
    ImGui::Begin("Ram", p_open, windows_status);
    ImGui::SetWindowFontScale(1.5f);

    {
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;

        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);

        ImGui::BeginChild("Ram data", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), false, window_flags);

        ImVec4 bgColor(139.0f / 255.0f, 0, 0, 1);
        ImGui::PushStyleColor(ImGuiCol_Text, bgColor);
        std::vector<std::pair<std::string, u_int64_t>> vct = std::move(dbg.get_ram_vct());

        for (auto p : vct)
        {
            ImGui::Text("%s\t\t0x%lx", p.first.c_str(), p.second);
        }
        ImGui::PopStyleColor();

        ImGui::EndChild();
        ImGui::PopStyleVar();
    }
    ImGui::End();
}



/**
 * @brief 显示全局堆栈信息的ImGui窗口
 * 
 * @param p_open 
 */
void UI::showCallStack(bool *p_open)
{
    ImGui::Begin("Call Stack", p_open, windows_status);

    std::vector<std::pair<uint64_t, std::string>> call_stack_vct = move(dbg.get_backtrace_vct());

    ImGui::SetWindowFontScale(1.5f);
    {
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
        ImGui::BeginChild("Src data", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), false, window_flags);

        int index = 0;
        for (auto p : call_stack_vct)
        {
            ImGui::Text("f#%d:0x%lx\t%s", ++index, p.first, p.second.c_str());
        }

        ImGui::EndChild();
    }
    ImGui::End();
}

/**
 * @brief 显示主菜单栏，包括 "File"、"View" 和 "Run" 三个菜单。
 *        这些菜单提供了加载程序、切换视图元素以及运行调试器操作的选项。
 * 
 * @todo complete 'File -> Load Program'
 */
void UI::showOptionMainMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        ImGui::SetWindowFontScale(1.5f);
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Load Program"))
            {
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    if (ImGui::BeginMainMenuBar())
    {
        ImGui::SetWindowFontScale(1.5f);
        if (ImGui::BeginMenu("View"))
        {
            if (ImGui::MenuItem("Format"))
            {
                ImGui::SetWindowFontScale(1.5f);
                if (ImGui::MenuItem("Dec"))
                {
                }
                if (ImGui::MenuItem("Hex"))
                {
                }
                if (ImGui::MenuItem("Bin"))
                {
                }
            }
            if (ImGui::BeginMenu("Elements"))
            {
                ImGui::SetWindowFontScale(1.5f);
                if (ImGui::MenuItem("Program", NULL, show_program))
                {
                    show_program = !show_program;
                }
                if (ImGui::MenuItem("Stack", NULL, show_stack))
                {
                    show_stack = !show_stack;
                }
                if (ImGui::MenuItem("Global Stack", NULL, show_global_stack))
                {
                    show_global_stack = !show_global_stack;
                }
                if (ImGui::MenuItem("Call Stack", NULL, show_call_stack))
                {
                    show_call_stack = !show_call_stack;
                }
                if (ImGui::MenuItem("Src", NULL, show_src))
                {
                    show_src = !show_src;
                }
                if (ImGui::MenuItem("Ram", NULL, show_ram))
                {
                    show_ram = !show_ram;
                }

                if (ImGui::MenuItem("Demo Table ", NULL, show_demo_window))
                {
                    show_demo_window = !show_demo_window;
                }

                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Layout"))
            {
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    if (ImGui::BeginMainMenuBar())
    {
        ImGui::SetWindowFontScale(1.5f);
        if (ImGui::BeginMenu("Run"))
        {
            ImGui::SetWindowFontScale(1.5f);
            if (ImGui::MenuItem("Stepi"))
            {
                dbg.si_execution();
            }
            if (ImGui::MenuItem("Next"))
            {
                dbg.next_execution();
            }
            if (ImGui::MenuItem("Continue"))
            {
                dbg.continue_execution();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}


/**
 * @brief 显示选项栏窗口，包含多个按钮用于执行调试器操作。
 *
 * @details 此函数创建了一个名为 "Option Bar" 的选项栏窗口，其中包含多个按钮，用于调用相应函数执行调试器操作。
 *          - "file": 用于执行文件相关操作。
 *          - "start": 用于启动调试器。
 *          - "next": 用于执行下一条指令。
 *          - "si": 用于单步执行。
 *          - "step in": 用于进入函数调用。
 *          - "finish": 用于跳出当前函数调用。
 *          - "continue": 用于继续执行程序。
 * 
 * @param p_open 控制窗口是否可见的指针。
 */
void UI::showOptionBar(bool *p_open)
{
    ImGui::Begin("Option Bar", p_open, windows_status | ImGuiWindowFlags_NoTitleBar);
    ImGui::SetWindowFontScale(1.5f);
    showOptionMainMenuBar();

    if (ImGui::BeginTable("split", 10, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings))
    {
        ImGui::TableNextColumn();
        //@todo: 结束旧的被调试程序，并修改所有有关变量（包括但不限于pid、程序路径）。
        if (ImGui::Button("file", ImVec2(-FLT_MIN, -FLT_MIN)))
        {
            std::string filePath = openFileDialog();
            if (!filePath.empty()) {
                //@todo: close old program
                // close_window();
                if(!dbg.kill_prog()){
                    std::cerr<<"Error: kill child program failed\n";
                }
                pid_t pid = fork();     // 新建进程
                if (pid == -1) {
                    std::cerr << "Error: fork() failed\n";
                    exit(EXIT_FAILURE);
                }
                if (pid == 0) {     //子进程: 设置被调试程序
                    personality(ADDR_NO_RANDOMIZE);
                    ptrace(PTRACE_TRACEME, 0, nullptr, nullptr);
                    execl(filePath.c_str(), filePath.c_str(), nullptr);
                } 
                else if (pid > 0) {
                    // 父进程: 重新初始化调试器并在main函数设置断点
                    std::cout << "start debugging process " << pid << "\n";
                    dbg.initDbg(filePath, pid);
                    dbg.break_execution("main");
                    dbg.continue_execution();
                }
            }
        };
        ImGui::TableNextColumn();
        if (ImGui::Button("start", ImVec2(-FLT_MIN, -FLT_MIN)))
        {
        };
        ImGui::TableNextColumn();
        if (ImGui::Button("next", ImVec2(-FLT_MIN, -FLT_MIN)))
        {
            dbg.next_execution();
        };
        ImGui::TableNextColumn();
        if (ImGui::Button("si", ImVec2(-FLT_MIN, -FLT_MIN)))
        {
            dbg.si_execution();
        };
        ImGui::TableNextColumn();
        if (ImGui::Button("step in", ImVec2(-FLT_MIN, -FLT_MIN)))
        {
            dbg.step_into_execution();
        };
        ImGui::TableNextColumn();
        if (ImGui::Button("finish", ImVec2(-FLT_MIN, -FLT_MIN)))
        {
            dbg.finish_execution();
        };
        ImGui::TableNextColumn();
        if (ImGui::Button("continue", ImVec2(-FLT_MIN, -FLT_MIN)))
        {
            dbg.continue_execution();
        };

        ImGui::EndTable();
    }

    ImGui::End();
}

int UI::buildWindows()
{
    glfwSetErrorCallback(glfw_error_callback);      // 设置 GLFW 的错误回调函数
    if (!glfwInit())         // 初始化 GLFW
        return 1;

    // 设置 OpenGL 版本和相关参数
    const char *glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // 创建窗口及上下文
    GLFWwindow *window = glfwCreateWindow(1680, 896, "minidbg", nullptr, nullptr);
    if (window == nullptr)
        return 1;

    // 使新创建的窗口的OpenGL上下文成为当前线程的主上下文
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);     //启用垂直同步（V-Sync），限制帧率与显示器的刷新率同步

    IMGUI_CHECKVERSION();

    //创建ImGui上下文。每个ImGui程序都必须精确地调用一次
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();       // 获取ImGui的IO对象，用于后续配置
    (void)io;                           // 消除编译器产生的未使用变量的警告
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

    // 设置ImGui的颜色主题为浅色
    ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    bool show_another_window = false;
    bool show_my_table = true;

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    while (!glfwWindowShouldClose(window))
    {
        // 每一帧调用一次，处理窗口事件
        glfwPollEvents();

        // 启动 Dear ImGui 帧
        ImGui_ImplOpenGL3_NewFrame();       // 准备绘制新的一帧
        ImGui_ImplGlfw_NewFrame();          // 检查 GLFW 的输入事件
        ImGui::NewFrame();                  // 构建新的界面布局

        // 显示窗口和内容
        render();

        // 渲染
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}


} // namespace minidbg
