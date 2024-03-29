/**
 * @file asmparaser.h
 * @brief 逐行解析汇编文件，分为函数头asm_head和指令asm_entry
 * @version 0.1
 * @date 2024-02-19
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#ifndef MINIDBG_ASMPARASER_H
#define MINIDBG_ASMPARASER_H

#include <vector>
#include <string>
#include "utility.hpp"

namespace minidbg {

// assembly entry
struct asm_entry {
    uint64_t addr;          // Address of the instruction
    std::string mechine_code;  // Machine code of the instruction
    std::string asm_code;      // Assembly code of the instruction
    std::string comment;       // Comment associated with the instruction
};

// a block of assembly code
struct asm_head {
    uint64_t start_addr;            // Start address of the block
    uint64_t end_addr;              // End address of the block
    std::string function_name;      // Name of the function
    std::vector<asm_entry> asm_entris; // Vector containing assembly entries
};

/**
 * @brief 解析由objdump产生的反汇编文件。
 * 
 */
class asmparaser {

public:

    /**
     * @brief 解析汇编文件，将汇编条目填入向量中。
     * 
     * @param file_path 
     * @return std::vector<asm_head> 
    */
    std::vector<asm_head> get_asm_data(std::string file_path);

private:
    /**
     * @brief 去除字符串左侧的空格
     * 
     * @param str 
    */
    void trimLeft(std::string &str);

    /**
     * @brief 去除字符串右侧的空格
     * 
     * @param str 
     */
    void trimRight(std::string &str);

    /**
     * @brief 解析单个汇编指令将其转换为 asm_entry 结构体.
     * 
     * @param command 
     * @return asm_head 
     */
    asm_entry cope_asm_entry(std::string &command);

    /**
     * @brief 解析汇编文件中的函数头部信息
     * 
     * @param command 
     * @return asm_head 
     */
    asm_head cope_asm_head(std::string &command);
    };

} // namespace minidbg

#endif // MINIDBG_ASMPARASER_H
