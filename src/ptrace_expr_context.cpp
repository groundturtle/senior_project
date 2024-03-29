#include "ptrace_expr_context.h"
#include "utility.hpp"
#include <iostream>
#include <iomanip>  
#include <sys/ptrace.h> 
#include "registers.h"
#include "utility.hpp"

namespace minidbg{

ptrace_expr_context::ptrace_expr_context(pid_t pid, uint64_t load_address) 
    : m_pid(pid), m_load_address(load_address) {}

dwarf::taddr ptrace_expr_context::reg(unsigned regnum) {
    return get_register_value_from_dwarf_register(m_pid, regnum);
}

dwarf::taddr ptrace_expr_context::pc(){
    struct user_regs_struct regs;
    ptrace(PTRACE_GETREGS, m_pid, nullptr, &regs);
    return regs.rip - m_load_address;
}

dwarf::taddr ptrace_expr_context::deref_size(dwarf::taddr address, unsigned size) {

    uint64_t full_address = address + m_load_address;
    if (!utility::is_valid_address(m_pid, full_address)) {
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

}