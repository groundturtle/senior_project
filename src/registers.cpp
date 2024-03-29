#include <sys/user.h>
#include <algorithm>
#include <sys/ptrace.h>

#include <sstream>
#include "registers.h"


namespace minidbg
{

    // 静态数组，包含寄存器描述符的实例, 每个实例表示一个寄存器的信息
    const std::array<reg_descriptor, n_registers> g_register_descriptors{{
        {reg::r15, 15, "r15"},
        {reg::r14, 14, "r14"},
        {reg::r13, 13, "r13"},
        {reg::r12, 12, "r12"},
        {reg::rbp, 6, "rbp"},
        {reg::rbx, 3, "rbx"},
        {reg::r11, 11, "r11"},
        {reg::r10, 10, "r10"},
        {reg::r9, 9, "r9"},
        {reg::r8, 8, "r8"},
        {reg::rax, 0, "rax"},
        {reg::rcx, 2, "rcx"},
        {reg::rdx, 1, "rdx"},
        {reg::rsi, 4, "rsi"},
        {reg::rdi, 5, "rdi"},
        {reg::orig_rax, -1, "orig_rax"},
        {reg::rip, -1, "rip"},
        {reg::cs, 51, "cs"},
        {reg::rflags, 49, "eflags"},
        {reg::rsp, 7, "rsp"},
        {reg::ss, 52, "ss"},
        {reg::fs_base, 58, "fs_base"},
        {reg::gs_base, 59, "gs_base"},
        {reg::ds, 53, "ds"},
        {reg::es, 50, "es"},
        {reg::fs, 54, "fs"},
        {reg::gs, 55, "gs"},
    }};

  
    std::string get_register_name(reg r)
    {
        auto it = std::find_if(begin(g_register_descriptors), end(g_register_descriptors),
                            [r](auto &&rd)
                            { return rd.r == r; });
        return it->name;
    }

    
    uint64_t get_register_value(pid_t pid, reg r) {
        user_regs_struct regs;
        if (ptrace(PTRACE_GETREGS, pid, nullptr, &regs) != 0) {
            std::ostringstream oss;
            oss << "Failed to get registers for pid " << pid;
            throw std::runtime_error(oss.str());
        }

        auto it = std::find_if(begin(g_register_descriptors), end(g_register_descriptors),
                            [r](const auto& rd)
                            { return rd.r == r; });
        if (it == end(g_register_descriptors)) {
            std::ostringstream oss;
            oss << "Register " << get_register_name(r) << " not found among register descriptors";
            throw std::out_of_range(oss.str());
        }

        // Here, ensure the offset calculation is correct for your architecture.
        auto offset = it - begin(g_register_descriptors);
        if (offset >= sizeof(regs) / sizeof(uint64_t)) {
            throw std::runtime_error("Register offset out of bounds");
        }

        return *(reinterpret_cast<uint64_t *>(&regs) + offset);
    }


    void set_register_value(pid_t pid, reg r, uint64_t value)
    {
        user_regs_struct regs;
        ptrace(PTRACE_GETREGS, pid, nullptr, &regs);
        auto it = std::find_if(begin(g_register_descriptors), end(g_register_descriptors),
                            [r](auto &&rd)
                            { return rd.r == r; });

        *(reinterpret_cast<uint64_t *>(&regs) + (it - begin(g_register_descriptors))) = value;
        ptrace(PTRACE_SETREGS, pid, nullptr, &regs);
    }


    uint64_t get_register_value_from_dwarf_register(pid_t pid, unsigned regnum) {
        auto it = std::find_if(begin(g_register_descriptors), end(g_register_descriptors),
                            [regnum](const auto& rd)
                            { return rd.dwarf_r == regnum; });
        if (it == end(g_register_descriptors)) {
            std::ostringstream oss;
            oss << "Unknown dwarf register number: " << regnum;
            throw std::out_of_range(oss.str());
        }

        try {
            return get_register_value(pid, it->r);
        } catch (const std::exception& e) {
            std::ostringstream oss;
            oss << "Failed to get value for register number " << regnum << " due to: " << e.what();
            throw std::runtime_error(oss.str());
        }
    }


    reg get_register_from_name(const std::string &name)
    {
        auto it = std::find_if(begin(g_register_descriptors), end(g_register_descriptors),
                            [name](auto &&rd)
                            { return rd.name == name; });
        return it->r;
    }

}