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
}