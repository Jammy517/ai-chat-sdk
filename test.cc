#include <iostream>
#include <spdlog/spdlog.h>

int main()
{
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v");
    spdlog::info("hello,world");
    return 0;
}