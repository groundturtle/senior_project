#include "asmparaser.h"
#include "utility.hpp"


namespace minidbg{
/**
 * @brief 解析汇编文件，将汇编条目填入向量中。
 * 
 * @param file_path 
 * @return std::vector<asm_head> 
 */
std::vector<asm_head> asmparaser::get_asm_data(std::string file_path)
{
    std::vector<asm_head> result;
    std::ifstream inFile(file_path);
    std::string line;

    if (!inFile)
    {
        std::cerr << "Failed to open input file " << file_path << std::endl;
        return result;
    }

    // read line by line, entry or head
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

/**
 * @brief 去除字符串左侧的空格
 * 
 * @param str 
 */
void asmparaser::trimLeft(std::string &str)
{
    int index = str.find_first_not_of(' ');
    if (index != std::string::npos)
        str.erase(0, index);
}

/**
 * @brief 去除字符串右侧的空格
 * 
 * @param str 
 */
void asmparaser::trimRight(std::string &str)
{
    int index = str.find_last_not_of(' ');
    if (index != std::string::npos)
        str.erase(index + 1);
}

/**
 * @brief 解析单个汇编指令将其转换为 asm_entry 结构体.
 * 
 * @param command 
 * @return asm_entry 
 */
asm_entry asmparaser::cope_asm_entry(std::string &command)
{
    asm_entry result;
    // 输入的字符串按制表符分割
    std::vector<std::string> args = utility::split(command, '\t');
    if (args.back().find('#') != std::string::npos)         // 行尾注释
    {
        std::vector<std::string> temp = utility::split(args.back(), '#');
        args.pop_back();
        args.push_back(temp[0]);
        args.push_back(temp[1]);
    }

    for (int i = 0; i < args.size(); i++)
    {
        trimLeft(args[i]);
        trimRight(args[i]);
    }
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

/**
 * @brief 解析汇编文件中的函数头部信息
 * 
 * @param command 
 * @return asm_head 
 */
asm_head asmparaser::cope_asm_head(std::string &command)
{
    std::vector<std::string> temp = utility::split(command, ' ');
    asm_head head;
    head.start_addr = std::stol(temp[0], 0, 16);
    head.function_name = temp[1].substr(1, temp[1].size() - 3);
    return head;
}

}
