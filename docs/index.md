## sjson
 一个简易的，小型的 json 库。

 - 只需要添加单个头文件 `json.hpp` 即可使用（需要编译器支持 C++11 或以上）
 - 基于 `initializer_list` 的直观的初始化方法
 - 类似 STL 的访问
 - 自动类型调整（可禁用）

## 示例

### 初始化

如果你需要通过下面的 json 对象来初始化

```json
{
    "string": "abcdefg",
    "number_integer": 114514,
    "number_double": 1919.810,
    "boolean": false,
    "null": null,
    "array": [
        1,
        2,
        3
    ]
}
```

您可以这样做：

```c++
json x;
x["string"] = "abcdefg";
x["number_integer"] = 114514;
x["number_double"] = 1919.810;
x["boolean"] = false;
x["null"] = nullptr;
x["array"] = {1, 2, 3};
```

#### 使用 \{...}

像初始化 `vector`/`map` 那样：

```c++
json x =
{
    {"string", "abcdefg"},
    {"number_integer", 114514},
    {"number_double", 1919.810},
    {"boolean", false},
    {"null", nullptr},
    {"array", {1, 2, 3}}
};
/*
* 如果提供的列表最外层每一项都是大小为 2 的 array 且
* 每一个 array 的第一项为 string 则该列表会被识别成
* object
*/
```

如果您想表达特殊情况（如满足 object 识别条件的数组）请使用 `json::array` 来标识：
```c++
json arr = json::array{{"string", "abcdefg"}};
```

#### 从字符串读取

使用 sjson 定义的字面量 `_json` 作为后缀：
```c++
json j = "{\"string\": \"abcdefg\"}"_json;
// 也可以使用原始字符串
json j2 = R"__(
    {
        "string": "abcdefg"
    }
)__"_json;
```
注意：如果没有 `_json` 后缀，则会被当成 string 类型的变量存储

也可以使用 `json::parse` ：

```c++
json j = json::parse(R"__({"string": "abcdefg"})__");
```

### 序列化

使用 `dump` 来获取 json 对象序列化后的字符串。

` void dump(std::string& dest, const std::string& tab = "  ", int deep = 0)const` ： 
|参数|说明|
|-|-|
|`dest`|指示输出目标|
|`tab`|格式化时使用的 tab 的内容（如果传入空串则不进行格式化）|
|`deep`|指示该节点在根节点中的深度（指示需要在前面加的 tab 数）|

`std::string dump(const std::string& tab="  ")const` ： 上一个函数的简化版本


### 类似 STL 的访问

事实上，`json::object` 和 `json::array` 分别继承自 `std::unordered_map` 和 `std::vector` 。

```c++
```
