#ifndef PTRACE_EXPR_CONTEXT_HPP
#define PTRACE_EXPR_CONTEXT_HPP


#include "dwarf/dwarf++.hh"
#include "elf/elf++.hh"
#include "dwarf/expr.cc"
/**
* @class ptrace_expr_context
* @brief 为dwarf表达式提供评估环境，使用ptrace获取变量值。
* 
* @details 该类覆写了dwarf::expr_context中的几个关键函数，以便使用ptrace从调试目标进程获取寄存器和内存值。
*/
class ptrace_expr_context : public dwarf::expr_context {
public:
    /**
    * @brief 构造函数，初始化pid和加载地址。
    * @param pid 被调试的进程ID。
    * @param load_address 程序的加载地址，用于地址计算。
    */
    ptrace_expr_context(pid_t pid, uint64_t load_address);

    /**
    * @brief 获取指定寄存器的值。
    * @param regnum DWARF定义的寄存器编号。
    * @return 寄存器的值。
    */
    dwarf::taddr reg(unsigned regnum) override;

    /**
    * @brief 获取程序计数器的当前值。
    * @return 程序计数器的值，相对于加载地址。
    */
    dwarf::taddr pc();

    /**
    * @brief 读取指定地址的内存值。
    * @param address 目标地址，相对于加载地址。
    * @param size 读取的数据大小（目前未使用）。
    * @return 地址处的内存值。
    */
    dwarf::taddr deref_size(dwarf::taddr address, unsigned size) override;

private:
    pid_t m_pid; // 被调试的进程ID
    uint64_t m_load_address; // 程序加载地址
};


ptrace_expr_context::ptrace_expr_context(pid_t pid, uint64_t load_address) 
    : m_pid(pid), m_load_address(load_address) {}

dwarf::taddr ptrace_expr_context::reg(unsigned regnum) {
    return minidbg::get_register_value_from_dwarf_register(m_pid, regnum);
}

dwarf::taddr ptrace_expr_context::pc(){
    struct user_regs_struct regs;
    ptrace(PTRACE_GETREGS, m_pid, nullptr, &regs);
    return regs.rip - m_load_address;
}

dwarf::taddr ptrace_expr_context::deref_size(dwarf::taddr address, unsigned size) {

    uint64_t full_address = address + m_load_address;
    if (!is_valid_address(m_pid, full_address)) {
        std::cerr << "Attempt to dereference invalid address: " << std::hex << full_address << std::endl;
        return 0; // 或其他错误处理方式
    }
    errno = 0;
    long data = ptrace(PTRACE_PEEKDATA, m_pid, full_address, nullptr);
    if (errno != 0) {
        // 处理错误，例如打印错误消息或返回一个特定的错误值
        std::cerr << "ptrace PEEKDATA failed at address: " << std::hex << full_address << std::dec << " with errno: " << errno << std::endl;
        return 0; // 或者其他合适的错误处理方式
    }
    return data;
}

#endif