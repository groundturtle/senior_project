/**
 * @file breakpoint.h
 * @brief 断点类，包含启用和禁用功能，实例化后存储于debugger类中的m_breakpoints向量。
 * @version 0.1
 * @date 2024-03-11
 * 
 * Copyright (c) 2024
 * 
 */
#ifndef BREAKPOINT_H
#define BREAKPOINT_H

#include <linux/types.h>
#include "utility.hpp"
#include <string>

namespace minidbg
{
    /**
     * @brief 断点类，包含启用和禁用功能，实例化后存储于debugger类中的m_breakpoints向量。
     * 
     */
    class breakpoint
    {
    public:
        breakpoint();
        breakpoint(pid_t pid, std::intptr_t addr);

        void enable();
        void disable();
        auto is_enabled() const -> bool;
        auto get_address() const -> intptr_t;

    private:
        pid_t m_pid;
        std::intptr_t m_addr;
        bool m_enabled;
        uint8_t m_save_data;
    };
}

#endif // BREAKPOINT_H
