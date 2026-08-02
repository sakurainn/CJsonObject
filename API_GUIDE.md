# cppJSON API 使用指引

cppJSON 是一个基于 cJSON（官方 v1.7.19 + 64 位整数扩展）的 C++ JSON 封装库。本仓库在官方 cJSON 基础上做了两项增强，贯穿本指引：

- **64 位整数精确支持**：`int64`/`uint64` 全范围精确解析、生成、读取，无 double 精度损失。
- **顺序数组遍历接口**：`GetNextValue()`，把 `Get(i)` 循环的 O(n²) 降为 O(n)。

---

## 1. 编译与集成

项目自带文件：

```
cppJSON.hpp    cppJSON.cpp
CMakeLists.txt     patches/cJSON_v1.7.19_cppJSON.patch
```

**cJSON 和 Ryū 都不随仓库发布**，构建时自动拉取：

- **cJSON**：官方 v1.7.19 → 应用补丁 `patches/cJSON_v1.7.19_cppJSON.patch`（含 64 位整数、`sign` 字段、`CreateInt64/Uint64`、Ryū 打印、解析优化等全部扩展）
- **Ryū**：固定 commit `4c0618b`（浮点最短精确算法，原版直接使用，无需补丁）

| 项 | 要求 |
|----|------|
| C++ 标准 | **C++11 及以上**（STL 容器支持、`operator\|`、`nullptr` 等特性依赖 C++11） |
| 构建工具 | **CMake**（推荐）；需要 **git**（应用 cJSON 补丁）和**网络**（下载 cJSON/Ryū；受限环境可设 `HTTPS_PROXY`） |
| 依赖 | 无第三方运行时库，纯标准库 + C 标准库 |

### CMake（推荐，自动拉取 + 补丁 + 编译）

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

`configure` 时自动完成：拉取官方 cJSON v1.7.19 + Ryū（固定 commit）→ `git apply` 应用 cJSON 补丁 → 编译 `libcppJSON`。

### 手动编译（不走 CMake）

```bash
# 1. 拉取官方 cJSON v1.7.19
curl -L -o cJSON.c https://raw.githubusercontent.com/DaveGamble/cJSON/v1.7.19/cJSON.c
curl -L -o cJSON.h https://raw.githubusercontent.com/DaveGamble/cJSON/v1.7.19/cJSON.h
# 2. 应用 cJSON 补丁（git apply 需要 git index）
git init -q && git add cJSON.c cJSON.h
git apply patches/cJSON_v1.7.19_cppJSON.patch
# 3. 拉取 Ryū（固定 commit，保持 ryu/ 目录结构）
mkdir -p ryu && cd ryu
for f in d2s.c ryu.h common.h digit_table.h d2s_intrinsics.h d2s_full_table.h; do
  curl -sL -o $f https://raw.githubusercontent.com/ulfjack/ryu/4c0618b0e44f7ef027ebae05d2cc7812048f7c8f/ryu/$f
done
cd ..
# 4. 编译
gcc  -c -o cJSON.o cJSON.c
gcc  -c -o ryu/d2s.o ryu/d2s.c -I.
g++  -std=c++11 -c -o cppJSON.o cppJSON.cpp
g++  -o app app.cpp cppJSON.o cJSON.o ryu/d2s.o
```

> Ryū（Apache/Boost 双许可）用于 `double` 的**最短、精确**格式化——浮点序列化性能约 **10×**，
> round-trip 精确（打印后解析能逐位还原原值）。若不需要可移除 Ryū 并回退到原 `%g` 实现（需同步调整补丁）。

引用头文件：

```cpp
#include "cppJSON.hpp"
```

`cppJSON` 是**全局类**（无命名空间），直接用即可。

### CMake 集成

提供 `CMakeLists.txt`，支持**子项目集成**（`add_subdirectory`）或**独立构建**。

**作为子项目集成**（推荐，直接链接 `cppJSON::cppJSON`）：

```cmake
# 你的 CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(your_app CXX)

add_subdirectory(path/to/cppJSON)
target_link_libraries(your_target PRIVATE cppJSON::cppJSON)
```

**独立构建**：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

CMake 选项：

| 选项 | 默认 | 说明 |
|------|------|------|
| `CPPJSON_BUILD_DEMO` | ON | 构建 demo（仅顶层项目时生效；作为子项目时自动跳过） |
| `CPPJSON_BUILD_SHARED` | OFF | 构建共享库（默认静态 `libcppJSON`） |

> 作为子项目集成时，demo **不会**被构建（只有 `CMAKE_CURRENT_SOURCE_DIR == CMAKE_SOURCE_DIR` 时才构建 demo），避免污染你的构建树。

---

## 2. 快速上手

```cpp
#include "cppJSON.hpp"
#include <iostream>

int main()
{
    // 从 JSON 字符串解析
    cppJSON oJson(
        "{\"name\":\"nebula\","
        "\"version\":1,"
        "\"plugins\":[\"log\",\"net\"]}");

    // 读取
    std::string strName;
    int iVersion = 0;
    oJson.Get("name", strName);
    oJson.Get("version", iVersion);
    std::cout << strName << " " << iVersion << std::endl;   // nebula 1

    // 修改
    oJson.Replace("version", 2);
    oJson.Add("owner", "Bwar");

    // 数组追加
    oJson["plugins"].Add("http");

    // 序列化
    std::cout << oJson.ToString() << std::endl;
    // {"name":"nebula","version":2,"plugins":["log","net","http"],"owner":"Bwar"}

    return 0;
}
```

---

## 3. 赋值语法 `a["key"] = value`（map 风格）

从空对象开始，可直接用下标赋值构建 JSON，无需先建空子对象：

```cpp
cppJSON o;
o["name"]  = "nebula";                        // 字符串
o["count"] = 42;                              // 整数
o["pi"]    = 3.14;                            // 浮点
o["ok"]    = true;                            // 布尔
o["big"]   = (int64)1283949231388184576LL;    // 64 位整数，精确
o["sub"]["deep"] = "nested";                  // 嵌套，逐级自动创建

// 结果：
// {"name":"nebula","count":42,"pi":3.14,"ok":true,
//  "big":1283949231388184576,"sub":{"deep":"nested"}}
```

### 语义说明

| 场景 | 行为 |
|------|------|
| key 不存在 | 自动创建并赋值（std::map 风格） |
| key 已存在 | 覆盖旧值 |
| 嵌套 `o["a"]["b"] = v` | 逐级自动创建中间节点 |
| 数组元素 `arr[i] = v` | 仅 `i` 已存在时生效（修改该元素）；越界赋值不影响数组 |
| 对象赋值 `o["k"] = subJson` | 深拷贝子对象内容到 key |
| 链式 `o["a"] = o["b"] = 7` | 两个 key 都赋 7 |

支持的类型：`int32`、`uint32`、`int64`、`uint64`、`float`、`double`、`bool`、`const char*`、`std::string`、`cppJSON`。

> ⚠️ **读取时注意**：`o["k"]` 若 key 不存在会**创建空节点**。判断 key 是否存在、避免误读拼错的 key 污染数据，用：

```cpp
if (o.KeyExist("k"))     // 先判断
{
    std::string v;
    o.Get("k", v);       // 再读取
}
```

### 默认值读取（ArduinoJson 风格）：`auto v = obj["key"] | defval`

支持类似 ArduinoJson 的 `doc["key"] | default` 语法，key 缺失或类型不符时返回默认值：

```cpp
cppJSON o("{\"count\":42,\"pi\":3.14,\"name\":\"nebula\",\"ok\":true,\"big\":1283949231388184576}");

int     count = o["count"] | 0;                 // 42
double  pi    = o["pi"]    | 0.0;               // 3.14
std::string name = o["name"] | std::string("none");  // "nebula"
const char* nm  = o["name"] | "none";           // "nebula"（指向内部，生命周期同 cJSON）
bool    ok    = o["ok"]    | false;             // true
int64   big   = o["big"]   | (int64)0;          // 1283949231388184576（64位精确）
int     miss  = o["missing"] | 99;              // key 缺失 → 99
int     mis2  = o["name"] | 5;                  // 类型不符（字符串）→ 5
```

支持的 T 类型：`int32`/`uint32`/`int64`/`uint64`/`float`/`double`/`bool`/`std::string`/`const char*`。数值类型间自动转换（如整数值可读为 double）。

**`nullptr` 与 `NULL`**：

```cpp
const char* s = o["name"] | nullptr;     // "nebula"；值是字符串时返回内部指针
const char* t = o["missing"] | nullptr;  // nullptr；key 缺失或不是字符串
int n = o["count"] | NULL;               // 42 —— 注意！NULL 就是 0，走的是数值路径（int）
int m = o["missing"] | NULL;             // 0
```

- `| nullptr` 返回 `const char*`：值为 JSON 字符串时指向内部 `valuestring`（生命周期同 cJSON 对象），否则 `nullptr`。适合"取字符串或空"的指针语义。
- `| NULL` 的 `NULL` 在 C++ 中是 `0`，被当作 `int` 数值默认值处理（不是指针）。**要用指针空默认值请用 `nullptr`**。

> ⚠️ **副作用警告（重要）**：`obj["key"] | defval` 会先求值 `obj["key"]`，而**非 const** `operator[]` 是 map 语义——key 不存在会被创建成空节点。要**无副作用**地读取，用以下两种方式之一：

```cpp
// 方式一：const 引用（推荐，语法不变）
const cppJSON& ro = o;
int v = ro["notthere"] | 7;       // 7，且不会创建 "notthere"

// 方式二：Get(key, defval)（无需 const 引用，直接无副作用）
int v2 = o.Get("notthere", 7);    // 7，不创建任何节点
std::string s = o.Get("name", std::string("x"));
```

`Get(key, defval)` 与既有 `Get(key, value)`（传引用）重载共存，自动区分：`o.Get("k", 42)` 走默认值版，`o.Get("k", v)`（v 是变量）走引用版。

---

## 4. 构造与生命周期

cppJSON 是 **RAII 类型**，内部 cJSON 内存随析构自动释放，**无需手动 free**（这是相对裸 cJSON 最大的便利）。

```cpp
cppJSON o1;                          // 空对象
cppJSON o2("{\"a\":1}");             // 从 JSON 字符串解析
cppJSON* p = new cppJSON(o2);  // 从指针复制
cppJSON o3(o2);                      // 拷贝构造（深拷贝）
cppJSON o4(std::move(o2));           // C++11 移动构造
o1 = o3;                                      // 拷贝赋值
```

注意：

- **拷贝是深拷贝**，两个对象互不影响。
- **解析失败**不抛异常，构造出的对象为空，可用 `GetErrMsg()` 查原因（见 §10）。
- `operator==` 比较的是序列化文本（`ToString()`），即逻辑相等。

---

## 5. 解析与序列化

| 方法 | 说明 |
|------|------|
| `bool Parse(const std::string&)` | 从字符串解析，成功返回 true；失败返回 false，原数据被清空 |
| `std::string ToString()` | 紧凑序列化（无空白字符） |
| `std::string ToFormattedString()` | 格式化序列化（带缩进，便于阅读） |
| `const std::string& GetErrMsg()` | 最近一次操作的错误信息 |

```cpp
cppJSON o;
if (!o.Parse("{\"bad\": }"))   // 非法 JSON
{
    std::cerr << o.GetErrMsg() << std::endl;   // 报错位置
}
```

---

## 6. 对象（Object）操作

对象形如 `{"key": value, ...}`。

### 6.1 读取

```cpp
cppJSON o("{\"name\":\"nebula\",\"count\":42,\"ok\":true,\"pi\":3.14,\"child\":{\"x\":1}}");

// 方式一：Get()，推荐 —— 带类型检查，类型不符返回 false
std::string name;
int64 count = 0;
bool ok = false;
double pi = 0.0;
o.Get("name", name);    // true
o.Get("count", count);  // true
o.Get("ok", ok);        // true
o.Get("pi", pi);        // true

// 方式二：operator()，返回字符串表示，取不到返回空串
std::string s = o("name");       // "nebula"
std::string s2 = o("nokey");     // ""

// 方式三：operator[]，返回 cppJSON 引用，用于访问嵌套子对象
cppJSON& sub = o["child"];
int x = 0;
sub.Get("x", x);                 // 1
```

`Get()` 支持的类型：`cppJSON`、`std::string`、`int32`、`uint32`、`int64`、`uint64`、`bool`、`float`、`double`。

配套查询：

```cpp
o.KeyExist("name");          // true —— key 是否存在
o.ValueType("count");        // 返回 cJSON_Number 等类型值
o.IsNull("nokey");           // 是否为 null 值
o.IsEmpty();                 // 是否无数据
o.IsArray();                 // 是否为数组
```

### 6.2 增 / 改 / 删

```cpp
cppJSON o;

o.Add("str",  std::string("hello"));   // 字符串
o.Add("i32",  (int32)1);               // 32 位整数
o.Add("i64",  (int64)-9223372036854775807LL - 1);  // 64 位整数，精确
o.Add("u64",  (uint64)18446744073709551615ULL);    // 64 位无符号，精确
o.Add("dbl",  3.14);                   // 浮点
o.Add("flag", true, false);            // 布尔 —— 注意两个参数，用于重载区分

o.AddNull("nil");                      // "nil": null

// 嵌套：先建空子对象/子数组，再往里填
o.AddEmptySubObject("config");
o["config"].Add("host", "127.0.0.1");
o.AddEmptySubArray("ports");
o["ports"].Add(80);
o["ports"].Add(443);
```

> `Add(bool, bool)` 的第二个参数是无实际含义的占位符，仅用于和 `Add(int)` 重载区分。`Replace(bool, bool)` 同理。

**语义区别（易踩坑）：**

| 方法 | 行为 |
|------|------|
| `Add(key, val)` | key 已存在则**失败返回 false**（不覆盖） |
| `Replace(key, val)` | key 不存在则**失败返回 false**（不新增） |
| `ReplaceAdd(key, val)` | key 存在则替换，不存在则新增（推荐日常使用） |
| `ReplaceWithNull(key)` | 把值替换为 null |

```cpp
cppJSON o("{\"a\":1}");
o.Add("a", 2);           // false，key "a" 已存在
o.Replace("b", 2);       // false，key "b" 不存在
o.ReplaceAdd("a", 2);    // true，替换为 {"a":2}
o.ReplaceAdd("c", 3);    // true，新增 {"c":3}

o.Delete("a");           // 删除 key
```

### 6.3 遍历对象的所有 key

```cpp
cppJSON o("{\"a\":1,\"b\":2,\"c\":3}");
std::string strKey;
o.ResetTraversing();                 // 游标复位
while (o.GetKey(strKey))
{
    std::cout << strKey << " ";      // a b c
}
```

> 注意：`GetKey()` 首次调用前必须 `ResetTraversing()`。`Add`/`Delete` 会重置游标到根，遍历顺序为内部链表顺序，**不是** JSON 源码顺序。

---

## 7. 数组（Array）操作

数组形如 `[v1, v2, ...]`。

### 7.1 读取

```cpp
cppJSON o("[10, 20, 30]");

int n = o.GetArraySize();        // 3

int v = 0;
o.Get(0, v);                     // 10
o.Get(2, v);                     // 30

std::string s = o(1);            // "20"（字符串表示）

// 嵌套数组 / 对象数组
cppJSON arr("[{\"x\":1},{\"x\":2}]");
int x = 0;
arr[0].Get("x", x);              // 1
```

### 7.2 增 / 改 / 删

```cpp
cppJSON arr;
arr.Add(1);                      // 追加数字
arr.Add(std::string("str"));     // 追加字符串
arr.Add(3.14);                   // 追加浮点
arr.Add(true, false);            // 追加布尔（第二个参数为占位）
arr.AddNull();                   // 追加 null

arr.AddAsFirst(0);               // 头插
arr.AddNullAsFirst();            // 头插 null

arr.Replace(0, 100);             // 替换第 0 个
arr.ReplaceWithNull(1);          // 第 1 个替换为 null
arr.Delete(2);                   // 删除第 2 个
```

`Add` / `AddAsFirst` / `Replace` 支持的类型与对象侧一致：`cppJSON`、`std::string`、`int32`、`uint32`、`int64`、`uint64`、`bool`、`float`、`double`。

### 7.3 顺序遍历数组（推荐，O(n)）

`Get(int i, ...)` 内部是 cJSON 链表 O(n) 查找，**用下标循环遍历总代价 O(n²)**。大数组建议用顺序遍历接口：

```cpp
cppJSON arr("[1,2,3,4,5]");

int64 v = 0;
arr.ResetArrayTraversing();          // 游标定位到第一个元素
while (arr.GetNextValue(v))          // 依次取每个元素
{
    std::cout << v << " ";           // 1 2 3 4 5
}
```

`GetNextValue` 支持全类型：`std::string`、`int32`、`uint32`、`int64`、`uint64`、`float`、`double`、`cppJSON`（取嵌套子对象）。

实测（10 万元素数组）：

| 方式 | 单元素耗时 | 复杂度 |
|------|-----------|--------|
| `for(i) Get(i, v)` | ~238 us | O(n²) |
| `while(GetNextValue(v))` | ~4.6 ns | O(n) |

**注意事项：**

- 遍历前必须先 `ResetArrayTraversing()`。
- `GetNextValue` 返回 `false` 表示遍历结束，**或**当前元素与目标类型不匹配（此时游标已前进）。
- 同一数组可多次遍历，每次重新 `ResetArrayTraversing()`。
- 小数组（几十个元素）用 `Get(i)` 即可，差异可忽略。

---

## 8. STL 容器支持（C++11）

可直接与 STL 容器互转，无需手动逐元素转换：

```cpp
std::vector<int> v{1,2,3};
o.Add("arr", v);                 // {"arr":[1,2,3]}
std::map<std::string,int> m{{"a",1},{"b",2}};
o.Add("obj", m);                 // {"obj":{"a":1,"b":2}}

o["k"] = v;                      // 赋值语法同样适用
o.Add(v);                        // 数组追加：数组元素是一个数组 [ [1,2,3], ... ]
```

读取：

```cpp
std::vector<int> v2;
o.Get("arr", v2);                            // 数组 -> vector
std::map<std::string,int> m2;
o.Get("obj", m2);                            // 对象 -> map
auto v3 = o["arr"].ToVector<int>();          // 便捷转换
auto m3 = o["obj"].ToMap<std::string,int>();
```

**支持的容器：**

| 容器 | JSON 形态 |
|------|----------|
| `std::vector` / `std::list` / `std::deque` / `std::set` | 数组 |
| `std::map` / `std::unordered_map` | 对象（key 用字符串） |

**支持的元素类型**：所有标量（`int32`/`uint32`/`int64`/`uint64`/`float`/`double`/`bool`/`std::string`/`const char*`）、`cppJSON`，以及**嵌套容器**（如 `std::vector<std::map<std::string,int>>`、`std::map<std::string,std::vector<int>>`）。

**注意：**

- 读取时对象 key 需为字符串类型。
- 64 位整数元素精确往返（如 `std::vector<int64>`）。
- 需要 C++11（内部用 `<type_traits>` 模板元编程）。

**API 一览**：`Add(key, container)`、`Add(container)`（数组追加）、`o[k] = container`、`Get(key, container&)`、`Get(i, container&)`、`ToVector<T>()`、`ToMap<K,V>()`。

---

## 9. 64 位整数支持（本扩展）

官方 cJSON 用 `double` 存所有数字（53 位精度），大整数会丢精度。本仓库给 cJSON 恢复了 `int64_t valueint` + `sign` 字段，**`int64`/`uint64` 全范围精确往返**。

```cpp
// 全范围精确
cppJSON o;
o.Add("min_i64", (int64)-9223372036854775807LL - 1);   // INT64_MIN
o.Add("max_u64", (uint64)18446744073709551615ULL);     // UINT64_MAX

std::string s = o.ToString();
// {"min_i64":-9223372036854775808,"max_u64":18446744073709551615}  —— 精确，无科学计数法

int64 v = 0;
o.Get("min_i64", v);             // 精确还原
```

### 数值类型判定规则

| 场景 | 内部行为 |
|------|---------|
| JSON 字面量无小数点/指数（如 `42`） | 按**整数**解析，`int64` 精确；超出 int64 范围按 `uint64` 处理 |
| JSON 字面量含小数点/指数（如 `3.14`、`1e5`） | 按**浮点**解析 |
| `Add` 传入 `int32/int64/uint32/uint64` | 按**整数**精确保存 |
| `Add` 传入 `float/double` | 按**浮点**保存；若值是整数值（如 `5.0`）打印为 `5` |

---

## 10. 错误处理

cppJSON **不抛异常**，所有操作通过返回值报告成败：

- 返回 `bool` 的方法：`true` 成功，`false` 失败。
- 失败原因存于内部，用 `GetErrMsg()` 读取（仅最近一次操作，字符串）。

常见失败场景：

| 场景 | 返回值 | 原因 |
|------|--------|------|
| `Parse` 非法 JSON | false | 语法错误，`GetErrMsg()` 给位置 |
| `Add` 但 key 已存在 | false | "key exists!" |
| `Add` 但当前是数组而非对象 | false | "not a json object! json array?" |
| `Get` 但 key 不存在 | false | 无 |
| `Get` 但类型不匹配 | false | 无 |

```cpp
cppJSON o;
if (!o.Add("x", 1))  std::cout << o.GetErrMsg();
o.Add("x", 2);       // 此时会失败
if (!o.Add("x", 2))  std::cout << o.GetErrMsg();   // key exists!
```

---

## 11. 性能指引

### 11.1 数组顺序访问

- **优先 `GetNextValue()` 顺序遍历**大数组（见 §7.3）。
- 按下标 `Get(i)` 适合小数组或随机访问。

### 11.2 批量 Add

`Add`/`Replace` 已利用 cJSON 的 `cJSON_bool` 返回值做校验（无多余 O(n) 二次遍历）。但对象 `Add` 的 **key 已存在检查**是 O(n) 的（语义需要）：

- 批量加**不同 key**：`Add` 每次 O(n) 查重，总 O(n²)。大对象批量构建建议直接操作底层或接受此成本。
- 批量加**数组元素**：`arr.Add(...)` 已是 O(1)/次，无此问题。

### 11.3 浮点序列化（Ryū）

浮点打印用 [Ryū](https://github.com/ulfjack/ryu) 最短精确算法（非 `%g`+`sscanf` 校验），`double` 序列化约 **10×** 提升，round-trip 逐位精确。实测：

| 数值 | 输出 |
|------|------|
| `0.1` | `0.1` |
| `3.141592653589793` | `3.141592653589793`（最短精确） |
| `1e20` | `1e20`（定点/科学自动选择） |
| `5e-324`（次正规） | `5e-324` |
| `-0.0` | `-0`（负零保留） |

### 11.4 主流 C++ JSON 库对照（1.2MB 负载，O2）

| 库 | Parse | Dump | 遍历嵌套 |
|----|-------|------|---------|
| **cppJSON** | 50 MB/s | **133 MB/s** | 611 ns/元素 |
| nlohmann/json | 40 MB/s | 132 MB/s | 244 ns/元素 |
| RapidJSON | 309 MB/s | 434 MB/s | 10 ns/元素 |
| simdjson | 763 MB/s | — | 1.7 ns/元素 |

定位：**构建**是强项（9 ns/元素，快过 RapidJSON）；**序列化**经 Ryū 优化后已达 nlohmann 水平；解析为 cJSON 经典解析器固有水平（约 nlohmann 相当，RapidJSON/simdjson 用 SIMD 更快）。

### 11.5 编译选项

- 嵌入式 / 无 FPU 平台：整数解析/打印走纯整数路径（`strtoll`/`%lld`），不依赖浮点库；仅浮点数字才触发 `strtod`/Ryū。
- 编译时建议 `-O2`。

---

## 12. 与官方 cJSON 的关系

本仓库 `cJSON.h` / `cJSON.c` 基于官方 **v1.7.19**，做了向后兼容的扩展（不影响官方 API 使用）：

| 扩展 | 说明 |
|------|------|
| `cJSON.valueint` | `int` → `int64_t`，支持精确 64 位整数 |
| `cJSON.sign` | 新增字段：`1`=无符号整数、`-1`=有符号整数、`0`=浮点 |
| `cJSON_CreateInt64()` / `cJSON_CreateUint64()` | 新增精确创建整数 API |
| `parse_number` / `print_number` | 纯整数字面量精确解析/打印 |
| `print_number` 浮点 | 集成 Ryū 最短精确算法（约 10× 浮点序列化提升） |

**如果你同时需要原生 cJSON API**，直接在代码中 `#include "cJSON.h"` 即可，`cJSON_Parse`、`cJSON_Print` 等官方函数全部可用。

---

## 附：完整方法速查

| 类别 | 方法 |
|------|------|
| 生命周期 | 6 种构造、析构、`operator=`、`operator==` |
| 赋值语法 | `o["k"] = 值`（int32/uint32/int64/uint64/float/double/bool/const char*/string/cppJSON）、`o[i] = 值` |
| 默认值读取 | `o["k"] \| 默认值`（ArduinoJson 风格）、`o.Get("k", 默认值)`（无副作用）、const `operator[]` |
| 解析/序列化 | `Parse` `ToString` `ToFormattedString` `GetErrMsg` |
| 通用 | `Clear` `IsEmpty` `IsArray` |
| 对象-读 | `operator[]` `operator()` `Get(key,...)` `KeyExist` `ValueType` `IsNull` |
| 对象-写 | `Add(key,...)` `Replace(key,...)` `ReplaceAdd` `AddNull` `ReplaceWithNull` `Delete` `AddEmptySubObject` `AddEmptySubArray` |
| 对象-遍历 | `GetKey` `ResetTraversing` |
| 数组-读 | `GetArraySize` `operator[]` `operator()` `Get(i,...)` `ValueType` `IsNull` |
| 数组-写 | `Add(...)` `AddAsFirst(...)` `AddNull` `AddNullAsFirst` `Replace(i,...)` `ReplaceWithNull` `Delete` |
| 数组-遍历 | `GetNextValue` `ResetArrayTraversing` |
| STL 容器 | `Add(key,容器)` `Add(容器)` `o[k]=容器` `Get(key,容器&)` `Get(i,容器&)` `ToVector<T>` `ToMap<K,V>` |
