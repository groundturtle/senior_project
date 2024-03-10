#ifndef MINIGDB_REGISTERS_HPP
#define MINIGDB_REGISTERS_HPP

#include <sys/user.h>
#include <algorithm>

namespace minigdb
{
    /**
     * @brief 常见的寄存器名称
     * 
     */
    enum class reg
    {
        rax,
        rbx,
        rcx,
        rdx,
        rdi,
        rsi,
        rbp,
        rsp,
        r8,
        r9,
        r10,
        r11,
        r12,
        r13,
        r14,
        r15,
        rip,
        rflags,
        cs,
        orig_rax,
        fs_base,
        gs_base,
        fs,
        gs,
        ss,
        ds,
        es
    };

    static constexpr std::size_t n_registers = 27;

    /**
     * @brief 包含寄存器名称、对应的dwarf寄存器编号
     * 
     */
    struct reg_descriptor
    {
        reg r;
        int dwarf_r;
        std::string name;
    };

    // 静态数组，包含寄存器描述符的实例, 每个实例表示一个寄存器的信息
    static const std::array<reg_descriptor, n_registers> g_register_descriptors{{
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

    /**
    * @brief 根据给定的进程ID和寄存器枚举变量，获取该寄存器的值。
    * 
    * @param pid 目标进程的进程ID
    * @param r 要获取值的寄存器枚举变量
    * @return uint64_t 寄存器中存储的值
    */
    uint64_t get_register_value(pid_t pid, reg r)
    {
        user_regs_struct regs;
        ptrace(PTRACE_GETREGS, pid, nullptr, &regs);
        auto it = std::find_if(begin(g_register_descriptors), end(g_register_descriptors),
                            [r](auto &&rd)
                            { return rd.r == r; });

        return *(reinterpret_cast<uint64_t *>(&regs) + (it - begin(g_register_descriptors)));
    }

    /**
    * @brief 设置（改写）指定寄存器的值。
    * 
    * @details
    * 首先使用ptrace读取进程的寄存器状态到user_regs_struct结构体中，然后强制更改指定寄存器的前64位（即value），最后将更改后的寄存器状态写回目标进程。
    * 
    * @param pid 目标进程的进程ID
    * @param r 要设置值的寄存器枚举变量
    * @param value 要设置的值
    */
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

    /**
    * @brief 根据给定的进程ID和DWARF寄存器编号，获取该寄存器的值。
    * 
    * @param pid 目标进程的进程ID
    * @param regnum DWARF寄存器编号
    * @return uint64_t 寄存器中存储的值
    */
    uint64_t get_register_value_from_dwarf_register(pid_t pid, unsigned regnum)
    {
        auto it = std::find_if(begin(g_register_descriptors), end(g_register_descriptors),
                            [regnum](auto &&rd)
                            { return rd.dwarf_r == regnum; });
        if (it == end(g_register_descriptors))
        {
            throw std::out_of_range{"Unknown dwarf register"};
        }

        return get_register_value(pid, it->r);
    }

    /**
    * @brief 获取给定寄存器枚举变量的人类可读名称。
    * 
    * @param r 寄存器枚举变量
    * @return std::string 寄存器名称的字符串表示
    */
    std::string get_register_name(reg r)
    {
        auto it = std::find_if(begin(g_register_descriptors), end(g_register_descriptors),
                            [r](auto &&rd)
                            { return rd.r == r; });
        return it->name;
    }

    /**
    * @brief 根据给定的寄存器名称字符串，获取对应的寄存器枚举变量。
    * 
    * @param name 寄存器名称字符串
    * @return reg 对应的寄存器枚举变量
    */
    reg get_register_from_name(const std::string &name)
    {
        auto it = std::find_if(begin(g_register_descriptors), end(g_register_descriptors),
                            [name](auto &&rd)
                            { return rd.name == name; });
        return it->r;
    }
}
#endif