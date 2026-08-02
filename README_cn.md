[English](/README.md) | 中文

[![License](https://img.shields.io/github/license/mashape/apistatus.svg)](LICENSE)

**cppJSON** 是一个轻量、易用的 C++ JSON 库，由 [neb::CJsonObject](https://github.com/Bwar/CJsonObject) 演进而来，底层基于 cJSON v1.7。主要特性：

- **精确 64 位整数**：`int64`/`uint64` 全范围精确解析、生成、读取
- **STL 容器互转**：`vector`/`map`/`list`/`set` 等 ↔ JSON，支持嵌套
- **map 风格赋值**：`o["k"] = value` 自动创建 key
- **ArduinoJson 风格默认值**：`o["k"] | defval`、`o.Get("k", defval)`
- **O(n) 数组遍历**：`GetNextValue()`、Ryū 快速浮点打印
- **RAII**：内存自动管理，无第三方运行时依赖，嵌入式友好

完整的 API 使用方法与示例见 [API_GUIDE.md](/API_GUIDE.md)。

---

## 特性

基于官方 cJSON v1.7 并做了增强：

| 特性 | 说明 |
|------|------|
| **64 位整数精确** | `int64`/`uint64` 全范围精确解析、生成、读取（官方 cJSON 用 `double` 存数会丢精度） |
| **赋值语法** | `o["k"] = value`（map 风格，key 不存在自动创建），支持嵌套 `o["a"]["b"] = v` |
| **默认值读取** | ArduinoJson 风格 `operator\|`、`o.Get("k", defval)` 无副作用读取 |
| **STL 容器** | `vector`/`list`/`deque`/`set` ↔ 数组、`map`/`unordered_map` ↔ 对象，支持嵌套容器 |
| **数组顺序遍历** | `GetNextValue()` O(n)（用 `Get(i)` 循环是 O(n²)，实测快 5 万倍） |
| **浮点打印 Ryū** | 最短精确算法，round-trip 逐位还原，比 `%g` 快约 10× |
| **轻量 RAII** | 无第三方依赖，内存自动管理，嵌入式友好 |

## 性能

对比主流 C++ JSON 库（1.2 MB 负载，`-O2`，MinGW x86_64）：

| 库 | Parse | Dump | 构建 | 遍历嵌套对象 |
|----|-------|------|------|-------------|
| **cppJSON** | **51** MB/s | **143** MB/s | **9** ns/元素 ✅ | 490 ns/元素 |
| nlohmann/json | 40 MB/s | 132 MB/s | 90 ns/元素 | 244 ns/元素 |
| RapidJSON | 309 MB/s | 434 MB/s | 16 ns/元素 | 10 ns/元素 |
| simdjson | 763 MB/s | — | — | 1.7 ns/元素 |

**✅ = 全场最快（构建 9 ns/元素，比 RapidJSON 快约 1.8×、比 nlohmann 快约 10×）**

**加粗** = 同档（经典逐字符解析器）对比中 cppJSON 占优：
- **Parse 51 MB/s > nlohmann 40 MB/s**
- **Dump 143 MB/s > nlohmann 132 MB/s**

定位：

- **构建**是强项：全场最快。
- **序列化**经 Ryū 优化后已超过 nlohmann。
- **解析**为 cJSON 经典解析器固有水平（优于 nlohmann；RapidJSON / simdjson 用 SIMD 批量解析更快）。
- 需 C++11 及以上（STL 容器、`operator|`、`nullptr` 特性依赖）。

基准代码可复跑，详见 [API_GUIDE.md](/API_GUIDE.md) §11。

## 项目来源

本项目（`cppJSON`）**派生自 [Bwar/CJsonObject](https://github.com/Bwar/CJsonObject)**（原 `neb::CJsonObject`），感谢原作者 Bwar 的开源贡献。

> **AI 改造声明（诚实披露）**：本项目由 **AI 编程工具（Claude Code）** 在 Bwar/CJsonObject 基础上改造完成。
> AI 承担了主要改造工作：类名更名 `cppJSON`、移除 `neb` 命名空间、新增 C++11 特性
> （精确 64 位整数、STL 容器互转、map 风格赋值、`operator|` 默认值读取、`GetNextValue` 数组遍历、
> Ryū 浮点打印）、重构构建流程（CMake 构建时拉取 cJSON v1.7.19 + Ryū 并应用补丁）。
> 原作者的代码结构与设计思路是本项目的基础，但改造后的代码主体由 AI 生成，
> 请使用者知悉。

相比原版的主要变更：

- 类名 `neb::CJsonObject` → 全局类 **`cppJSON`**（移除 `neb` 命名空间）
- 底层 cJSON 升级并扩展（精确 64 位整数、Ryū 浮点打印）
- 新增 STL 容器互转、map 风格赋值、`operator|` 默认值读取等特性
- 构建流程重构（CMake，构建时拉取 cJSON + Ryū，不再随仓库发布第三方源码）

## Fork 声明

本项目使用的 cJSON **fork 自 [DaveGamble/cJSON](https://github.com/DaveGamble/cJSON)（官方 v1.7.19）**，MIT 许可，保留原作者版权声明。

**cJSON 不随仓库发布**：构建时自动拉取官方 v1.7.19 并应用补丁 `patches/cJSON_v1.7.19_cppJSON.patch`（含全部扩展）。详见 [API_GUIDE.md](/API_GUIDE.md) §1。

在官方版本基础上做了**向后兼容的扩展**（不影响官方 API 使用）：

- `cJSON.valueint` 扩为 `int64_t`，支持精确 64 位整数（官方用 `double` 存数会丢精度）
- 新增 `cJSON.sign` 字段（整数有符号/无符号标记）
- 新增 `cJSON_CreateInt64()` / `cJSON_CreateUint64()` API
- 浮点打印集成 [Ryū](https://github.com/ulfjack/ryu) 最短精确算法（约 10× 提速）
- 解析器优化（数字字面量扫描等）

cppJSON 是 cJSON 之上的独立演进封装，二者可独立使用——本仓库的 `cJSON.c/h` 也可单独编译。本项目在 GitHub 上独立维护（不与 cJSON 的 issue/PR 混同）。

## 第三方依赖

| 依赖 | 许可证 | 用途 |
|------|--------|------|
| [cJSON](https://github.com/DaveGamble/cJSON) | MIT | JSON 解析 / 序列化核心 |
| [Ryu](https://github.com/ulfjack/ryu) | Apache-2.0 OR Boost-1.0 | double 最短精确格式化（构建时拉取，固定 commit `4c0618b`） |

完整许可文本见 [THIRD_PARTY_LICENSES.md](/THIRD_PARTY_LICENSES.md)。

