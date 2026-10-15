# Review: Option / Result 值类别改动

- Date: 2026-10-14
- Scope: `src/option.cppm`、`src/result.cppm`，以及配套的 `src/string.cppm`、`tests/option.cpp`、`tests/result.cpp`、`tests/mutex.cpp`、`tests/string.cpp`。对照 `.agents/api_design.md`（2026-10-14 第三版，含路线 A）。
- Verdict: **`unwrap` / `expect` 的按值返回、`Result::ok` / `err` 不再对左值偷值、`or_` 的右值重置，这几处和路线 A 一致。** 问题 1、2 已修。其余落地和文档里的「已完成」对不上：`Result::as_ref` / `as_mut` 的返回类型编不过，`Result::or_` / `or_else` 改错了类型参数，多处 `noexcept` 会把用户异常变成 `terminate`。现有测试碰不到这些路径。

## P0

### 1. 主模板没有 take 家族的替代 API

状态：2026-10-14 已补到主模板 `Option<T>`（`unwrap_or` / `unwrap_or_else` / `unwrap_or_default`、`ok_or` / `ok_or_else`、`filter`、`is_some_and`、`contains`）。左值拷贝、右值移出并置 `None`；`ok_or` 用 `in_place_index` 区分 Ok/Err。覆盖见 `tests/option.cpp` 的 `UnwrapOr`、`OkOr`、`Filter`、`IsSomeAnd`、`Contains`。指针/引用特化原有实现未改（问题 5、8、12 仍在）。

`take` / `take_if` / `take_or_*` / `expect_take` 已从三个 `Option` 形态删除，`tests/option.cpp` 里对应用例也删了。`.agents/api_design.md` §8 步骤 5 写的是主模板、指针特化、引用特化都已补上 `unwrap_or` / `unwrap_or_else` / `unwrap_or_default`、`ok_or` / `ok_or_else`、`filter`、`map_or`、`is_some_and`、`contains`。

主模板 `Option<T>`（`src/option.cppm` 约 L101–505）实际只有原来的组合子加上 `map_or`、`or_else`、`as_ref`、`as_mut`。下面这些只出现在指针特化（约 L753 起）和引用特化（约 L1188 起）：

| API | 主模板 | 指针特化 | 引用特化 |
| --- | :-: | :-: | :-: |
| `unwrap_or` / `unwrap_or_else` / `unwrap_or_default` | 无 | 有 | 有，见问题 5 |
| `ok_or` / `ok_or_else` | 无 | 有 | 有，返回类型非法，见问题 5 |
| `filter` / `is_some_and` / `contains` | 无 | 有 | 有 |
| `map_or` | 有 | 有 | 有 |

文件开头 L14–20 的注释还在讲「下面的 deduction guide 是为了 `Option::ok_or`」，后面只有 `Result` 的前向声明，没有 deduction guide，也没有 `ok_or`。这组 API 是在主模板上写了一半又丢了。

`Option<int>` 上现在没有 `unwrap_or`，也没有 `filter`。删掉 `take_or_*` / `take_if` 之后，普通值类型没有替代写法。

### 2. 右值组合子把值移走之后，Option 仍是 Some

状态：2026-10-14 已修主模板。`map` / `and_then` / `map_or` / `map_or_else` 在右值回调返回后 `self = None{}`；`or_else` 先移出再置 `None`；`and_` 在右值 Some 分支丢弃内部值并置 `None`。左值路径不变。覆盖见 `tests/option.cpp` 的 `RvalueCombinatorsResetToNone`。指针/引用特化仍不消费（问题 12）。

§2 / §3.3 / DP-11：右值 self 把 `T&&` 交给回调，然后 `self = None{}`。`or_`（L312–331）做了。下面这些右值分支只 `std::get<1>(std::move(self))`，没有重置：

- `map` L232–234
- `and_then` L150–152
- `map_or` L296–298
- `map_or_else` L264–266

回调按值或按 `T&&` 取走对象之后，`std::move(opt).map(f)` 的 `opt` 仍然 `is_some()`，里面是 moved-from 的 `T`。再 `unwrap` 不会 panic，拿到的是已移走的值。回调如果只接 `const T&`，值还在，但按已拍板的规则右值路径仍然应该变成 `None`。

`or_else`（L354–357）右值分支是 `return std::move(self)`，同样没有 `self = None{}`。`std::variant` 移后只保证「有效但未指定」，不是 `None`。

`and_`（L121–126）右值 Some 直接返回 `optb`，self 保持 Some。值没有被移走，后状态同样不符合 DP-11。

### 3. `Result::as_ref` / `as_mut` 的返回类型无法实例化

```96:118:src/result.cppm
[[nodiscard]] auto as_ref() const noexcept -> Result<const T&, const E&>
// ...
[[nodiscard]] auto as_mut() noexcept -> Result<T&, E&>
```

`Result` 只有一个实现，基类是 `std::variant<T, E>`。`std::variant` 不能装引用，`Result<const T&, const E&>` / `Result<T&, E&>` 一实例化就是非法类型。

`Option::as_ref` / `as_mut` 能工作，是因为有 `Option<T&>` 特化，内部存指针。`Result` 没有对应特化。路线 A 写的借用路径（`as_ref().unwrap()` / `as_mut().unwrap()`）在 `Result` 上走不通。`tests/` 里没有调用，所以现有测试发现不了。

### 4. `Result::and_` 拒掉正常用法，`or_` / `or_else` 改错了类型参数

Rust 的形状是：

- `and` / `and_then`：Ok 类型可以变，Err 类型不动
- `or` / `or_else`：Ok 类型不动，Err 类型可以变

`and_`（L63–80）的函数体是对的：Ok 返回 `res`，Err 把 `E` 装进 `Result<U, E>`。约束却是

```cpp
requires (std::convertible_to<T, U> || std::convertible_to<U, T>)
```

`T` 和 `U` 在 Ok 分支根本不互转。`Result<int, std::string>::and_(Result<std::string, std::string>)` 这种 `and` 的本来用途会被约束直接丢掉。

`or_`（L285–303）签名是 `Result<U, E> -> Result<U, E>`：改 Ok 类型、留下原来的 `E`。`Result<int, ErrA>::or_(Result<int, ErrB>)` 对不上这个签名。Ok 分支还要 `Result<U, E>(T(...))`，所以只有 Ok 类型碰巧能互转时才能编译。

`or_else`（L314–334）把回调锁成 `Result<typename U::type, E>`，同样是「Ok 类型可变、Err 类型冻结」。回调收到的永远是 `E&`（L333），右值 self 也不会把 `E&&` 交出去。Rust `or_else` 要的是 `FnOnce(E) -> Result<T, F>`。

`@brief` 和函数体还反了：`and_` 的 brief 描述的是 `or_`（Err 时返回 res），`@return` 才是函数体。`Option::and_`（L112–115）的 brief 和 `@return` 都写成了 `or_`（Some 时返回自己），函数体是 Some 时返回 `optb`。

### 5. 引用特化的 `unwrap_or_default` 和 `ok_or` 不能用

状态：2026-10-14 已删除引用特化的 `unwrap_or_default`。引用没有默认值，None 分支无法返回一个活过函数的引用。`ok_or` / `ok_or_else` 仍返回 `Result<T&>`，问题还在。

`Option<T&>::unwrap_or_default`（已删除）在 None 分支：

```cpp
return static_cast<T>(raw_type{});
```

`T` 是引用。

- `Option<int&>`：`static_cast<int&>(int{})` 不能把右值绑到非 const 左值引用，这一支让整个函数实例化失败。
- `Option<const int&>`：`static_cast<const int&>(int{})` 能编译，返回的引用绑在函数内的临时对象上，返回时临时对象已经销毁。`const int x = opt.unwrap_or_default();` 也是从未定义行为里拷贝。

`ok_or` / `ok_or_else`（L1239–1266）的返回类型是 `Result<T, E>`，`T` 在这个特化里是引用，于是落到问题 3：`Result<int&, E>` 非法。指针特化的 `ok_or` 返回 `Result<T*, E>`，这一条本身可以成立。

### 6. `std::hash<Option<T>>` 撞车，并且指针特化一用就编不过

```1438:1454:src/option.cppm
template<typename T>
    requires requires (const T& t)
    {
        { std::hash<T>{}(t) } -> std::convertible_to<size_t>;
    }
struct hash<crab_cpp::Option<T>>
{
    auto operator()(const crab_cpp::Option<T>& opt) const -> size_t
    {
        if (opt.is_none())
        {
            return std::hash<int>{}(0);
        }

        return std::hash<T>{}(std::get<1>(opt));
    }
};
```

三件事叠在一起：

- None 的哈希是 `hash<int>(0)`，也就是 0。`std::hash<int>` 对 0 也是 0，所以 `Option<int>{}` 和 `Option<int>{0}` 的哈希永远相同。§6.4 要求 None/Ok/Err 带判别式再和值哈希混合。`Result` 的 `std::hash`（`src/result.cppm` L656–664）已经这么做了。
- 这个偏特化不区分主模板和指针特化。`std::hash<int*>` 存在，于是 `std::hash<Option<int*>>` 会选中它，函数体里的 `std::get<1>` 要求 `std::variant`。`Option<int*>` 不是 variant，调用即硬错误。`unordered_set<Option<int*>>` 编不过。
- `std::hash<int&>` 不存在，`Option<int&>` 没有哈希。§6.4 要求三个形态都能做无序容器的 key，引用形态应哈希被引用的对象。

## P1

### 7. 组合子的约束按 `T&` 写，右值分支传入的是 `T&&`

`Option` 的 `map` / `and_then` / `map_or` / `map_or_else` 以及 `Result` 的 `map` / `and_then` / `map_err` / `map_or` 都用 `invoke_result_t<F, T&>`（或 `E&`）做默认模板参数和 `requires`。右值分支实际调用的是 `f(std::get<...>(std::move(self)))`，实参类别是 `T&&`。

- 只接 `T&` 的 lambda 能通过约束，右值调用在函数体内失败，不是 SFINAE。
- 只接 `T&&` 或按值接 move-only `T` 的 lambda，在 `invoke_result_t<F, T&>` 这一步就被丢掉。`std::move(opt).map([](std::unique_ptr<int> p) { ... })` 没有候选。
- 能同时走过左右值的，是 `auto&&`、`const T&`，以及可拷贝类型的按值参数。

`Result` 还有对称的另一支忘了移：

- `map` 的 Err 支（L244）和 `and_then` 的 Err 支（L53）是 `return std::get<1>(self)`。`self` 是参数名，表达式永远是左值，`E` 被拷贝。`E` 为 move-only 时，Ok 支即使改对了，整个函数仍然编不过。
- `map_err` 的 Ok 支（L274）同样 `return std::get<0>(self)`，move-only 的 `T` 让 `map_err` 编不过。

右值分支要在 `if constexpr` 里把约束也拆开，Err/Ok 的另一支用 `std::move`。

### 8. `noexcept` 仍然会把用户异常变成 `terminate`

§3.7 要修的就是这类谎报。函数被标成 `noexcept` 之后，回调或转换抛出的异常在边界上变成 `std::terminate`。

无条件 `noexcept`、函数体却调用用户代码：

| 位置 | 调用 |
| --- | --- |
| 指针 / 引用 `or_else`（option.cppm L690、L1126） | `f()` |
| 指针 / 引用 `unwrap_or_else`（L771、L1206） | `f()` |
| 指针 / 引用 `filter`（L834、L1277） | `pred(...)` |

同文件里的 `is_some_and` 没有标 `noexcept`，谓词同样可能抛。`filter` 多出来的 `noexcept` 是写错了。

`Result::unwrap_or_else`（L464–479）的 `noexcept` 只看 `T` 的拷贝和移动。`T` 为 `int` 时条件为真，`return T(f())` 里 `f()` 抛异常仍会 terminate。`unwrap_or`（L435）没有把 `is_nothrow_constructible_v<T, U>` 算进去；`unwrap_or_default`（L492）没有把 `is_nothrow_default_constructible_v<T>` 算进去。

`Result::and_`（L66）的 `noexcept` 只看 `E`。Ok 分支 `return res` 要移动 `U`，`U` 的移动抛异常时同样 terminate。`or_`（L288）只看 `T`，`return res` 还要移动 `Result<U, E>`。

### 9. `operator<=>` 的返回类型写成了 `std::strong_ordering`

主模板 `Option`（L500–504）和 `Result`（L630–634）都 `requires std::three_way_comparable<...>`，返回类型写死 `std::strong_ordering`，函数体把底层 `<=>` 直接返回。

`std::three_way_comparable<double>` 为真，但 `std::variant` 的 `<=>` 得到的是 `std::partial_ordering`。`partial_ordering` 不能转换成 `strong_ordering`。`Option<double>`、`Result<double, int>` 的 `<=>` 在调用时实例化失败。引用特化（L1401–1419）对值的比较有同样的问题；None/Some 的判别式比较本身可以是 `strong_ordering`，值和判别式要取公共比较类别。

指针特化（L936–939）返回 `lhs.ptr <=> rhs.ptr`。不指向同一数组的指针，`<=>` 的结果是未指定的，`Option<T*>` 的顺序没有可用的全序。

### 10. `unzip` 对左值也是消费

§3.8：左值拷贝两个元素、self 不变；右值移出并置 `None`。主模板 `unzip`（L457–480）不是 deducing this，Some 分支无条件 `std::forward` 移出元素再 `*this = None{}`。

`tests/option.cpp` 的 `OptionTest.Unzip` 断言左值 `unzip` 之后 `some.is_none()`。测试把和 §3.8 相反的行为钉死了。`noexcept` 也只检查了移动构造，和「左值要拷贝」对不上。

### 11. `inspect` 把右值降成左值，存下来的引用仍然悬垂

§3.4 把 `inspect` 改成返回 `Self&`，并认为 `auto&& x = make().inspect(f)` 的悬垂因此消失。

deducing this 在右值上把 `Self` 推成值类型，返回类型就是 `Option<T>&` / `Result<T, E>&`，绑的是参数 `self`。临时对象的生命周期不会因为「返回了指向参数的左值引用」而延长。`auto& x = Option<int>(1).inspect(f);` 在完整表达式结束时悬垂，和改之前一样。

链式调用还把右值变成左值。`Option<std::unique_ptr<int>>(...).inspect(f).unwrap()` 里 `unwrap` 走左值拷贝，move-only 的 `T` 直接编不过。可拷贝类型则会多一次拷贝，并且后续组合子不再走右值重置。完整表达式里的链式调用只保证临时对象活到分号，不保证后面的调用仍是消费。

### 12. 指针 / 引用特化没有跟着值类别规则走

§2.5：指针特化右值消费要把 `ptr` 置空；引用特化右值同样置空。`unwrap` / `expect` / `unwrap_unchecked` 做了。新补上的 `or_`、`or_else`、`filter`、`map`、`and_then`、`map_or`、`unwrap_or` 都不是 deducing this，右值调用和左值一样，源对象保持 Some。

`std::move(ptr_opt).filter(pred)` 在谓词为假时返回 None，`ptr_opt` 里的指针还在。主模板 `or_` 会重置，两个特化的 `or_`（L672、L1107）不会。

指针特化 `is_none` / `is_some` 的 `@brief` 和 `@return` 是反的（L583–598）：`is_none` 的 brief 写成「指针非空时返回 true」。`@return` 那一行是对的。

## P2

### 13. 文档和代码互相矛盾，代理会按错的文档改

- `.agents/codeStyle.md` 被模板替换过：`crab_cpp` 全部变成了 `module_name`，示例模块也变成了 `module_name:sub1`。`AGENTS.md` 要求严格遵循这份风格。`api_design.md` §8 步骤 1 写过「若被模板覆盖需先恢复」，勾成完成时文件仍是替换后的内容。命名示例里还留着已经删除的 `expect_take`。
- `api_design.md` §3.1 的代码样例仍是「左值 `unwrap` 返回 `T&`」。§2 和 §8 步骤 3 的路线 A 已经改成一律按值返回，实现走的是路线 A。按 §3.1 回改会把 `MutexGuard` 的迁移（`as_mut().unwrap()`）再弄坏。
- `.agents/codeStructure.md` 的 `Option` 方法列表仍包含 `take` / `take_if` / `take_or_*`，`Result` 仍包含 `replace`。

### 14. 新契约没有测试

`tests/option.cpp` 删除了 `Take`、`TakeIf`、`TakeOrDefault`、`TakeOrElse`、`ExpectTake`，`tests/result.cpp` 删除了 `Replace`。仓库里的测试没有覆盖：

- `unwrap_or*`、`ok_or*`、`filter`、`is_some_and`、`contains`
- 右值 `map` / `and_then` 之后源对象必须是 `None`
- `Result::as_ref` / `as_mut`
- `operator==` / `operator<=>` / `std::hash`
- move-only `T` / `E` 的右值组合子
- `noexcept` 边界（回调抛异常时应传播，而不是 terminate）

`OptionTest.Unzip` 目前保护的是「左值也被掏空」。问题 1–9 都碰不到现有断言。

### 15. `replace` 先移出旧值再构造新值

主模板 `replace`（L375–380）：先 `auto temp = std::move(std::get<1>(*this))`，再 `*this = T(std::forward<U>(t))`。新值的构造或赋值抛出时，`temp` 在栈展开中销毁，`*this` 留下 moved-from 的 `T`，旧值丢了。先在局部把新的 `T` 构造出来，再移出旧值，抛异常时 `*this` 可以保持原值。

## 核对过、和设计一致的部分

- 主模板 `unwrap` / `expect` / `unwrap_unchecked` 经 `internal::unwrap_impl` 按值返回：左值拷贝，右值移出并 `self = None{}`。指针特化右值置 `nullptr`，引用特化右值置空后返回原来的引用，不再返回指向成员槽位的 `T&`。
- `Result::ok` / `err` 左值拷贝、右值移出，左值不再被无条件 `std::move` 掏空。
- `Result::replace` 已删除。`operator""_S` 改为 `std::move(...ok()).expect(...)`，和按值 `expect` 一致。
- `tests/mutex.cpp` 里 move-only 的 `MutexGuard` 改为 `as_mut().unwrap()`，和路线 A 的借用路径一致。
- `tests/string.cpp` 去掉 `capacity()` 相等断言，是因为右值 `unwrap` 现在真的移走了 `String`，不再靠拷贝留下相同容量。

## 建议的修复顺序

1. 把问题 1 的 P0 API 补回 `Option<T>` 主模板，引用特化不要返回 `Result<T&>` 或指向临时对象的引用（问题 5）。
2. 右值组合子在回调返回后置 `None`（问题 2）；`Result` 的另一支改为 `std::move`，约束按值类别拆开（问题 7）。
3. `Result::as_ref` / `as_mut` 需要能存放引用的表示，或者先不要提供这条 API（问题 3）。在此之前，路线 A 的借用写法只对 `Option` 成立。
4. `Result::or_` / `or_else` 改成保留 `T`、替换 `E`；`and_` 去掉 `T`/`U` 可转换约束（问题 4）。
5. 按问题 8 收紧 `noexcept`，再补问题 6 的哈希和问题 9 的比较类别。
6. 用会失败的测试把上述契约钉住，再改 `unzip` 的左值语义（问题 10，要同步改 `OptionTest.Unzip`）。
