//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJBuffer.h"

NS_HJ_BEGIN
//***********************************************************************************//
// 定义一个普通函数
int add(int a, double b) { return a + (int)b; }

// 提取add的类型信息
using AddTraits = HJFuncTraits<decltype(add)>;

// 1. 获取参数个数：2（int和double）
static_assert(AddTraits::arity == 2, "add有2个参数");

// 2. 获取返回值类型：int
static_assert(std::is_same_v<AddTraits::RetType, int>, "返回值为int");

// 3. 获取第0个参数类型：int
static_assert(std::is_same_v<AddTraits::args<0>::type, int>, "第0个参数是int");

// 4. 获取第1个参数类型：double
static_assert(std::is_same_v<AddTraits::args<1>::type, double>, "第1个参数是double");

class Calculator {
public:
    double multiply(int x, float y) const { return x * y; }
};

// 提取成员函数&Calculator::multiply的信息
using MultiplyTraits = HJFuncTraits<decltype(&Calculator::multiply)>;

// 1. 参数个数：2（int和float）
static_assert(MultiplyTraits::arity == 2, "multiply有2个参数");

// 2. 返回值类型：double
static_assert(std::is_same_v<MultiplyTraits::RetType, double>, "返回值为double");

// 3. 第0个参数类型：int
static_assert(std::is_same_v<MultiplyTraits::args<0>::type, int>, "第0个参数是int");


// 定义一个lambda表达式（无捕获时可隐式转换为函数指针）
auto lambda = [](std::string s, bool flag) { return s.size() + (flag ? 1 : 0); };

// 提取lambda的类型信息（通过函数对象特化）
using LambdaTraits = HJFuncTraits<decltype(lambda)>;

// 1. 参数个数：2（std::string和bool）
static_assert(LambdaTraits::arity == 2, "lambda有2个参数");

// 2. 返回值类型：size_t（std::string::size()的返回类型）
static_assert(std::is_same_v<LambdaTraits::RetType, size_t>, "返回值为size_t");

// 3. 第1个参数类型：bool
static_assert(std::is_same_v<LambdaTraits::args<1>::type, bool>, "第1个参数是bool");


// 定义std::function对象
std::function<void(int64_t)> func = [](int64_t val) { std::cout << val << std::endl; };

// 提取std::function的信息
using StdFuncTraits = HJFuncTraits<decltype(func)>;

// 1. 参数个数：1（int64_t）
static_assert(StdFuncTraits::arity == 1, "std::function有1个参数");

// 2. 返回值类型：void
static_assert(std::is_same_v<StdFuncTraits::RetType, void>, "返回值为void");


class MyClass {
public:
    int foo(double x) { return (int)x; } // 普通成员函数
};

// 提取MyClass::foo的类型信息
using FooTraits = HJFuncTraits<decltype(&MyClass::foo)>;

// 验证：参数个数为1，返回值为int，第0个参数为double
static_assert(FooTraits::arity == 1, "foo有1个参数");
static_assert(std::is_same_v<FooTraits::RetType, int>, "返回值为int");
static_assert(std::is_same_v<FooTraits::args<0>::type, double>, "参数为double");

NS_HJ_END