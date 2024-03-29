

#ifndef SYMBOLTYPE_HPP
#define SYMBOLTYPE_HPP

#include "elf/elf++.hh"
#include "symboltype.h"
#include <algorithm>



namespace symboltype
{

    /**
     * @brief 将 enum symbol_type 类型转换为字符串
     * 
     * @param st 符号类型
     * @return std::string 符号类型的字符串表示形式
     */
    std::string to_string(symbol_type st)
    {
        switch (st)
        {
        case symbol_type::notype:
            return "notype";
        case symbol_type::object:
            return "object";
        case symbol_type::func:
            return "func";
        case symbol_type::section:
            return "section";
        case symbol_type::file:
            return "file";
        default:
            return "unknown";
        }
    }

    /**
     * @brief 将elf++库中的符号类型转换为 enum symbol_type
     * 
     * @param sym elf++库中的符号类型
     * @return symbol_type 转换后的符号类型枚举
     */
    symbol_type to_symbol_type(elf::stt sym)
    {
        switch (sym)
        {
        case elf::stt::notype:
            return symbol_type::notype;
        case elf::stt::object:
            return symbol_type::object;
        case elf::stt::func:
            return symbol_type::func;
        case elf::stt::section:
            return symbol_type::section;
        case elf::stt::file:
            return symbol_type::file;
        default:
            return symbol_type::notype;
        }
    };


    bool symbol::operator==(const symbol &other) const {
        return (type == other.type) && (name == other.name) && (addr == other.addr);
    }

    /**
    * @brief 根据给定的符号名称，查找ELF文件中对应的符号，并返回包含所有匹配符号的向量。
    * 
    * @details
    * 该函数遍历ELF文件的所有节，检查类型为符号表（symtab）或动态符号表（dynsym）的节。
    * 对于每个符号，如果名称与给定的名称匹配，则将符号生成 symbol 结构体添加到结果向量中。
    * 最后，使用std::unique函数去除重复的符号，并返回结果向量。
    * 
    * @param name 符号名称
    * @return std::vector<symboltype::symbol> 包含所有匹配符号的向量
    */
    std::vector<symboltype::symbol> lookup_symbol(const std::string &name, elf::elf& m_elf)
    {
        std::vector<symboltype::symbol> syms;

        // 遍历ELF文件的所有节            
        for (auto &sec : m_elf.sections())
        {
            // 跳过非符号表和动态符号表类型的节
            if (sec.get_hdr().type != elf::sht::symtab && sec.get_hdr().type != elf::sht::dynsym)
                continue;

            for (auto sym : sec.as_symtab())
            {
                if (sym.get_name() == name)
                {
                    auto &d = sym.get_data();
                    syms.push_back(symboltype::symbol{symboltype::to_symbol_type(d.type()), sym.get_name(), d.value});
                }
            }
        }
        // 去重
        std::vector<symboltype::symbol>::iterator unique_end = std::unique(syms.begin(), syms.end());
        syms.erase(unique_end, syms.end());
        return syms;
    }
}


#endif