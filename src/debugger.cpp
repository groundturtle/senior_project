#include "debugger.h"
#include "utility.hpp"

template class std::initializer_list<dwarf::taddr>; 

namespace minidbg
{

dwarf::die debugger::get_function_die_from_pc(uint64_t pc) {
    for (const auto &cu : m_dwarf.compilation_units()) {        // 找到编译单元
        for(auto die : cu.root()){                   // 对于其中的每条die
            if (die.tag == dwarf::DW_TAG::subprogram            // 如果是函数，且为当前正在执行的函数
                && get_function_from_pc(pc).function_name == dwarf::at_name(die)) 
            {     
                    std::cout << "function "+dwarf::at_name(die)+"'s die found.\n";
                    return die;
            }
        }
    }
    throw std::out_of_range{"get_function_die_from_pc(): funtion die not found.\n"};
}


std::string debugger::read_variable(const std::string& var_name) {
    //@todo: 
    std::string error_msg;

    try {
        // auto func = get_function_die_from_pc(get_offset_pc());
        auto func_die = get_function_die_from_pc(get_pc());

        for (const auto& die : func_die) {
            // 跳过非变量, 检查变量名是否匹配
            if (die.tag != dwarf::DW_TAG::variable) continue;
            if (!die.has(dwarf::DW_AT::name) || dwarf::at_name(die) != var_name) continue;

            //debug
            // std::cout<<"read_variable(): variable " + var_name + "/" + dwarf::at_name(die) + "'s die" + " found.\n";

            auto loc_val = die[dwarf::DW_AT::location];

            // 只支持exprlocs类型的位置表达式
            if (loc_val.get_type() == dwarf::value::type::exprloc) {
                ptrace_expr_context context(m_pid, m_load_address);
                auto result = loc_val.as_exprloc().evaluate(&context);

                // 根据位置类型读取并返回变量的值
                switch (result.location_type) {
                    case dwarf::expr_result::type::address: {  // 地址
                        auto offset_addr = result.value;
                        long data = ptrace(PTRACE_PEEKDATA, m_pid, reinterpret_cast<void*>(offset_addr), nullptr);
                        if (errno != 0) {
                            error_msg = "Error: Failed to read memory at address " + std::to_string(offset_addr);
                            return error_msg;
                        }
                        return std::to_string(data);
                    }
                    case dwarf::expr_result::type::reg: {  // 寄存器
                        try {
                            auto value = get_register_value_from_dwarf_register(m_pid, result.value);
                            return std::to_string(value);
                        } catch(const std::exception& e) {
                            error_msg = "Error: Failed to read register value, " + std::string(e.what());
                            return error_msg;
                        }
                    }
                    default:
                        return "Error: Unhandled variable location type.";
                }
            } else {
                return "Error: Variable location not supported.";
            }
        }
        return "Error: Variable not found.";
    } catch (const std::exception& e) {
        // 处理所有预期之外的异常，并记录足够的信息来修复bug
        std::stringstream ss;
        ss << "read_variable(): exception: " << e.what();
        ss << "Variable name: " << var_name << ", ";
        ss << "PID: " << m_pid << ", Load Address: " << std::hex << m_load_address << "\n\n";
        // 输出异常信息到标准错误
        std::cerr << ss.str();
        // 返回明确的错误消息
        return "Error: An exception occurred while reading variable.";
    }
}


void debugger::handle_command(const std::string &line)
{
    std::vector<std::string> args = utility::split(line, ' ');
    std::string command = args[0];

    if (utility::is_prefix(command, "break"))
    {
        if (args[1][0] == '0' && args[1][1] == 'x')
        {
            std::string addr{args[1], 2}; // naively assume that the user has written 0xADDRESS like 0xff
            set_breakpoint_at_address(std::stol(addr, 0, 16) + m_load_address);
        }
        else if (args[1].find(':') != std::string::npos)
        {
            auto file_and_line = utility::split(args[1], ':');
            set_breakpoint_at_source_file(file_and_line[0], std::stoi(file_and_line[1]));
        }
        else
        {
            set_breakpoint_at_function(args[1]);
        }
    }
    else if(utility::is_prefix(command, "continue"))
    {
        continue_execution();
    }
    else if (utility::is_prefix(command, "register"))
    {
        if (utility::is_prefix(args[1], "dump"))
        {
            dump_registers();
        }
        else if (utility::is_prefix(args[1], "read"))
        {
            std::cout << get_register_value(m_pid, get_register_from_name(args[2])) << std::endl;
        }
        else if (utility::is_prefix(args[1], "write"))
        {
            std::string val{args[3], 2}; // assume 0xVALUE
            set_register_value(m_pid, get_register_from_name(args[2]), std::stol(val, 0, 16));
            std::cout << "write data " << args[3] << " into reg " << args[2] << " successfully\n";
        }
        else
        {
            std::cout << "unknow command for register\n";
        }
    }
    else if (utility::is_prefix(command, "symbol"))
    {
        auto syms = symboltype::lookup_symbol(args[1], m_elf);
        for (auto &s : syms)
        {
            std::cout << s.name << " " << symboltype::to_string(s.type) << " 0x" << std::hex << s.addr << std::endl;
        }
    }
    else if (utility::is_prefix(command, "memory"))
    {
        std::string addr{args[2], 2}; // assume 0xADDRESS

        if (utility::is_prefix(args[1], "read"))
        {
            std::cout << std::hex << read_memory(std::stol(addr, 0, 16)) << std::endl;
        }
        if (utility::is_prefix(args[1], "write"))
        {
            std::string val{args[3], 2}; // assume 0xVAL
            write_memory(std::stol(addr, 0, 16), std::stol(val, 0, 16));
        }
    }
    else if (utility::is_prefix(command, "si"))
    {
        single_step_instruction_with_breakpoint_check();
        auto offset_pc = offset_load_address(get_pc());
        auto line_entry = get_line_entry_from_pc(offset_pc);
    }
    else if (utility::is_prefix(command, "step"))
    {
        step_in();
    }
    else if (utility::is_prefix(command, "next"))
    {
        step_over();
    }
    else if (utility::is_prefix(command, "finish"))
    {
        step_out();
    }
    else if (utility::is_prefix(command, "backtrace"))
    {
        // print_backtrace();
    }
    else if (utility::is_prefix(command, "ls"))
    {

        auto line_entry = get_line_entry_from_pc(get_offset_pc());
        // print_source(line_entry->file->path, line_entry->line);
        // print_asm(m_asm_name, get_offset_pc());
    }
    else
    {
        std::cerr << "unknow command\n";
    }
}




/**
    * @brief 终止被调试程序
    * 
    */
bool debugger::kill_prog()
{
    if (ptrace(PTRACE_KILL, m_pid, NULL, NULL) == -1) {
        std::cerr << "Failed to ptrace PTRACE_KILL." << std::endl;
        return false;
    }
    return true;
}


std::vector<std::pair<std::string, u_int64_t>> debugger::get_ram_vct()
{
    std::vector<std::pair<std::string, u_int64_t>> m_ram_vct;
    for (const auto &rd : g_register_descriptors)
    {
        m_ram_vct.push_back(std::make_pair(rd.name, get_register_value(m_pid, rd.r)));
    }
    return m_ram_vct;
};

unsigned debugger::get_src_line()
{
    auto line_entry = get_line_entry_from_pc(get_offset_pc());
    return line_entry->line;
}

uint64_t debugger::get_pc()
{
    return get_register_value(m_pid, reg::rip);
};

/**
    * @brief 获取当前基址寄存器（RBP）的值。
    * 
    * @return uint64_t 当前基址寄存器（RBP）的值
    */
uint64_t debugger::get_rbp()
{
    return get_register_value(m_pid, reg::rbp);
}

uint64_t debugger::get_rsp()
{
    return get_register_value(m_pid, reg::rsp);
}

std::vector<std::pair<uint64_t, std::string>> debugger::get_backtrace_vct()
{
    std::vector<std::pair<uint64_t, std::string>> backtrace_vct;

    auto current_func = get_function_from_pc(get_pc());

    // 结束地址为0表示没有找到有效的函数信息，可能已经到达了程序的起始位置
    if (current_func.end_addr == 0)
    {
        return backtrace_vct;
    }

    // 将当前函数的起始地址和函数名加入回溯函数列表
    backtrace_vct.push_back(std::make_pair(current_func.start_addr, current_func.function_name));

    // 获取当前栈帧的帧指针（RBP寄存器的值）
    auto frame_pointer = get_register_value(m_pid, reg::rbp);
    auto return_address = read_memory(frame_pointer + 8);       // 读取返回地址，即上一级函数调用的地址

    // 开始不断循环回溯，直到当前函数为main函数为止   
    while (current_func.function_name != "main")
    {
        current_func = get_function_from_pc(return_address);
        if (current_func.end_addr == 0)         // 检查获取到的函数信息是否有效
        {
            return backtrace_vct;
        }
        backtrace_vct.push_back(std::make_pair(current_func.start_addr, current_func.function_name));

        frame_pointer = read_memory(frame_pointer);         // 获取上一级函数的帧指针         
        return_address = read_memory(frame_pointer + 8);        //上一级函数的返回地址，即再上一级函数的地址        
    }
    return backtrace_vct;
}

std::vector<std::pair<uint64_t, std::vector<uint8_t>>> debugger::get_global_stack_vct(uint64_t start_addr, uint64_t end_addr)
{
    std::vector<std::pair<uint64_t, std::vector<uint8_t>>> global_stack_vct;    // 存储全局堆栈信息的向量，每个元素为一个地址和对应的字节向量        
    
    // 以8字节为单位遍历内存区域，读取每个地址处的数据，将其转换为字节向量存储起来
    for (auto i = start_addr; i < end_addr; i += 8)
    {
        uint64_t temp_data = read_memory(i);        // 读取内存地址处的数据
        std::vector<uint8_t> temp_byte_vct(8);          // 存储当前地址处数据的字节向量

        // 提取uint64_t的每一个字节并存储到字节向量中
        for (auto j = 0; j < 8; ++j)
        {
            temp_byte_vct[j] = static_cast<uint8_t>((temp_data >> (8 * j)) & 0xff);
        }

        // 将当前地址和对应的字节向量存储为一对键值对，并加入全局堆栈信息的向量中
        global_stack_vct.push_back(std::make_pair(i, temp_byte_vct));
    }
    
    return global_stack_vct;
}


debugger::debugger()
{
}

void debugger::initDbg(std::string prog_name, pid_t pid)
{
    // 清理旧的调试状态
    m_breakpoints.clear(); // 清除所有的断点
    m_prog_name = std::move(prog_name);
    m_pid = pid;
    m_asm_name = m_prog_name + ".asm";
    auto fd = open(m_prog_name.c_str(), O_RDONLY);
    m_elf = elf::elf{elf::create_mmap_loader(fd)};
    m_dwarf = dwarf::dwarf{dwarf::elf::create_loader(m_elf)};

    // 等待目标进程发送信号
    wait_for_signal();
    // 初始化加载地址
    initialise_load_address();
    // 运行objdump获取汇编信息，加载源代码和汇编信息
    initialise_run_objdump();
    initialise_load_asm();
    initialise_load_src();

    std::cout << "初始化minidbg成功\n";
}

void debugger::continue_execution()
{
    step_over_breakpoint();
    ptrace(PTRACE_CONT, m_pid, nullptr, nullptr);
    wait_for_signal();
}

void debugger::break_execution(std::string command)
{

    // 0x<hexadecimal> -> address breakpoint
    // <filename>:<line> -> line number breakpoint
    // <anything else> -> function name breakpoint
    if (command[0] == '0' && command[1] == 'x')
    {
        std::string addr{command, 2}; // naively assume that the user has written 0xADDRESS like 0xff
        set_breakpoint_at_address(std::stol(addr, 0, 16) + m_load_address);
    }
    else if (command.find(':') != std::string::npos)
    {
        auto file_and_line = utility::split(command, ':');
        set_breakpoint_at_source_file(file_and_line[0], std::stoi(file_and_line[1]));
    }
    else
    {
        set_breakpoint_at_function(command);
    }
}

void debugger::next_execution()
{
    step_over();
}

void debugger::finish_execution()
{
    step_out();
}

void debugger::step_into_execution()
{
    step_in();
}

void debugger::si_execution()
{
    single_step_instruction_with_breakpoint_check();
}

// todo: figure out
void debugger::handle_sigtrap(siginfo_t info)
{
    switch (info.si_code)
    {
    case SI_KERNEL:
    case TRAP_BRKPT:
    {
        auto nowpc = get_pc();
        set_pc(nowpc - 1);
        nowpc--;
        return;
    }
    case TRAP_TRACE:
        std::cout << "get signal trap_trace" << std::endl;
        return;
    default:
        std::cout << "unknow sigtrap code" << info.si_code << std::endl;
        return;
    }
};

uint64_t debugger::offset_load_address(uint64_t addr)
{
    return addr - m_load_address;
}

uint64_t debugger::offset_dwarf_address(uint64_t addr)
{
    return addr + m_load_address;
}

uint64_t debugger::get_offset_pc()
{
    return offset_load_address(get_pc());
}

uint64_t debugger::read_memory(uint64_t address)
{
    return ptrace(PTRACE_PEEKDATA, m_pid, address, nullptr);
};

void debugger::write_memory(uint64_t address, uint64_t value)
{
    ptrace(PTRACE_POKEDATA, m_pid, address, value);
};

void debugger::set_breakpoint_at_address(std::intptr_t addr)
{
    // std::cout << "set breakpoint at address 0x" << std::hex << addr << std::endl;
    breakpoint bp(m_pid, addr);
    bp.enable();
    m_breakpoints[addr] = bp;
};

void debugger::dump_registers()
{
    for (const auto &rd : g_register_descriptors)
    {                                                           // 最小宽度为 16 个字符
        std::cout << rd.name << "  0x" << std::setfill('0') << std::setw(16) << std::hex << get_register_value(m_pid, rd.r) << std::endl;
    }
};

void debugger::set_pc(uint64_t pc)
{
    set_register_value(m_pid, reg::rip, pc);
};

void debugger::wait_for_signal()
{
    int wait_status;
    auto options = 0;
    // 将状态信息存储到 wait_status 中
    waitpid(m_pid, &wait_status, options);
    auto siginfo = get_signal_info();

    switch (siginfo.si_signo)
    {
    case SIGTRAP:           // 遇到断点
        handle_sigtrap(siginfo);
        break;
    case SIGSEGV:           // 段错误
        std::cout << "sorry, segment fault . reason : " << siginfo.si_code << std::endl;
        break;
    default:
        std::cout << "get signal  " << strsignal(siginfo.si_signo) << std::endl;
        break;
    }
}

void debugger::step_over_breakpoint()
{
    // 判断当前指令地址是否处于断点集合中
    if (m_breakpoints.count(get_pc()))
    {
        auto &bp = m_breakpoints[get_pc()];   // 获取对映射中特定键值的引用
        if (bp.is_enabled())
        {
            bp.disable();       // 禁用当前断点，即将断点位置的0xcc替换为原指令，以允许程序继续执行
            ptrace(PTRACE_SINGLESTEP, m_pid, nullptr, nullptr);     // 使用 ptrace 让目标进程执行一条指令，然后暂停, 方便调试器进行下一步操作。
            wait_for_signal();
            bp.enable();        // 恢复当前断点，确保在下次执行到该断点时，程序会暂停执行
        }
    }
};

asm_head debugger::get_function_from_pc(uint64_t pc)
{
    for (auto &head : m_asm_vct)
    {
        // 该 PC 地址位于当前函数的代码范围内，返回当前函数的信息
        if (pc >= head.start_addr && pc <= head.end_addr)
        {
            return head;
        }
    }
    asm_head temp;
    temp.end_addr = 0;
    return temp;
}

dwarf::line_table::iterator debugger::get_line_entry_from_pc(uint64_t pc)
{
    for (auto &cu : m_dwarf.compilation_units())
    {
        // 判断给定的 PC 地址是否在当前编译单元的调试信息条目范围内
        if (die_pc_range(cu.root()).contains(pc))       // die_pc_range(): 计算给定编译单元的根DIE的地址范围
        {
            auto &lt = cu.get_line_table();
            auto it = lt.find_address(pc);
            if (it == lt.end())
            {
                return lt.begin();
            }
            else
            {
                return it;
            }
        }
    }
    throw std::out_of_range{"can't find line entry"};
}

dwarf::line_table::iterator debugger::get_next_line_entry_from_pc(uint64_t pc)
{
    auto line_entry = get_line_entry_from_pc(pc);
    line_entry++;
    return line_entry;
}

siginfo_t debugger::get_signal_info()
{
    siginfo_t info;
    ptrace(PTRACE_GETSIGINFO, m_pid, nullptr, &info);
    return info;
}

void debugger::single_step_instruction()
{
    ptrace(PTRACE_SINGLESTEP, m_pid, nullptr, nullptr);
    wait_for_signal();
}

void debugger::single_step_instruction_with_breakpoint_check()
{
    if (m_breakpoints.count(get_pc()))
    {
        step_over_breakpoint();
    }
    else
    {
        single_step_instruction();
    }
}

void debugger::remove_breakpoint(std::intptr_t addr)
{
    if (m_breakpoints.at(addr).is_enabled())
    {
        m_breakpoints.at(addr).disable();
    }
    m_breakpoints.erase(addr);
}

void debugger::step_out()
{
    auto frame_pointer = get_register_value(m_pid, reg::rbp);
    auto return_address = read_memory(frame_pointer + 8);

    bool should_remove_breakpoint = false;
    if (!m_breakpoints.count(return_address))
    {
        set_breakpoint_at_address(return_address);
        should_remove_breakpoint = true;
    }

    continue_execution();

    if (should_remove_breakpoint)
    {
        remove_breakpoint(return_address);
    }
}

void debugger::step_in()
{
    auto line = get_line_entry_from_pc(get_offset_pc())->line;      //  line 成员变量
    // 源代码行号没有改变，循环继续
    while (get_line_entry_from_pc(get_offset_pc())->line == line)
    {
        single_step_instruction_with_breakpoint_check();
    }

    auto line_entry = get_line_entry_from_pc(get_offset_pc());
    //print_source(line_entry->file->path, line_entry->line);
}

void debugger::step_over()
{
    auto line_entry = get_next_line_entry_from_pc(get_offset_pc());
    auto newpc = offset_dwarf_address(line_entry->address);
    if (!m_breakpoints.count(newpc))
    {
        set_breakpoint_at_address(newpc);
    }
    continue_execution();

    remove_breakpoint(newpc);
};

void debugger::set_breakpoint_at_function(const std::string &name)
{
    bool flag = false;
    for (const auto &cu : m_dwarf.compilation_units())
    {
        // 检查每个 DIE 是否具有名称属性并且与参数匹配.
        for (const auto &die : cu.root())
        {
            if (die.has(dwarf::DW_AT::name) && at_name(die) == name)
            {
                flag = true;
                auto low_pc = at_low_pc(die);
                auto entry = get_line_entry_from_pc(low_pc);
                ++entry;                // 在源代码行条目的下一行设置断点
                set_breakpoint_at_address(offset_dwarf_address(entry->address));
            }
        }
    }
    if (!flag)
    {
        std::cout << "fails to set breakpoint at function " << name << "\nCan't find it\n";
    }
}

void debugger::set_breakpoint_at_source_file(const std::string &file, unsigned line)
{
    for (const auto &cu : m_dwarf.compilation_units())
    {
        auto rootName = at_name(cu.root());         // 源文件的完整路径名
        size_t pos = rootName.rfind('/');
        if (pos != std::string::npos && pos != rootName.size() - 1)
        {
            rootName = rootName.substr(pos + 1);
        }

        if (file == rootName)
        {
            const auto &lt = cu.get_line_table();
            // 在行表中查找与目标行匹配的行条目                    
            for (const auto &entry : lt)
            {
                if (entry.is_stmt && entry.line == line)
                {
                    set_breakpoint_at_address(offset_dwarf_address(entry.address));
                    std::cout<<"set breakpoint at " + file + ":" + std::to_string(line)<<std::endl;
                    return;
                }
            }
        }
    }
    std::cout << "set breakpoint at function " << file << " and line " << line << " fails\n";
}

/*** @brief 初始化 * */
void debugger::initialise_load_address()
{
    //@todo: 为什么是动态库？
    if (m_elf.get_hdr().type == elf::et::dyn)
    {
        // The load address is found in /proc/&lt;pid&gt;/maps
        std::ifstream map("/proc/" + std::to_string(m_pid) + "/maps");

        std::string addr;
        std::getline(map, addr, '-');
        addr = "0x" + addr;
        m_load_address = std::stoul(addr, 0, 16);       // 将字符串转换为unsigned long
    }
    std::cout<< "PID: " << m_pid << ", Load Address: 0x" << std::hex << m_load_address << "\n";
}

void debugger::initialise_load_asm()
{
    asmparaser paraser;
    m_asm_vct = move(paraser.get_asm_data(m_asm_name));
    // 将汇编数据块的起始地址和结束地址, 条目地址, 都加上加载地址，换为真实地址
    for (auto &head : m_asm_vct)
    {
        head.start_addr += m_load_address;
        head.end_addr += m_load_address;
        for (auto &entry : head.asm_entris)
        {
            entry.addr += m_load_address;
        }
    }
}

void debugger::initialise_run_objdump()
{
    std::string binaryFile = m_prog_name;
    std::string middleFile = m_prog_name + ".asm";

    // 使用 objdump 命令生成反汇编代码
    std::string command = "objdump -d " + binaryFile + "  | tail -n +4 > " + middleFile;
    int result = std::system(command.c_str());

    if (result != 0)
    {
        std::cerr << "error when run command: " + command << std::endl;
    }
}

void debugger::initialise_load_src()
{
    m_src_vct.clear();
    auto offset_pc = offset_load_address(get_pc());
    auto line_entry = m_dwarf.compilation_units().begin()->get_line_table().begin();
    std::string file_path = std::string(line_entry->file->path);

    std::ifstream inFile(file_path);
    std::string line;

    if (!inFile)
    {
        std::cerr << "Failed to open input file " << file_path << std::endl;
        return;
    }

    while (std::getline(inFile, line))
    {
        m_src_vct.push_back(line);
    }

    inFile.close();
    return;
};

};