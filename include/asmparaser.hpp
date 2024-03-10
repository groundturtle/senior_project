/**
 * @file asmparaser.hpp
 * @brief 逐行解析汇编文件，分为函数头asm_head和指令asm_entry
 * @version 0.1
 * @date 2024-02-19
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#ifndef MINIGDB_ASMPARASER_HPP
#define MINIGDB_ASMPARASER_HPP

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
namespace minigdb
{
    // split a string into parts
    std::vector<std::string> split(const std::string &s, char delimiter)
    {
        std::vector<std::string> out{};
        std::stringstream ss{s};
        std::string item;

        while (std::getline(ss, item, delimiter))
        {
            out.push_back(item);
        }
        return out;
    };

    // assembly entry
    struct asm_entry
    {
        uint64_t addr;          // Address of the instruction
        std::string mechine_code;  // Machine code of the instruction
        std::string asm_code;      // Assembly code of the instruction
        std::string comment;       // Comment associated with the instruction
    };

    // a block of assembly code 函数
    struct asm_head {
        uint64_t start_addr;            // Start address of the block
        uint64_t end_addr;              // End address of the block
        std::string function_name;      // Name of the function
        std::vector<asm_entry> asm_entris; // Vector containing assembly entries
    };

    // 解析由objdump产生的汇编文件
    class asmparaser
    {
    public:
        // 解析由objdump产生的汇编文件
        std::vector<asm_head> get_asm_data(std::string file_path)
        {
            std::vector<asm_head> result;
            std::ifstream inFile(file_path);
            std::string line;

            if (!inFile)
            {
                std::cerr << "Failed to open input file " << file_path << std::endl;
                return result;
            }

            // read line by line, entry or head ????
            while (std::getline(inFile, line))
            {
                if (line.size() == 0)
                    continue;
                // 如果当前行不是以 "Disassembly" 开头，则说明该行是汇编指令的一部分
                else if (line.compare(0, 11, "Disassembly") != 0)
                {
                    // 如果当前行中不包含制表符 '\t'，则说明该行是函数头部信息
                    if (line.find('\t') == std::string::npos)
                    {
                        result.push_back(cope_asm_head(line));
                    }
                    // 否则，当前行是汇编指令的一部分, 加入result结尾元素的asm_entris中
                    else
                    {
                        result.back().asm_entris.push_back(cope_asm_entry(line));
                    }
                }
            }

            // Set the end address for each block of assembly code
            for (auto &head : result)
            {
                head.end_addr = head.asm_entris.back().addr;
            }

            inFile.close();
            return result;
        }

    private:
        // 去除左边的空格
        void trimLeft(std::string &str)
        {
            int index = str.find_first_not_of(' ');
            if (index != std::string::npos)
                str.erase(0, index);
        }

        // 去除右边的空格
        void trimRight(std::string &str)
        {
            int index = str.find_last_not_of(' ');
            if (index != std::string::npos)
                str.erase(index + 1);
        }

        // 解析单个汇编指令将其转换为 asm_head 结构体.
        asm_entry cope_asm_entry(std::string &command)
        {
            // 输入的字符串按制表符分割
            std::vector<std::string> args = split(command, '\t');
            if (args.back().find('#') != std::string::npos)
            {
                std::vector<std::string> temp = split(args.back(), '#');
                args.pop_back();
                args.push_back(temp[0]);
                args.push_back(temp[1]);
            }

            for (int i = 0; i < args.size(); i++)
            {
                trimLeft(args[i]);
                trimRight(args[i]);
            }
            asm_entry result;
            if (args.size() < 3)
            {
                return result;
            }
            else
            {
                result.addr = std::stol(args[0], 0, 16);
                result.mechine_code = args[1];
                result.asm_code = args[2];
                if (args.size() == 4)
                    result.comment = args[3];
                return result;
            }
        }

        // 解析汇编文件中的函数头部信息
        asm_head cope_asm_head(std::string &command)
        {
            std::vector<std::string> temp = split(command, ' ');
            asm_head head;
            head.start_addr = std::stol(temp[0], 0, 16);
            head.function_name = temp[1].substr(1, temp[1].size() - 3);
            return head;
        }
    };
};

#endif