# CMake 最低版本要求
cmake_minimum_required(VERSION 3.10)

# 项⽬名称
project(LLMTest)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置构建类型为Debug
set(CMAKE_BUILD_TYPE Debug)

# 添加可执⾏⽂件
add_executable(LLMTest TestLLM.cpp ../sdk/src/util/log.cpp ../sdk/src/DeepSeekProvider.cpp)

# 设置输出⽬录
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})

# 添加头⽂件搜索路径
include_directories(${PROJECT_SOURCE_DIR}/../sdk/include)

# 查找OpenSSL包
find_package(OpenSSL REQUIRED)

# 包含OpenSSL头⽂件路径
include_directories(${OPENSSL_INCLUDE_DIR})

# 添加编译定义
target_compile_definitions(LLMTest PUBLIC
CPPHTTPLIB_OPENSSL_SUPPORT
)

# 链接库
target_link_libraries(LLMTest fmt jsoncpp OpenSSL::SSL OpenSSL::Crypto spdlog
gtest)