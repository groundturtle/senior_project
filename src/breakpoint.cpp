#include "asmparaser.h"
#include <iostream>
#include <fstream>
#include <sstream>

namespace minidbg
{

breakpoint::breakpoint() = default;
breakpoint::breakpoint(pid_t pid, std::intptr_t addr)
    : m_pid{pid}, m_addr{addr}, m_enabled{false}, m_save_data{}
{
}

/**
 * @brief 启用断点
 * 通过ptrace系统调用读取原始数据，将其最低有效字节（LSB）
 * 替换为软件中断0xcc（用于暂停程序执行），
 * 然后使用PTRACE_POKEDATA将修改后的数据写入目标地址
 */
void breakpoint::enable()
{
    auto data = ptrace(PTRACE_PEEKDATA, m_pid, m_addr, nullptr);
    m_save_data = static_cast<uint8_t>(data & 0xff);
    uint64_t int3 = 0xcc; // 系统软件中断，程序运行到这个地方，就会执行主函数的wait函数
    uint64_t data_with_int3 = ((data & ~0xff) | int3);
    ptrace(PTRACE_POKEDATA, m_pid, m_addr, data_with_int3);

    m_enabled = true;
};

/**
 * @brief 将软件中断0xcc替换成原来的数据. 删除debugger类的向量m_breakpoints的元素前，要先禁用，才是真正删除。
 * 
 */
void breakpoint::disable()
{
    auto data = ptrace(PTRACE_PEEKDATA, m_pid, m_addr, nullptr);
    auto restore_data = ((data & ~0xff) | m_save_data);
    ptrace(PTRACE_POKEDATA, m_pid, m_addr, restore_data);

    m_enabled = false;
};

auto breakpoint::is_enabled() const -> bool { return m_enabled; }

/**
 * @brief Get the breakpoint address object
 * @return intptr_t 
 */
auto breakpoint::get_address() const -> intptr_t { return m_addr; }

};  // minidbg