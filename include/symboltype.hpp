/**
 * @file symboltype.hpp
 * @author your name (you@domain.com)
 * @brief 定义了符号类型的枚举类、符号描述结构体，符号与elf中的符号类型一一对应并有转换函数。
 * @details 符号映射的作用：
 * 1. 解耦合：将libelfin的符号类型映射到自定义枚举类型可以减少对特定第三方库的依赖。这意味着如果将来决定更换底层库，或者libelfin库发生了变化，只需更新这个映射函数即可，而不用修改整个代码库中对符号类型的引用。这样做提高了代码的模块化和可维护性。
 * 2. 接口清晰：使用自定义枚举类型作为应用程序内部的标准，可以确保接口的一致和清晰（命名）。
 * @version 0.1
 * @date 2024-03-11
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef SYMBOLTYPE_HPP
#define SYMBOLTYPE_HPP

#include "elf/elf++.hh"
namespace symboltype
{
    /**
     * @brief 符号类型枚举
     * 
     * 符号类型枚举定义了不同类型的符号，包括无类型、数据对象、函数入口点、关联节和源文件。
    */
    enum class symbol_type
    {
        notype,  /**< 无类型（例如，绝对符号） */
        object,  /**< 数据对象 */
        func,    /**< 函数入口点 */
        section, /**< 符号与节相关联 */
        file,    /**< 与对象文件关联的源文件 */
    };           

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
     * @brief 符号结构体, 包含符号的类型、名称和地址。
     */
    struct symbol
    {
        symbol_type type;
        std::string name;
        std::uintptr_t addr;
        bool operator==(const symbol &other) const
        {
           return (type==other.type)&&(name==other.name)&&(addr==other.addr);
        }
    };

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