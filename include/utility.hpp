#include<string>

/**
 * @brief 判断字符串s是否为字符串of的前缀。
 * 
 * @param s 要检查的前缀字符串
 * @param of 目标字符串
 * @return true 如果s是of的前缀
 * @return false 如果s不是of的前缀
 */
bool is_prefix(const std::string &s, const std::string &of)
{
    // 如果`s`是`of`的前缀
    // 例如：s="con" of="continue" 返回true
    if (s.size() > of.size())
        return false;
    return std::equal(s.begin(), s.end(), of.begin());
}

/**
 * @brief 判断地址是否有效：逐行读取/proc/_pid/maps文件。
 * 
 * @param pid 
 * @param address 
 * @return bool 
 */
bool is_valid_address(pid_t pid, uintptr_t address) {
    std::ifstream maps_file("/proc/" + std::to_string(pid) + "/maps");
    std::string line;
    while (std::getline(maps_file, line)) {
        uintptr_t start_addr, end_addr;
        sscanf(line.c_str(), "%lx-%lx", &start_addr, &end_addr);
        if (address >= start_addr && address < end_addr) {
            return true; // 地址有效
        }
    }
    return false; // 地址无效
}