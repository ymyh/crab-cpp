# API Design — Option / Result 语义对齐审查

> 状态：2026-10-14 第三版。第七章全部决策点已拍板（✅ 并写入正文），
> 无阻塞的开放问题；仅 `Ok` / `Err` 构造器的两种形态留待实现时二选一
> （6.3，P2）。本文档是 `Option<T>` / `Result<T, E>` 与 Rust `std` 语义
> 的对齐审查与修正计划；通用公共 API 设计原则待后续补充。

## 1. 背景与目标

`Option<T>` / `Result<T, E>` 的 API 面按 Rust 语义设计，但若干方法在
C++ 化的过程中语义发生偏移（`unwrap` 恒返回左值引用、`take` 返回 `T`
且 panic 等）。目标：以 Rust `std` 语义为基准修正不对齐之处、补齐重要
缺失 API。优先级：P0 = 正确性问题或最常用的缺失 API；P1 = 常用但可
绕过；P2 = 语法糖与生态整合。

## 2. 所有权与值类别总约定（统一值类别规则）

C++ 没有生命周期检查器，Rust 的"消费式 `self` + 编译器禁止后续使用"
无法照搬。crab_cpp 的对应物是**统一值类别规则**（✅ DP-1 修订版 /
DP-10 / DP-11）：**self 的值类别表达借用或消费意图**，与
`auto x = vec[0];`（拷贝）vs `auto x = std::move(vec[0]);`（移动）的
标准库惯例一致。

1. **左值 self（含 const）→ 借用**：组合子回调拿 `T&` / `const T&`；
   返回新对象的方法（`ok` / `or_` / `map` …）按需拷贝构造，**self 保持
   不变**。`unwrap` 家族（`unwrap` / `expect` / `unwrap_unchecked` /
   `unwrap_or` / `unwrap_or_else` / `unwrap_or_default`）例外：全部
   **按值返回**（左值拷贝、右值 move），与 Rust 消费语义一致，同族
   签名统一。左值想不拷贝地借用内部值，走 `as_ref()`（→
   `Option<const T&>`）/ `as_mut()`（→ `Option<T&>`）后 `.unwrap()`
   得引用，或 match / `std::get`。
2. **右值 self → 消费**：访问器在函数内部 move-out 并**返回 `T` 按值**
   （NRVO）；组合子回调拿 `T&&`；Option 消费后 **self 重置为 `None`**
   （✅ DP-11a，确定性后状态）；`Result` 没有空状态，只能留 moved-from，
   `@note` 标注。
   - **实现硬约束**：右值路径不能写成 `decltype(auto)` 纯转发
     （`return std::get<1>(std::forward<Self>(self))` 返回 `T&&`，move
     实际发生在调用方，函数内没有安全的时机执行重置——先重置则返回
     悬垂引用）。必须 `if constexpr` 按值类别分路，move 与重置都发生在
     函数内部，见 3.1 的模式。
3. **显式放弃所有权**：用户写 `std::move(opt).unwrap()` 表示消费
   （对应 Rust 的 move-out）；`opt.unwrap()` 表示借用。不产生
   `auto b = a.map(f);` 隐式窃取 `a` 的惊吓。
4. **panic 约定**：`unwrap*` / `expect*` 家族 panic；组合子与
   `unwrap_or*` 家族永不 panic。
5. **指针特化**：指针拷贝即借用，`unwrap` / `expect` 一律返回 `T` 按值；
   左值不置空（借用），右值置 `ptr = nullptr`（消费）。
   **引用特化**：`T = U&`，左值返回 `U&`（self 不动）；右值返回 `U&`
   并置 `ptr = nullptr`。

## 3. `Option<T>` 主模板 — 语义不对齐清单

### 3.1 `unwrap` / `expect` / `unwrap_unchecked`（✅ DP-1 修订版）

**决定**：按统一值类别规则——左值返回引用，右值 move-out 返回 `T`
按值并重置 self 为 `None`（✅ DP-11）。

```cpp
template<typename Self>
[[nodiscard]] auto unwrap(this Self&& self)
    -> /* 左值 self → T&（const self → const T&）；右值 self → T */
{
    if (self.is_some())
    {
        if constexpr (std::is_lvalue_reference_v<Self>)
        {
            return std::get<1>(self);              // 借用：T&
        }
        else
        {
            T tmp = std::get<1>(std::move(self));  // 消费：move-out
            self = None{};                          // DP-11：重置
            return tmp;                             // 按值（NRVO）
        }
    }

    panic("Calling Option<T>::unwrap() on a None value");
}
```

要点：

- 返回类型用 `if constexpr` 配套的条件返回类型（或等价 trait）实现；
  声明处需区分 const 左值（`const T&`）与非 const 左值（`T&`）。
- `noexcept` 相应条件化：左值路径返回引用基本不抛；右值路径
  `noexcept(std::is_nothrow_move_constructible_v<T>)`。
- `expect(msg)` 同一模式；`unwrap_unchecked`（✅ DP-3）改为
  `std::get_if` + 解引用（`None` 为 UB，`@warning` 标注），同样按值
  类别分路。
- 迁移影响：README L126-127 `int num = opt.unwrap();`（左值借用）不变；
  `tests/string.cpp` 的 `String::from(...).unwrap()`（右值）从"靠拷贝
  碰巧成立"变为真正的 move ✓；唯一的破坏是旧代码若把右值 unwrap 的
  结果当左值引用用（本来就悬垂，属既有 bug）。

### 3.2 take 家族删除（✅ DP-2 / DP-7）

**删除**：`take`、`take_if`、`take_or_default`、`take_or_else`、
`expect_take`（主模板与两个特化同步删除）。

理由（已确认）：Rust 需要 `take` 是因为 `unwrap` 消费 self，`&mut
Option` 无法调用，只能 take 先把借用转成所有权；C++ 的借用/消费由值
类别表达（`opt.unwrap()` 借用、`std::move(opt).unwrap()` 消费），
take 家族整体冗余。替换映射：

| 原用法 | 替换 |
| --- | --- |
| `take().unwrap()` | `std::move(opt).unwrap()` |
| `take().unwrap_or(d)` / `take_or_default()` / `take_or_else(f)` | `std::move(opt).unwrap_or(d)` / `unwrap_or_default()` / `unwrap_or_else(f)`（P0 补齐） |
| `take_if(pred)` | `filter(pred)`（P0 补齐，Rust 语义） |
| `expect_take(msg)` | `std::move(opt).expect(msg)` |
| "self 变为 None"后条件 | 右值消费路径的重置（DP-11） |

唯一未完全覆盖的模式：**从左值取出整个 `Option<T>` 同时 self 置
None**（take() 形态本身）。部分替代：`opt.replace(new)` 取出旧值的
`Option<T>`（代价是必须提供新值）。已接受的权衡。

### 3.3 组合子（✅ DP-10：与 unwrap 同一逻辑）

`map` / `and_then` / `map_or_else` / `or_else` / `filter` 及 `Result`
的 `map` / `map_err` / `and_then` 全部按统一值类别规则：

- 左值 self：回调拿 `T&`，self 保持不变（现有用法零破坏）；
- 右值 self：回调拿 `T&&`，随后 self 重置为 `None`（Option）/
  moved-from（Result）。

```cpp
// map 的右值路径示例
if constexpr (/* 右值 */)
{
    auto result = f(std::get<1>(std::move(self)));
    self = None{};
    return result;
}
```

`take_if` 传 `T&` 是正确的这一性质由 `filter` 继承（Rust `filter`
谓词为 `FnOnce(&T)`）。`inspect` 是唯一例外：恒为借用（3.4）。

### 3.4 `inspect`（✅ DP-4a）

Option 与 Result 统一返回 `Self&`（借用式副作用探针，对 Rust 的
**有意偏离**——Rust 的 `inspect` 消费并返回 `Self`）：

```cpp
template<typename F, typename Self>
    requires std::invocable<F, T&>
auto inspect(this Self&& self, F&& f) -> Self&
{
    if (self.is_some())
    {
        f(std::get<1>(self));
    }

    return self;
}
```

`self` 是有名参数，返回 `Self&` 恒合法；完整表达式内链式调用安全
（旧 `Self&&` 版本的 `auto&& x = make().inspect(f);` 悬垂与旧 Option
版本的整份拷贝问题一并消除）。

### 3.5 `and_` / `or_`（✅ DP-5，按统一规则修订）

返回 `Option<T>` 按值（无法条件返回引用），按值类别决定来源：

- 左值 self：**拷贝** self（或 optb）进结果，self 保持不变；
- 右值 self：move 进结果并重置 self 为 `None`。

```cpp
template<typename Self>
[[nodiscard]] auto or_(this Self&& self, Option<T> optb) -> Option<T>
{
    if (self.is_some())
    {
        return std::forward<Self>(self);  // 左值拷贝、右值 move（+重置）
    }

    return optb;
}
```

`and_` / `or_else` 同理。与 Rust（无条件消费）的偏离在 Doxygen
`@note` 标注。

### 3.6 `replace` 参数改万能引用（✅ 用户决定）

比按值省一次 move，且额外接受可转换到 `T` 的实参（如
`Option<String>::replace("literal")`）：

```cpp
template<typename U>
    requires std::convertible_to<U&&, T>
auto replace(U&& t) noexcept(std::is_nothrow_assignable_v<T&, U&&>) -> Option<T>
```

取旧值返回 `Option<T>` 保持不变（对齐 Rust）。指针 / 引用特化仍按值收
`T`（指针 trivially copyable，无转发收益）。`replace` 同时是"取出旧值
的 owned `Option<T>`"这一模式的逃生口（见 3.2）。

### 3.7 `noexcept` 谎报

`ok` / `err` / `unzip` / `replace` / `unwrap_or*` 等内部构造 `T` 可能
抛出却标了 `noexcept` → 直接 `std::terminate`。统一改为条件 noexcept
（右值/拷贝路径 `noexcept(std::is_nothrow_move_constructible_v<T>)`
等）。3.1 的左值引用路径可保留较强 noexcept。

### 3.8 其他

- 查询与转换类函数补 `[[nodiscard]]`（含引用特化的 `is_some` /
  `is_none`）。
- `unzip`（主模板）纳入统一值类别规则：左值拷贝两元素、右值 move 两
  元素并重置 self 为 `None`。与 Rust（无条件消费）的偏离 `@note`
  标注。

## 4. 指针 / 引用特化

### 4.1 指针特化 `expect` 的引用推导问题

```cpp
template<typename Self>
[[nodiscard]] auto expect(this Self&& self, std::string_view msg) -> decltype(auto)
{
    if (self.is_some())
    {
        return self.ptr;   // ← 问题在这
    }
    panic(msg);
}
```

返回类型是 **`decltype(auto)` 而非 `T`**。`self.ptr` 是左值表达式，
decltype 推导为 `T&` —— 即 `int*&`，指向 **self.ptr 成员槽位**的引用，
不是指针值。后果：临时对象上调用且绑定到引用时悬垂
（`auto& p = Option<int*>{&x}.expect("m");`）；左值上返回的引用别名了
槽位（之后 `opt.replace(q)` 会改变该引用本身）。`auto p = ...`（按值）
恰好解引用拷贝出指针值，掩盖了签名问题；Doxygen `@return The pointer`
同样误导。✅ 统一值类别规则后自然修复：`unwrap` / `expect` 一律返回
`T` 按值（第 2 节第 5 条），左值不置空、右值置 `ptr = nullptr`。

（引用特化 `expect` 声明 `-> T`（`T = U&`），返回原对象引用 ✓ 正确，
仅需右值路径配套置空指针。）

### 4.2 `unzip` 补齐到特化（✅ 用户决定）

指针 / 引用特化在 **pointee / 被引对象是二元 tuple-like** 时提供
`unzip`（crab_cpp 扩展，Rust 借用形态无对应物）：

- `Option<std::pair<int, int>*>` → `(Option<int*>, Option<int*>)`，
  元素为 `addressof(pair.first)` / `addressof(pair.second)`；
- `Option<std::pair<int, int>&>` → `(Option<int&>, Option<int&>)`。

推荐语义：**借用、非消费** —— 结果是指向 pointee 成员的借用，消费
self（置空 ptr）没有意义，self 保持可用。这与主模板的值类别规则
（右值才消费）方向一致但永远走借用分支，差异在 Doxygen 中标注。
约束：`tuple_of_size_2<remove_pointer_t<T>>` /
`tuple_of_size_2<remove_reference_t<T>>`。

### 4.3 API 一致性矩阵（更新后）

| 方法 | 主模板 | 指针特化 | 引用特化 |
| --- | :-: | :-: | :-: |
| `is_some` / `is_none` | ✓ | ✓ | ✓ |
| `unwrap` / `expect` | ✓ | ✓（`-> T` 修复 4.1） | ✓ |
| `unwrap_unchecked` | ✓ | ✓ | ✓ |
| `unwrap_or` / `unwrap_or_else` / `unwrap_or_default` | P0 补齐 | 补齐 | `unwrap_or` / `unwrap_or_else` 补齐；不提供 `unwrap_or_default`（引用没有默认值） |
| `map` | ✓ | ✓ | ✓ |
| `and_then` / `or_else` / `filter` | P0 补齐 | 补齐 | 补齐 |
| `map_or` / `map_or_else` | P0 补齐 / ✓ | 补齐 | 补齐 |
| `and_` / `or_` | ✓ | ✓ | ✓ |
| `inspect` | ✓ | ✓ | ✓ |
| `replace` | ✓ | ✓ | ✓ |
| `zip` / `unzip` | P1 补齐 / ✓ | `unzip` 补齐（借用式） | `unzip` 补齐（借用式） |
| `as_ref` / `as_mut` | P1 补齐 | —（本身即指针语义） | — |

（take / `expect_take` / `take_if` / `take_or_*` 各行已随删除消失。）

### 4.4 长期（P2）

用单一实现（mixin / CRTP 或统一存储 + 条件逻辑）消除三份手写拷贝 ——
特化与主模板的漂移正是第 4 节问题的根源。新增 API 从第一天起要求三个
特化同步交付。

## 5. `Result<T, E>` — 语义不对齐清单

### 5.1 `unwrap` / `expect` / `unwrap_err` / `expect_err`（✅ DP-1 修订版）

按统一值类别规则：`unwrap` / `expect` 左值返回 `T&`（const →
`const T&`）、右值 move-out 返回 `T`；`unwrap_err` / `expect_err` 对 `E`
同理。旧 `decltype(auto)` 方案的两分支类型收敛问题随条件返回类型消除。

**注意不对称性**：`Result` 无空状态，右值消费后 self 只能留 moved-from
（无法像 Option 重置为 `None`），`@note` 标注。

### 5.2 `ok()` / `err()`（既有隐患修复）

现状是**对左值也无条件 `std::move` 偷值**（`auto o = res.ok();` 掏空
左值 res 但 `is_ok()` 仍为 true）——隐藏惊吓，且与统一值类别规则矛盾。
**修正**：左值 → 拷贝构造 `Option<T>`（self 不变）；右值 → move 并将
self 置 moved-from。同时修 `noexcept` 谎报、补 `[[nodiscard]]`。
注意：既有代码若依赖"左值 `ok()` 偷值"的性能特征会多一次拷贝——
迁移时对照 `tests/` 评估。

### 5.3 组合子（✅ DP-10）

`map` / `map_err` / `and_then` 与 Option 的 3.3 同题同解（左值回调拿
`T&` / `E&`，右值拿 `T&&` / `E&&`）。`inspect` / `inspect_err` 回调恒
拿 `T&` / `E&` 与借用定位一致 ✓。

### 5.4 `inspect` / `inspect_err`（✅ DP-4a）

由 `Self&&` 统一改为 `Self&`，与 3.4 同。

### 5.5 `replace` 删除（✅ DP-6b）

Rust 的 `Result` 没有 `replace`，且现签名带 3.6 / 3.7 两处问题。直接
删除，不留 extension。

### 5.6 `Result<T, void>` 不支持（✅ DP-9）

不实现特化。文档化惯用法：unit 错误用 `Result<T, None>`（即
`variant<T, monostate>`，今天就能用，`is_err()` 即"持有 monostate"），
unit 值用 `Result<None, E>`。README 增加一节说明；可选提供别名
`template<typename E> using UnitResult = Result<None, E>;`（是否提供，
实现时定）。

## 6. 缺失功能清单（按优先级，更新后）

### 6.1 P0

| API | 归属 | 说明 |
| --- | --- | --- |
| `unwrap_or` / `unwrap_or_else` / `unwrap_or_default` | Option, Result | 被删 take 家族的正式替换（值类别规则） |
| `ok_or` / `ok_or_else` | Option | Option → Result 桥梁，目前完全缺失 |
| `map_or` | Option, Result | `map_or_else` 已有，非闭包版缺失 |
| `or_else` | Option, Result | 配合 `or_` |
| `and_` | Result | Option 有、Result 没有 |
| `filter` | Option | 替代被删的 `take_if` |
| `is_some_and` / `is_ok_and` / `is_err_and` | Option, Result | 短路谓词 |
| `contains` / `contains_err` | Result | |
| `operator==` / `operator<=>` + `std::hash` | Option, Result | 见 6.4 | ✅ 已完成（2026-10-14，150 测试全绿）：hidden friend `==` / `<=>`（主模板委托 `std::variant`，指针特化比较指针值，引用特化比较被指对象），`std::hash` 特化（None/Ok/Err 区分 + 值哈希组合） |

### 6.2 P1

| API | 归属 | 说明 |
| --- | --- | --- |
| `as_ref` / `as_mut` | Option, Result | **降回 P1**：左值 `unwrap` 已提供借用路径；`as_ref` 的剩余价值是产出可传递的 `Option<T&>` 对象 |
| `get_or_insert` / `get_or_insert_with` / `insert` | Option | 惰性初始化族（Rust 1.83+ stable） |
| `zip` | Option | 与 `unzip` 对称 |
| `cloned` / `copied` | Option, Result | `Option<T&>` / `Option<T*>` → `Option<T>` |
| `flatten` | Option, Result | `Option<Option<T>>` → `Option<T>` |
| `unwrap_err_unchecked` | Result | 配合 DP-3 |
| 特化补齐 | Option 指针/引用 | 见 4.3 矩阵（含 4.2 的 `unzip`） |

### 6.3 P2

| API | 归属 | 说明 |
| --- | --- | --- |
| `iter` / `begin` / `end` | Option, Result | 支持 range-for |
| `transpose` | Option, Result | `Option<Result<T, E>>` ↔ `Result<Option<T>, E>` |
| `xor` | Option | 恰好一个 Some |
| `some(T)` 工厂 | Option 自由函数 | `some(5)` |
| `Ok` / `Err` 构造器 | Result | ✅ DP-8 已决定引入。两个方向：a) tag 类型 + 模板转换运算符，支持 `return Ok(5);`（依赖返回语境推导）b) `Ok<T>` / `Err<E>` 模板 tag（类比 `std::in_place_type`，`Ok<int>{5}`，更可控）。实现时二选一 |
| `std::formatter` | Option, Result | Rust `Debug` 对应物；string 模块已有先例 |
| 单一实现重构 | Option | 见 4.4 |

### 6.4 比较与哈希

主模板经 `std::variant` 基类的隐式转换"碰巧"能用 `==` / `<`，但指针 /
引用特化完全没有比较运算；`std::hash` 特化不存在（无法做
`unordered_map` 的 key）；依赖基类转换是脆弱的隐式语义。

**建议**：hidden friend 的 `operator==` / `operator<=>`（委托底层表示）
+ `std::hash` 特化，三个特化统一提供。

## 7. 决策点汇总（2026-10-14 全部拍板）

| # | 问题 | 决定 |
| --- | --- | --- |
| 1 | `unwrap` / `expect` 返回语义 | **统一值类别规则**：左值 self 返回 `T&`（const → `const T&`）借用；右值 self 在函数内 move-out 返回 `T` 按值（不能是 `T&&` 纯转发，否则无法安全重置） |
| 2 | take 家族 | **整体删除**（C++ 用值类别表达借用/消费，左值可直接借用 unwrap，无 Rust 的借用检查限制） |
| 3 | `unwrap_unchecked` | `get_if` + 解引用，`None` 为 UB |
| 4 | `inspect` 返回 | `Self&`（Option / Result 统一，作为借用式探针例外） |
| 5 | `and_` / `or_` | 按统一值类别规则修订：左值拷贝进结果（self 不变），右值 move + 重置 |
| 6 | `Result::replace` | 删除 |
| 7 | take_* 与 unwrap_or_* 并存 | 并入 2：take 删除，`unwrap_or*` 家族补齐 |
| 8 | `Ok` / `Err` 构造器糖 | 引入（P2；tag 形态二选一，见 6.3） |
| 9 | `Result<T, void>` | 不支持，文档化 `Result<T, None>` / `Result<None, E>` 惯用法 |
| 10 | 组合子 self 语义 | **与 unwrap 同一逻辑**（左值回调拿 `T&`，右值拿 `T&&`） |
| 11 | 消费操作的后状态 | Option 重置为 `None`；Result 无空状态，留 moved-from 并 `@note` 标注 |

## 8. 实施顺序与破坏性变更

按依赖与风险排序，每步可独立提交。统一值类别规则使整个迁移的破坏面
大幅缩小（左值用法全部保持不变）：

1. **`noexcept` / `[[nodiscard]]` 修正**（3.7 / 5.2）— ✅ 已完成
   （2026-10-14，GCC 16.2 下 156 个测试全绿）。`codeStyle.md` 若被模板
   覆盖需先恢复。
2. **组合子值类别分路**（3.3 / 5.3：`map` / `and_then` / `map_or_else`）
   — ✅ 已完成（2026-10-14，156 个测试全绿）。左值回调拿 `T&`，右值
   回调拿 `T&&`。Result 的 `and_then` / `map_err` 同题同解一并完成。
3. **`unwrap` / `expect` / `unwrap_unchecked` / `unwrap_err` /
   `expect_err` 重写**（3.1 / 5.1，含 4.1 指针特化修复）— ✅ 已完成
   （2026-10-14，156 个测试全绿）。新增 `internal::unwrap_ret_t` /
   `internal::unwrap_noexcept_v` / `internal::unwrap_impl`，按统一值
   类别规则重写：左值返回引用、右值 move-out + 置 None + 按值返回；
   指针特化 `expect`/`unwrap`/`unwrap_unchecked` 修复 `decltype(auto)`
   引用推导悬垂 bug（`-> T` 按值，右值置空）；引用特化右值置空。
   `internal::unzip_first/second` 更名为 `unzip_option_first/second`。
   **路线 A 重做**（2026-10-14 晚）：`unwrap` / `expect` /
   `unwrap_unchecked` / `unwrap_err` / `expect_err` 全部改为 `-> T` /
   `-> E` **按值返回**（左值拷贝、右值 move + 置 None），删除条件返回
   类型 `unwrap_ret_t`，与 `unwrap_or` 家族签名统一。左值借用改走
   `as_ref()`（→ `Option<const T&>`）/ `as_mut()`（→ `Option<T&>`），
   复用引用特化，None 安全不悬垂。`MutexGuard`（move-only）等左值
   `unwrap()` 的现有用法迁移为 `as_mut().unwrap()`。`Option` 和
   `Result` 均新增 `as_ref()` / `as_mut()`（Result 返回
   `Result<const T&, const E&>` / `Result<T&, E&>`）。
4. **`ok()` / `err()` 左值停止偷值**（5.2）— ✅ 已完成（2026-10-14，
   156 个测试全绿）。改为 deducing this + 值类别分路：左值拷贝、右值
   move + moved-from。修复左值无条件 `std::move` 偷值的隐藏 bug。
5. **P0 增量补齐**（6.1）— ✅ 已完成（2026-10-14，156 个测试全绿 +
   smoke 验证）。新增 API（全部三处：主模板 / 指针特化 / 引用特化，及
   Result）：`unwrap_or` / `unwrap_or_else` / `unwrap_or_default`（左值
   返回 `T` 按值拷贝、右值 move-out + 重置为 None）、`ok_or` /
   `ok_or_else`、`filter`、`or_else`、`and_`（Result）、`map_or`、
   `is_some_and` / `is_ok_and` / `is_err_and`、`contains` /
   `contains_err`。`Option` 的 `ok_or` / `ok_or_else` 经前向声明 +
   透明守卫解决与 `:result` 的循环依赖。**设计修正**：`unwrap_or` 三
   件套左值也返回 `T` 按值（左值 None 时返回左值引用会悬垂，C++ 无
   Rust 借用检查，与 `map_or` 左值语义一致）；右值路径 Some 时亦重置为
   None（DP-11 语义扩展至默认值族）。引用特化不提供
   `unwrap_or_default`：返回类型是引用，None 分支没有可返回的默认对象。
6. **take 家族 + `Result::replace` 删除**（3.2 / 5.5）— ✅ 已完成
   （2026-10-14，150 个测试全绿）。删除 `take` / `take_if` /
   `take_or_default` / `take_or_else` / `expect_take`（主模板 + 指针/
   引用特化）及 `Result::replace`；`string.cppm` 的 `_S` 字面量迁移为
   `std::move(...).expect(...)`；`tests/option.cpp` 删除 5 个 take 测试
   块、`tests/result.cpp` 删除 `Replace` 测试。**测试修正**：
   `StringTest.CopyConstructor` / `AssignmentOperator` 的
   `EXPECT_EQ(s1.capacity(), s2.capacity())` 删除——旧 `unwrap` 返回
   `T&` 时 `s1` 是拷贝构造（容量 16），新 `unwrap` 返回按值时 `s1`
   是移动构造（容量 5），拷贝构造 `s2` 走 `+=` 得容量 16，二者本就不
   相等（容量断言依赖旧的拷贝副作用）。
7. **`inspect` 统一为 `Self&`**（3.4 / 5.4）— ✅ 已完成（2026-10-14，
   150 个测试全绿）。指针特化 `inspect` 从 `Self`（拷贝返回）改为
   `Self&`；`Result::inspect` / `inspect_err` 从 `Self&&` 改为 `Self&`，
   消除 `auto&& x = make().inspect(f);` 悬垂与 Option 整份拷贝问题。
8. **`and_` / `or_` 修订、`replace` 转发引用**（3.5 / 3.6）— ✅ 已完成
   （2026-10-14，150 个测试全绿）。主模板 `or_` 改 deducing this +
   值类别分路（左值拷贝进结果、右值 move + 重置为 None）；`and_` 同步；
   `replace` 参数 `T&&` → 万能引用 `U&&`（`std::convertible_to<U&&, T>`
   约束），额外接受可转换实参。Result 的 `and_` / `or_` 步骤 5 已消费化，
   无需再改。
9. **特化补齐 + 特化 `unzip`**（4.2 / 4.3）— ✅ 特化补齐已完成
   （2026-10-14，150 个测试全绿）。指针 / 引用特化补齐 `and_then` /
   `map_or_else`，至此两特化覆盖主模板全部组合子。特化 `unzip`（
   借用式，tuple-like pointee）仍为开放项（P1 / 4.2）。
10. P1 / P2 项（`as_ref`、`Ok` / `Err`、formatter、单一实现重构等）
    按排期推进。

需要同步修订的用户可见文档：`README.md`（unwrap / take 语义示例、
`std::move(opt).unwrap()` 消费写法、`Result<T, None>` 惯用法一节）、
`.agents/codeStructure.md`（方法清单）、`tests/`（take 用例改写为
`std::move(opt).unwrap_or*`，`Result::replace` 用例删除）。
