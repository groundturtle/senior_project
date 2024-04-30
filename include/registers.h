/**
 * @file registers.h
 * @brief 本文件定义了常用寄存器枚举类reg、寄存器描述符（struct，成员包括相对应的枚举变量、名称、dwarf编号），读取和设置寄存器值的函数。
 * @version 0.1
 * @date 2024-03-11
 * 
 * @copyright Copyright (c) 2024
 * 
 */
 
#ifndef REGISTERS_H
#define REGISTERS_H

#include <sys/user.h>
#include <string>
#include <array>
#include <cstdint>

namespace minidbg
{
    /**
     * @brief 常见的寄存器名称
     * 
     */
    enum class reg {
        rax, rbx, rcx, rdx,
        rdi, rsi, rbp, rsp,
        r8, r9, r10, r11,
        r12, r13, r14, r15,
        rip, rflags, cs,
        orig_rax, fs_base, gs_base,
        fs, gs, ss, ds, es
    };

    /**
     * @brief 寄存器数量
     * 
     */
    static constexpr std::size_t n_registers = 27;

    /**
     * @brief 寄存器描述符，包括寄存器的枚举、DWARF编号和名称
     * 
     */
    struct reg_descriptor {
        reg r;
        int dwarf_r;
        std::string name;
    };

    /**
     * @brief 寄存器描述符数组
     * 
     */
    extern const std::array<reg_descriptor, n_registers> g_register_descriptors;

    /**
     * @brief 获取寄存器的名称
     * 
     * @param r 寄存器枚举
     * @return std::string 寄存器名称
     */
    std::string get_register_name(reg r);

    /**
     * @brief 获取寄存器的值
     * 
     * @param pid 进程ID
     * @param r 寄存器枚举
     * @return uint64_t 寄存器值
     */
    uint64_t get_register_value(pid_t pid, reg r);

    /**
     * @brief 设置寄存器的值
     * 
     * @param pid 进程ID
     * @param r 寄存器枚举
     * @param value 值
     */
    void set_register_value(pid_t pid, reg r, uint64_t value);

    /**
     * @brief 通过DWARF编号获取寄存器的值
     * 
     * @param pid 进程ID
     * @param regnum DWARF编号
     * @return uint64_t 寄存器值
     */
    uint64_t get_register_value_from_dwarf_register(pid_t pid, unsigned regnum);

    /**
     * @brief 通过寄存器名称获取寄存器枚举
     * 
     * @param name 寄存器名称
     * @return reg 寄存器枚举
     */
    reg get_register_from_name(const std::string &name);
}

#endif // MINIDBG_REGISTERS_HPP
