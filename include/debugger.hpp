#ifndef MINIDBG_DEBUGGER_HPP
#define MINIDBG_DEBUGGER_HPP

#include <unordered_map>
#include <utility>
#include <string>
#include <linux/types.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <cstdio>
#include <algorithm>

#include "breakpoint.hpp"
#include "registers.hpp"
#include "dwarf/dwarf++.hh"
#include "elf/elf++.hh"
#include "symboltype.hpp"
#include "asmparaser.hpp"

namespace minidbg
{

/**
 * @brief 判断字符串s是否为字符串of的前缀。
 * 
 * @param s 要检查的前缀字符串
 * @param of 目标字符串
 * @return true 如果s是of的前缀
 * @return false 如果s不是of的前缀
 */
bool is_prefix(const std::string &s, const std::string &of)
{
    // 如果`s`是`of`的前缀
    // 例如：s="con" of="continue" 返回true
    if (s.size() > of.size())
        return false;
    return std::equal(s.begin(), s.end(), of.begin());
}

/**
 * @brief 调试器类，用于跟踪和调试程序执行。
 */
class debugger
{
public:
    // 需要展示的数据
    std::vector<asm_head> m_asm_vct;            /**< 存储汇编信息, 包括起止地址、汇编条目等 */
    std::vector<std::string> m_src_vct;         /**< 存储源代码 */

    /**
     * @brief 获取程序中各个寄存器的值。
     * 
     * @return std::vector<std::pair<std::string, u_int64_t>> 包含寄存器名称和值的向量
     */
    std::vector<std::pair<std::string, u_int64_t>> get_ram_vct()
    {
        std::vector<std::pair<std::string, u_int64_t>> m_ram_vct;
        for (const auto &rd : g_register_descriptors)
        {
            m_ram_vct.push_back(std::make_pair(rd.name, get_register_value(m_pid, rd.r)));
        }
        return m_ram_vct;
    };

    /**
     * @brief 获取当前源代码行号。
     * 
     * @return unsigned 当前源代码行号
     */
    unsigned get_src_line()
    {
        auto line_entry = get_line_entry_from_pc(get_offset_pc());
        return line_entry->line;
    }

    /**
     * @brief 获取当前程序计数器（PC）的值。
     * 
     * @return uint64_t 当前程序计数器（PC）的值
     */
    uint64_t get_pc()
    {
        return get_register_value(m_pid, reg::rip);
    };

    /**
     * @brief 获取当前基址寄存器（RBP）的值。
     * 
     * @return uint64_t 当前基址寄存器（RBP）的值
     */
    uint64_t get_rbp()
    {
        return get_register_value(m_pid, reg::rbp);
    }

    /**
     * @brief 获取当前栈指针寄存器（RSP）的值。
     * 
     * @return uint64_t 当前栈指针寄存器（RSP）的值
     */
    uint64_t get_rsp()
    {
        return get_register_value(m_pid, reg::rsp);
    }

    /**
     * @brief 获取当前线程的函数调用链回溯信息息。
     * 
     * @return std::vector<std::pair<uint64_t, std::string>> 包含调用堆栈信息的向量，每个元素为函数起始地址和函数名的键值对
     */
    std::vector<std::pair<uint64_t, std::string>> get_backtrace_vct()
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

    /**
    * @brief 获取全局堆栈信息。
    * 
    * @details 从指定的起始地址开始，以8字节为单位遍历内存区域，读取每个地址处的数据，将其转换为字节向量存储起来。
    * 最后返回包含这些信息的向量，每个元素是一个地址和对应的字节向量。
    * 
    * @param start_addr 起始地址（包含）
    * @param end_addr 终止地址（不包含）
    * @return std::vector<std::pair<uint64_t, std::vector<uint8_t>>> 包含全局堆栈信息的向量，每个元素为一个地址和对应的字节向量
    */
    std::vector<std::pair<uint64_t, std::vector<uint8_t>>> get_global_stack_vct(uint64_t start_addr, uint64_t end_addr)
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


        debugger()
        {
        }

        /**
         * @brief 初始化mini调试器。
         * 
         * @param prog_name 目标程序的名称。
         * @param pid 目标程序的进程ID。
        */
        void initDbg(std::string prog_name, pid_t pid)
        {
            m_prog_name = std::move(prog_name);
            m_pid = pid;
            m_asm_name = m_prog_name + ".asm";              // 汇编文件名
            auto fd = open(m_prog_name.c_str(), O_RDONLY);      // 打开目标程序文件
            m_elf = elf::elf{elf::create_mmap_loader(fd)};          // 使用elfio库加载 ELF 格式的目标文件
            m_dwarf = dwarf::dwarf{dwarf::elf::create_loader(m_elf)};   // dwarfdump  库加载 DWARF 调试信息

            wait_for_signal();                  // 等待目标进程发送信号
            initialise_load_address();              // 初始化加载目标程序的内存地址信息

            initialise_run_objdump();           // 运行objdump工具，获取目标程序的汇编信息
            initialise_load_asm();

            initialise_load_src();

            std::cout << "init minidbg successfully\n";
        }

        /**
         * @brief step_over()_breakpoint()跳过当前断点，然后ptrace_continue，让子进程继续执行
         * 
         */
        void continue_execution()
        {
            step_over_breakpoint();
            ptrace(PTRACE_CONT, m_pid, nullptr, nullptr);
            wait_for_signal();
        }

        /**
        * @brief 根据命令设置断点
        * 
        * @details 
        * 支持以下三种格式的断点设置：
        * 1. 以十六进制地址开头的格式，如 `0xADDRESS`。
        * 2. 文件名和行号的格式，如 `<filename>:<line>`。
        * 3. 函数名的格式，如 `<function_name>`。
        * 
        * @param command 用户输入的设置断点的命令
        * 
        * @note 
        * 目前版本仅在 'main' 函数中使用。
        */
        void break_execution(std::string command)
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
                auto file_and_line = split(command, ':');
                set_breakpoint_at_source_file(file_and_line[0], std::stoi(file_and_line[1]));
            }
            else
            {
                set_breakpoint_at_function(command);
            }
        }

        void next_execution()
        {
            step_over();
        }

        void finish_execution()
        {
            step_out();
        }

        void step_into_execution()
        {
            step_in();
        }

        void si_execution()
        {
            single_step_instruction_with_breakpoint_check();
        }

    private:
        std::string m_prog_name;
        std::string m_asm_name;
        pid_t m_pid;
        std::unordered_map<std::intptr_t, breakpoint> m_breakpoints;
        dwarf::dwarf m_dwarf;
        elf::elf m_elf;
        uint64_t m_load_address; // 偏移量，很重要

        /**
        * @brief 处理用户输入的调试器命令，并执行相应操作
        * 
        * @param line 用户输入的命令字符串
        * 
        * @details 根据命令的前缀进行分类处理，具体逻辑如下：
        * 
        * - 如果命令以 "break" 开头，则处理设置断点的逻辑。
        * - 如果命令以 "register" 开头，则根据子命令执行相关的寄存器操作，包括查看寄存器内容、修改寄存器值等。
        * - 如果命令以 "symbol" 开头，则查找并打印符号信息。
        * - 如果命令以 "memory" 开头，则根据子命令执行内存读写操作。
        * - 如果命令以 "si" 开头，则执行单步指令，并检查是否有断点。
        * - 如果命令以 "step" 开头，则执行单步进入操作。
        * - 如果命令以 "next" 开头，则执行下一步操作。
        * - 如果命令以 "finish" 开头，则执行执行到函数返回操作。
        * - 如果命令以 "backtrace" 开头，则打印回溯信息。
        * - 如果命令以 "ls" 开头，则打印源代码和汇编信息。
        * - 其他情况下，输出错误信息。
        */
        void handle_command(const std::string &line)
        {
            std::vector<std::string> args = split(line, ' ');
            std::string command = args[0];

            if (is_prefix(command, "break"))
            {
                if (args[1][0] == '0' && args[1][1] == 'x')
                {
                    std::string addr{args[1], 2}; // naively assume that the user has written 0xADDRESS like 0xff
                    set_breakpoint_at_address(std::stol(addr, 0, 16) + m_load_address);
                }
                else if (args[1].find(':') != std::string::npos)
                {
                    auto file_and_line = split(args[1], ':');
                    set_breakpoint_at_source_file(file_and_line[0], std::stoi(file_and_line[1]));
                }
                else
                {
                    set_breakpoint_at_function(args[1]);
                }
            }
            else if (is_prefix(command, "register"))
            {
                if (is_prefix(args[1], "dump"))
                {
                    dump_registers();
                }
                else if (is_prefix(args[1], "read"))
                {
                    std::cout << get_register_value(m_pid, get_register_from_name(args[2])) << std::endl;
                }
                else if (is_prefix(args[1], "write"))
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
            else if (is_prefix(command, "symbol"))
            {
                auto syms = lookup_symbol(args[1]);
                for (auto &s : syms)
                {
                    std::cout << s.name << " " << to_string(s.type) << " 0x" << std::hex << s.addr << std::endl;
                }
            }
            else if (is_prefix(command, "memory"))
            {
                std::string addr{args[2], 2}; // assume 0xADDRESS

                if (is_prefix(args[1], "read"))
                {
                    std::cout << std::hex << read_memory(std::stol(addr, 0, 16)) << std::endl;
                }
                if (is_prefix(args[1], "write"))
                {
                    std::string val{args[3], 2}; // assume 0xVAL
                    write_memory(std::stol(addr, 0, 16), std::stol(val, 0, 16));
                }
            }
            else if (is_prefix(command, "si"))
            {
                single_step_instruction_with_breakpoint_check();
                auto offset_pc = offset_load_address(get_pc());
                auto line_entry = get_line_entry_from_pc(offset_pc);
            //    print_source(line_entry->file->path, line_entry->line);      // debug
            }
            else if (is_prefix(command, "step"))
            {
                step_in();
            }
            else if (is_prefix(command, "next"))
            {
                step_over();
            }
            else if (is_prefix(command, "finish"))
            {
                step_out();
            }
            else if (is_prefix(command, "backtrace"))
            {
                // print_backtrace();
            }
            else if (is_prefix(command, "ls"))
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
         * @brief 根据siginfo_t结构体中的si_code字段来判断SIGTRAP信号的具体类型，并进行相应的处理：     

            如果si_code为SI_KERNEL或TRAP_BRKPT，表示到了断点处，获取当前指令地址（nowpc）然后将指令地址减1，程序停留在断点处的前一条指令上。    
                接着，将当前指令地址转换为加载地址（偏移量），并通过该地址获取对应的源代码行信息。最后，可以根据需要打印源代码行信息或进行其他操作。
            如果si_code为TRAP_TRACE，表示程序进入了跟踪状态，通常不需要额外处理，因此只是简单地打印一条信息。
            如果si_code不是上述两种情况，则打印一条未知SIGTRAP代码的消息。
         * 
         * @param info 
         */
        /**
        * @brief 根据 SIGTRAP 信号信息执行不同的操作，包括触发断点、打印调试信息等。
        * 
        * @details
        * 该函数根据接收到的信号信息执行不同的操作：
        * - 如果信号代码为SI_KERNEL或TRAP_BRKPT，则表示触发了断点，将当前程序计数器减一，并打印断点信息。
        * - 如果信号代码为TRAP_TRACE，则表示接收到了追踪信号。
        * - 对于其他信号代码，打印出未知的信号代码信息。
        * 
        * @param info 信号信息
        */
        void handle_sigtrap(siginfo_t info)
        {
            switch (info.si_code)
            {
            case SI_KERNEL:
            case TRAP_BRKPT:
            {
                auto nowpc = get_pc();
                set_pc(nowpc - 1);
                nowpc--;
                std::cout << "hit breakpoint at address 0x" << std::hex << nowpc << std::endl;
                // auto offset_pc = offset_load_address(nowpc);
                // auto line_entry = get_line_entry_from_pc(offset_pc);
                // print_source(line_entry->file->path, line_entry->line);
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

        /**
         * @brief 从实际地址转换为相对地址
         * 
         */
        uint64_t offset_load_address(uint64_t addr)
        {
            return addr - m_load_address;
        }

        /**
         * @brief 将相对地址转换为实际地址
         * 
         */
        uint64_t offset_dwarf_address(uint64_t addr)
        {
            return addr + m_load_address;
        }

        uint64_t get_offset_pc()
        {
            return offset_load_address(get_pc());
        }

        uint64_t read_memory(uint64_t address)
        {
            return ptrace(PTRACE_PEEKDATA, m_pid, address, nullptr);
        };

        void write_memory(uint64_t address, uint64_t value)
        {
            ptrace(PTRACE_POKEDATA, m_pid, address, value);
        };

        void set_breakpoint_at_address(std::intptr_t addr)
        {
            std::cout << "Set breakpoint at address 0x" << std::hex << addr << std::endl;
            breakpoint bp(m_pid, addr);
            bp.enable();
            m_breakpoints[addr] = bp;
        };

        /**
         * @brief 
         * 打印目标进程的寄存器信息:  遍历寄存器描述符数组，通过 get_register_value 函数获取每个寄存器的值，并打印出来
         */
        void dump_registers()
        {
            for (const auto &rd : g_register_descriptors)
            {                                                           // 最小宽度为 16 个字符
                std::cout << rd.name << "  0x" << std::setfill('0') << std::setw(16) << std::hex << get_register_value(m_pid, rd.r) << std::endl;
            }
        };

        void set_pc(uint64_t pc)
        {
            set_register_value(m_pid, reg::rip, pc);
        };

        /**
         * @brief 等待目标进程发送信号并做出相应处理.
         * 
         * @details
         * - 如果收到 SIGTRAP 信号，通常表示进程遇到了断点，调用 handle_sigtrap 函数处理。
         * - 如果收到 SIGSEGV 信号，表示进程发生了段错误，输出相应信息。
         */
        void wait_for_signal()
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

        /**
         * @brief 跳过当前断点（执行一条指令），若当前指令没有断点，则不做任何事。 
         * 
         * @details 禁用当前断点, 使用 ptrace_singlestep, 恢复当前断点, 确保在下次执行到该断点时，程序会暂停执行
         */
        void step_over_breakpoint()
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

        /**
         * @brief 确定当前指令所属的函数
         * 
         */
        asm_head get_function_from_pc(uint64_t pc)
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

        /**
         * @brief Get the line entry from pc object
         * 
         * @return 返回源代码行的迭代器，如找不到源码，将抛出异常。常见于程序执行完毕、进入系统调用和库函数。
         */
        dwarf::line_table::iterator get_line_entry_from_pc(uint64_t pc)
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

        /**
        * @brief 根据程序计数器获取下一行的DWARF调试信息。
        * 
        * 根据给定的程序计数器（PC），获取下一行的DWARF调试信息，并返回迭代器指向该行。
        * 
        * @param pc 程序计数器
        * @return dwarf::line_table::iterator 指向下一行的迭代器
        * 
        * @details
        * 该函数通过调用get_line_entry_from_pc函数获取当前PC对应的行的调试信息，
        * 然后通过递增迭代器实现获取下一行的调试信息，最后返回指向下一行的迭代器。
        */
        dwarf::line_table::iterator get_next_line_entry_from_pc(uint64_t pc)
        {
            auto line_entry = get_line_entry_from_pc(pc);
            line_entry++;
            return line_entry;
        }

        siginfo_t get_signal_info()
        {
            siginfo_t info;
            ptrace(PTRACE_GETSIGINFO, m_pid, nullptr, &info);
            return info;
        }

        /**
         * @brief 向子进程发送信号，让子进程只执行一条指令
         * 不进行断点检查，直接认为没有断点。
         * 
         */
        void single_step_instruction()
        {
            ptrace(PTRACE_SINGLESTEP, m_pid, nullptr, nullptr);
            wait_for_signal();
        }

        /**
         * @brief 让子进程执行一条指令。根据当前指令是否有断点，分别调用step_over_breakpoint()和single_step()
         */
        void single_step_instruction_with_breakpoint_check()
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

        /**
         * @brief 移除地址addr上的断点。
         * 
         */
        void remove_breakpoint(std::intptr_t addr)
        {
            if (m_breakpoints.at(addr).is_enabled())
            {
                m_breakpoints.at(addr).disable();
            }
            m_breakpoints.erase(addr);
        }

        /**
         * @brief 跳出函数：从rbp获取返回地址，检查是否有断点，无则设置，然后continue；如刚设置了断点，则去除，如原有则不动。
         * 
         */
        void step_out()
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

        /**
         * @brief 单步进入/进入到下一个源代码行: 循环执行单条指令，源代码行号发生变化，循环结束
         * 
         */
        void step_in()
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

        /**
         * @brief 检查下一行源码是否设置了断点，如没有则设置，然后continue，再删除；如有则直接continue.
         * 
         */
        void step_over()
        {
            // std::cout << "now pc is 0x" << std::hex << get_pc() << "\n";
            auto line_entry = get_next_line_entry_from_pc(get_offset_pc());
            auto newpc = offset_dwarf_address(line_entry->address);
            // std::cout << "my new pc is 0x" << std::hex << get_pc() << "\n";
            if (!m_breakpoints.count(newpc))
            {
                set_breakpoint_at_address(newpc);
            }
            continue_execution();

            remove_breakpoint(newpc);
            // std::cout << "after continue 0x" << std::hex << get_pc() << "\n";
        };

        /**
         * @brief Set the breakpoint at function object
         * 
         */
        void set_breakpoint_at_function(const std::string &name)
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
                        // 在源代码行条目的下一行设置断点. 函数定义通常位于函数体的开头，如果停在函数定义的位置，调试器会在函数被调用前就停止执行，无法观察到函数内部的变量状态、参数传递等信息。
                        ++entry;
                        set_breakpoint_at_address(offset_dwarf_address(entry->address));
                    }
                }
            }
            if (!flag)
            {
                std::cout << "fails to set breakpoint at function " << name << "\nCan't find it\n";
            }
        }

        void set_breakpoint_at_source_file(const std::string &file, unsigned line)
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
                            return;
                        }
                    }
                }
            }
            std::cout << "set breakpoint at function " << file << " and line " << line << " fails\n";
        }

        /**
        * @brief 查找符号, handle_command() 中使用.
        * 
        * 根据给定的符号名称，查找ELF文件中对应的符号，并返回包含所有匹配符号的向量。
        * 
        * @details
        * 该函数遍历ELF文件的所有节，检查类型为符号表（symtab）或动态符号表（dynsym）的节。
        * 对于每个符号，如果名称与给定的名称匹配，则将符号生成 symbol 结构体添加到结果向量中。
        * 最后，使用std::unique函数去除重复的符号，并返回结果向量。
        * 
        * @param name 符号名称
        * @return std::vector<symboltype::symbol> 包含所有匹配符号的向量
        */
        std::vector<symboltype::symbol> lookup_symbol(const std::string &name)
        {
            std::vector<symboltype::symbol> syms;

            // 遍历ELF文件的所有节            
            for (auto &sec : m_elf.sections())
            {
                // 跳过非符号表和动态符号表类型的节
                if (sec.get_hdr().type != elf::sht::symtab && sec.get_hdr().type != elf::sht::dynsym)
                    continue;

                for (auto sym : sec.as_symtab())
                {
                    if (sym.get_name() == name)
                    {
                        auto &d = sym.get_data();
                        syms.push_back(symboltype::symbol{symboltype::to_symbol_type(d.type()), sym.get_name(), d.value});
                    }
                }
            }
            // 去重
            std::vector<symboltype::symbol>::iterator unique_end = std::unique(syms.begin(), syms.end());
            syms.erase(unique_end, syms.end());
            return syms;
        }



/**
 * @brief 初始化 
 * 
 */
        /**
         * @brief 获取程序的偏移量
         * @details
         * 首先，通过m_elf.get_hdr().type获取目标程序的 ELF 文件类型。
         * 如果是动态链接库（et::dyn），则需要通过其他方式获取加载地址。
         * /proc/<pid>/maps是一个特殊的 Linux 文件，用于列出进程的内存映射。
         * 从maps文件中读取第一行，这一行包含了动态库的内存映射范围，形如<start_addr>-<end_addr>。
         * 
         */
        void initialise_load_address()
        {
            //  动态库
            if (m_elf.get_hdr().type == elf::et::dyn)
            {
                // The load address is found in /proc/&lt;pid&gt;/maps
                std::ifstream map("/proc/" + std::to_string(m_pid) + "/maps");

                std::string addr;
                std::getline(map, addr, '-');
                addr = "0x" + addr;
                m_load_address = std::stoul(addr, 0, 16);       // 将字符串转换为unsigned long
            }
        }

        /**
         * @brief 加载汇编数据到m_asm_vct向量中
         * 
         */
        void initialise_load_asm()
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

        /**
         * @brief 使用 objdump 命令生成反汇编代码
         * 
         */
        void initialise_run_objdump()
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

        /**
         * @brief 逐行读取源码到m_src_vct向量中
         * 
         */
        void initialise_load_src()
        {
            auto offset_pc = offset_load_address(get_pc());
            auto line_entry = m_dwarf.compilation_units().begin()->get_line_table().begin();
            std::string file_path = std::string(line_entry->file->path);
            // std::cout<<"trying to open src "<<file_path<<"\n";

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

}

#endif
