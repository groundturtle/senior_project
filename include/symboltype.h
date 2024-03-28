/**
 * @file symboltype.h
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

namespace symboltype {

    enum class symbol_type {
        notype,  // 无类型（例如，绝对符号）
        object,  // 数据对象
        func,    // 函数入口点
        section, // 符号与节相关联
        file,    // 与对象文件关联的源文件
    };

    std::string to_string(symbol_type st);

    struct symbol {
        symbol_type type;
        std::string name;
        std::uintptr_t addr;

        bool operator==(const symbol &other) const;
    };

    symbol_type to_symbol_type(elf::stt sym);

    std::vector<symbol> lookup_symbol(const std::string &name, elf::elf& m_elf);

}