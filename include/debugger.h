/**
 * @file debugger.h
 * @brief 调试器核心逻辑实现。调试器及UI启动和初始化、被调试程序控制流管理、程序数据读取等均在debugger类中实现。
 * @version 0.1
 * @date 2024-04-22
 */
#ifndef MINIDBG_DEBUGGER_H
#define MINIDBG_DEBUGGER_H

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
#include <iterator>

#include "breakpoint.h"
#include "registers.h"
#include "dwarf/dwarf++.hh"
#include "elf/elf++.hh"
// #include "dwarf/expr.cc"
#include "symboltype.h"
#include "asmparaser.h"
#include "utility.hpp"
#include "ptrace_expr_context.h"


namespace minidbg
{

/**
 * @brief 调试器类，用于跟踪和调试程序执行。
 */
class debugger
{
public:
    // 需要展示的数据
    std::vector<asm_head> m_asm_vct;            /**< 存储汇编信息, 包括起止地址、汇编条目等 */
    std::vector<std::string> m_src_vct;         /**< 存储源代码 */

public:
    debugger();
    ~debugger() = default;

public:

    /**
     * @brief 从绝对 pc 获取当前函数die
     * 
     * @param pc 
     * @return dwarf::die 
     */
    dwarf::die get_function_die_from_pc(uint64_t pc);


    /**
     * @brief 读取并打印当前函数中所有变量的值。
     * 
     * @details 遍历当前函数的所有变量DIE，评估它们的位置表达式（如果存在），然后根据表达式的结果读取并打印变量的值。
    */
    std::string read_variable(const std::string& var_name);


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
    void handle_command(const std::string &line);

    /**
     * @brief 终止被调试程序
     * 
     */
    bool kill_prog();

    /**
     * @brief 获取程序中各个寄存器的值。
     * 
     * @return std::vector<std::pair<std::string, u_int64_t>> 包含寄存器名称和值的向量
     */
    std::vector<std::pair<std::string, u_int64_t>> get_ram_vct();

    /**
     * @brief 获取当前源代码行号。
     * 
     * @return unsigned 当前源代码行号
     */
    unsigned get_src_line();

    /**
     * @brief 获取当前程序计数器（PC）的值。
     * 
     * @return uint64_t 当前程序计数器（PC）的值
     */
    uint64_t get_pc();

    /**
     * @brief 获取当前基址寄存器（RBP）的值。
     * 
     * @return uint64_t 当前基址寄存器（RBP）的值
     */
    uint64_t get_rbp();

    /**
     * @brief 获取当前栈指针寄存器（RSP）的值。
     * 
     * @return uint64_t 当前栈指针寄存器（RSP）的值
     */
    uint64_t get_rsp();

    /**
     * @brief 获取当前线程的函数调用链回溯信息息。
     * 
     * @return std::vector<std::pair<uint64_t, std::string>> 包含调用堆栈信息的向量，每个元素为函数起始地址和函数名的键值对
     */
    std::vector<std::pair<uint64_t, std::string>> get_backtrace_vct();

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
    std::vector<std::pair<uint64_t, std::vector<uint8_t>>> get_global_stack_vct(uint64_t start_addr, uint64_t end_addr);


    /**
     * @brief 初始化mini调试器。
     * 
     * @param prog_name 目标程序的名称。
     * @param pid 目标程序的进程ID。
    */
    void initDbg(std::string prog_name, pid_t pid);

    /**
     * @brief step_over()_breakpoint()跳过当前断点，然后ptrace_continue，让子进程继续执行
     * 
    */
    void continue_execution();

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
    void break_execution(std::string command);

    /**
     * @brief 执行下一行源代码
     * 
     */
    void next_execution();

    /**
     * @brief 跳出当前函数
     * 
     */
    void finish_execution();

    /**
     * @brief 单步执行，进入函数内部
     * 
     */
    void step_into_execution();

    /**
     * @brief 执行一条汇编指令
     * 
     */
    void si_execution();

private:
    std::string m_prog_name;
    std::string m_asm_name;
    pid_t m_pid;
    std::unordered_map<std::intptr_t, minidbg::breakpoint> m_breakpoints;
    dwarf::dwarf m_dwarf;
    elf::elf m_elf;
    uint64_t m_load_address; // 偏移量，很重要

    /**
     * @brief 根据 SIGTRAP 信号信息执行不同的操作，包括触发断点、打印调试信息等。
     * 
     * @details
     * 根据 info.si_code 判断具体的触发原因：
     * - TRAP_BRKPT：断点触发了 SIGTRAP。此时代码将程序计数器（PC）后退一位，确保继续执行时从断点前的地址开始。
     * - TRAP_TRACE：单步跟踪触发了 SIGTRAP。在这种情况下，函数仅打印一条信息，表明接收到 SIGTRAP。
     * - 对于其他信号代码，打印出未知的信号代码信息。
     * 
     * @param info 信号信息
    */
    void handle_sigtrap(siginfo_t info);

    /**
     * @brief 从实际地址转换为相对地址
     * 
    */
    uint64_t offset_load_address(uint64_t addr);

    /**
     * @brief 将相对地址转换为实际地址
     * 
    */
    uint64_t offset_dwarf_address(uint64_t addr);

    /**
     * @brief 获取偏移 pc 地址。
     * 
     * @return uint64_t 
     */
    uint64_t get_offset_pc();

    /**
     * @brief 从指定内存地址读取64位数据
     * 
     * @param address 
     * @return uint64_t 
     */
    uint64_t read_memory(uint64_t address);

    /**
     * @brief 向指定内存地址写入64位数据
     * 
     * @param address 
     * @param value 
     */
    void write_memory(uint64_t address, uint64_t value);

    /**
        * @brief Set breakpoint by address
        * 
        * @param addr 
        */
    void set_breakpoint_at_address(std::intptr_t addr);

    /**
        * @brief 
        * 打印目标进程的寄存器信息  
        * 
        * @details 遍历寄存器描述符数组，通过 get_register_value 函数获取每个寄存器的值，并打印出来
        */
    void dump_registers();

    /**
     * @brief 设置 pc 值（寄存器rip）。
     * 
     * @param pc 
     */
    void set_pc(uint64_t pc);

    /**
     * @brief 等待目标进程发送信号并做出相应处理.
     * 
     * @details
     * - 如果收到 SIGTRAP 信号，通常表示进程遇到了断点，调用 handle_sigtrap 函数处理。
     * - 如果收到 SIGSEGV 信号，表示进程发生了段错误，输出相应信息。
    */
    void wait_for_signal();

    /**
     * @brief 跳过当前断点（执行一条指令），若当前指令没有断点，则不做任何事。 
     * 
     * @details 禁用当前断点, 使用 ptrace_singlestep, 恢复当前断点, 确保在下次执行到该断点时，程序会暂停执行
    */
    void step_over_breakpoint();

    /**
     * @brief 确定当前指令所属的函数
     * 
    */
    asm_head get_function_from_pc(uint64_t pc);

    /**
     * @brief Get the line entry from pc object
     * 
     * @return 返回源代码行的迭代器，如找不到源码，将抛出异常。常见于程序执行完毕、进入系统调用和库函数。
    */
    dwarf::line_table::iterator get_line_entry_from_pc(uint64_t pc);

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
    dwarf::line_table::iterator get_next_line_entry_from_pc(uint64_t pc);

    /**
     * @brief Get the signal info
     * 
     * @return siginfo_t 
    */
    siginfo_t get_signal_info();

    /**
     * @brief 向子进程发送信号，让子进程只执行一条指令
     *
     * @details 不进行断点检查，直接认为没有断点。
     * 
    */
    void single_step_instruction();

    /**
     * @brief 让子进程执行一条指令。根据当前指令是否有断点，分别调用step_over_breakpoint()和single_step()
     *
    */
    void single_step_instruction_with_breakpoint_check();

    /**
     * @brief 移除地址addr上的断点。
     * 
    */
    void remove_breakpoint(std::intptr_t addr);

    /**
     * @brief 跳出函数：从rbp获取返回地址，检查是否有断点，无则设置，然后continue；如刚设置了断点，则去除，如原有则不动。
     * 
    */
    void step_out();

    /**
     * @brief 单步进入/进入到下一个源代码行: 循环执行单条指令，源代码行号发生变化，循环结束
     * 
    */
    void step_in();

    /**
     * @brief 检查下一行源码是否设置了断点，如没有则设置，然后continue，再删除；如有则直接continue.
     * 
    */
    void step_over();

    /**
     * @brief Set the breakpoint at function object
     * 在源代码行条目的下一行设置断点. 如果停在函数序言，就无法观察到函数内部的变量状态、参数传递等信息。
    */
    void set_breakpoint_at_function(const std::string &name);

    /**
     * @brief 通过'file:line'形式的命令设置断点。
     * 
     * @param file 
     * @param line 
    */
    void set_breakpoint_at_source_file(const std::string &file, unsigned line);


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
    void initialise_load_address();

    /**
     * @brief 加载汇编数据到m_asm_vct向量中
     * 
    */
    void initialise_load_asm();

    /**
     * @brief 使用 objdump 命令生成反汇编代码
     * 
    */
    void initialise_run_objdump();

    /**
     * @brief 逐行读取源代码到 m_src_vct 向量中。
     * 
    */
    void initialise_load_src();

};   // class debugger

}   // namespace minidbg

#endif