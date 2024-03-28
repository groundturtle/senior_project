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

namespace minidbg
{
    struct asm_entry
    {
        uint64_t addr;
        std::string mechine_code;
        std::string asm_code;
        std::string comment;
    };

    struct asm_head {
        uint64_t start_addr;
        uint64_t end_addr;
        std::string function_name;
        std::vector<asm_entry> asm_entris;
    };

    class asmparaser
    {
    public:
        std::vector<asm_head> get_asm_data(std::string file_path);

    private:
        void trimLeft(std::string &str);
        void trimRight(std::string &str);
        asm_entry cope_asm_entry(std::string &command);
        asm_head cope_asm_head(std::string &command);
    };
};

#endif // MINIDBG_ASMPARASER_H