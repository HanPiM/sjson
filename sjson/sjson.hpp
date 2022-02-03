#pragma once

#include <string>
#include <vector>

#include <unordered_map>

#include <utility>

#include <functional>
#include <type_traits>

#define _SJSON_DISABLE_AUTO_TYPE_ADJUST

#if defined(_MSC_VER)
#define _HASCPP14 (_MSVC_LANG >= 201402L)
#else
#define _HASCPP14 (__cplusplus >= 201402L)
#endif

namespace sjson
{

namespace _private
{
// https://en.cppreference.com/w/cpp/types/conjunction
template<class...> struct conjunction : std::true_type { };
template<class B1> struct conjunction<B1> : B1 { };
template<class B1, class... Bn>
struct conjunction<B1, Bn...> 
    : std::conditional<bool(B1::value), conjunction<Bn...>, B1>::type {};
    
template <bool B>
using bool_constant = std::integral_constant<bool, B>;

// https://en.cppreference.com/w/cpp/types/negation
template<class B>
struct negation : bool_constant<!bool(B::value)> { };
}

class _ref_union
{
public:
    constexpr _ref_union() : _data(nullptr){};
    constexpr _ref_union(const _ref_union &x) : _data(x._data) {}
    template <typename T>
    inline constexpr void set(const T *x) { _data = (void *)x; };
    template <typename T>
    inline constexpr T *get() { return (T *)_data; }
    template <typename T>
    inline constexpr const T *get() const { return (T *)_data; }

private:
    void *_data;
};
class _fake_union
{
public:
    _fake_union() : _data(nullptr) {}
    _fake_union(const _fake_union &x)
    {
        _data = nullptr;
        _assign(x);
    }
    template <typename _t>
    _fake_union(const _t &x)
    {
        _data = nullptr;
        set(x);
    }
    const _fake_union &operator=(const _fake_union &x)
    {
        _assign(x);
        return x;
    }

    ~_fake_union()
    {
        _destroy_data();
    }

    inline bool empty() const
    {
        return _data == nullptr;
    }
    inline void clear()
    {
        _destroy_data();
        _data = nullptr;
    }

    template <typename _t>
    inline _t &get()
    {
        return *((_t *)_data);
    }
    template <typename _t>
    inline const _t &get() const
    {
        return *((const _t *)_data);
    }
    // 如果原先为空则创建一个新对象再返回
    template <typename _t>
    inline _t &as()
    {
        if (empty())
            set(_t());
        return get<_t>();
    }
    template <typename _t>
    inline const _t &as() const
    {
        if (empty())
        {
            static _t x;
            return x;
        }
        return get<_t>();
    }

    template <typename _t>
    void set(const _t &x)
    {
        _destroy_data();
        _destructor = [](void *p)
        { delete (_t *)p; };
        _copy_constructor = [](void *x)
        { return new _t(*((_t *)x)); };
        _data = new _t(x);
    }

private:
    void *_data = nullptr;
    std::function<void(void *)> _destructor;
    std::function<void *(void *)> _copy_constructor;

    void _destroy_data()
    {
        if (!empty())
            _destructor(_data);
        // 可以不设为空指针，因为该函数在内部使用，可确保调用后会赋值
    }

    void _assign(const _fake_union &x)
    {
        _destroy_data();
        _destructor = x._destructor;
        _copy_constructor = x._copy_constructor;
        if (x._data != nullptr)
            _data = _copy_constructor(x._data);
        else
            _data = nullptr;
    }
};

enum class json_type
{
    value,
    array,
    object
};
constexpr inline const char *json_type_name(json_type x)
{
    /* 为了兼容 c++11 。。。 */
    return x == json_type::array
                ? "array"
                : (x == json_type::object ? "object" : "value");
}
inline std::string _basic_error_info(
    int line, const char *func, const std::string &str)
{
    return std::to_string(line) + ':' + func + ':' + str;
};

class json_error
{
public:
    json_error(){}
    json_error(const char *str) : _msg(str) {}
    json_error(const std::string &str) : _msg(str) {}
    json_error(int line, const char *func, const std::string &str)
        : _msg(std::string("sjson.hpp:") + std::to_string(line) +
               ':' + func + ":\n  " + str) {}
    const char *what() const { return _msg.c_str(); }

private:
    std::string _msg;
};

#define _SJSON_TRY(expr)                                 \
    do                                                   \
    {                                                    \
        try                                              \
        {                                                \
            expr;                                        \
        }                                                \
        catch (const json_error &e)                      \
        {                                                \
            throw json_error(                            \
                __LINE__, __FUNCTION__,                  \
                std::string(#expr) + " -> " + e.what()   \
            );                                           \
        }                                                \
    } while (0)

#define _SJSON_THROW(str) throw json_error(__LINE__,__FUNCTION__,str)
/*
* 禁用意外类型的自动类型调整
* 如：对 array 对象使用 ["key"]
*   如果是 const array 则会返回空的 json
*   如果是 array 则会将该结点转换成 object 再返回
*/
#ifndef _SJSON_DISABLE_AUTO_TYPE_ADJUST
#define _SJSON_THROW_TYPE_ADJUST(dest, need) ((void)0)
#define _SJSON_THROW_TYPE_ADJUST_(dest, need) ((void)0)
#else
#define _SJSON_THROW_TYPE_ADJUST_RAW(dest, need) \
_SJSON_THROW(std::string("call method of json_type::") + need + " on json_type::" + dest)
#define _SJSON_THROW_TYPE_ADJUST(dest, need) \
_SJSON_THROW_TYPE_ADJUST_RAW(json_type_name(dest), json_type_name(need))
#endif

template <typename _string_type>
class json_base
{
private:
    class _my_initializer_list;
public:
    using string_t = _string_type;
    using string_char_t = typename string_t::value_type;

    class value
    {
    public:
        enum
        {
            _empty,

            null,
            number_double,
            number_integer,
            boolean,
            string,

            _basic_type_end
        };

        value() : _type(_empty), _data(nullptr) {}
        value(std::nullptr_t) { assign(nullptr); }
        value(bool x) { assign(x); }
        value(double x) { assign(x); }
        value(int x) { assign(x); }
        value(const string_char_t *x) { assign(x); }
        value(const string_t &x) { assign(x); }
        value(const value &x) { assign(x); }

        const value &operator=(const value &x)
        {
            assign(x);
            return x;
        }

        virtual ~value() = default;

        operator double() const { return as<double>(); }
        operator int() const { return as<int>(); }
        operator bool() const { return as<bool>(); }
        operator string_t() const { return as<string_t>(); }

#define _MAKE(needtype, needtypeval)               \
    template <                                     \
        typename _t,                               \
        typename std::enable_if<                   \
            std::is_same<_t, needtype>::value, int \
        >::type = 0                                \
    > needtype &as()                               \
    {                                              \
        ensure_is(needtypeval);                    \
        return _data.as<needtype>();               \
    }                                              \
    template <                                     \
        typename _t,                               \
        typename std::enable_if<                   \
            std::is_same<_t, needtype>::value, int \
        >::type = 0                                \
    > const needtype &as() const                   \
    {                                              \
        ensure_is(needtypeval);                    \
        return _data.as<needtype>();               \
    }                                              \
    void assign(needtype x)                        \
    {                                              \
        _type = needtypeval;                       \
        _data = x;                                 \
    }

        _MAKE(double, number_double)
        _MAKE(int, number_integer)
        _MAKE(bool, boolean)
        _MAKE(string_t, string)

    #undef _MAKE

        void assign()
        {
            _type = _empty;
            _data.clear();
        }
        void assign(std::nullptr_t) { _type = null; }
        void assign(const string_char_t *x) { assign(string_t(x)); }
        void assign(const value &x)
        {
            _type = x._type;
            _data = x._data;
        }

        void basic_clear()
        {
            switch (_type)
            {
            case number_double:
                as<double>() = 0;
            case number_integer:
                as<int>() = 0;
            case string:
                as<string_t>().clear();
            case boolean:
                as<bool>() = 0;
            }
        }
        virtual void clear() { return basic_clear(); }

        std::string basic_to_string() const
        {
            switch (_type)
            {
            case number_double:
                return std::to_string(_data.as<double>());
            case number_integer:
                return std::to_string(_data.as<int>());
            case string:
                return "";//_data.as<string_t>();
            case null:
                return "null";
            case boolean:
                return _data.as<bool>() ? "true" : "false";
            case _empty:
                return "_empty";
            }
            return "unknown";
        }
        virtual std::string to_string() const { return basic_to_string(); }

        template <typename _iter_t>
        bool basic_parse(_iter_t first, _iter_t last)
        {
            assign();
            bool haserr = false;
            auto is_str = [&first, &last](const std::string &s)
            {
                auto it = first;
                for (auto ch : s)
                {
                    if (!(it != last))
                        return false; // #1
                    if (ch != *it)
                        return false;
                    ++it;
                    // #1 不能置于此处，因为会导致提前结束
                }
                /* 确保末尾没有非空字符 */
                if (it != last && !isspace(*it))
                    return false;
                return true;
            };
            switch (*first)
            {
            case 't':
                if (is_str("true"))
                    assign(true);
                break;
            case 'f':
                if (is_str("false"))
                    assign(false);
                break;
            case 'n':
                if (is_str("null"))
                    assign(nullptr);
                break;
            case '"':
            {
                assign(std::string());
                auto &s = as<std::string>();
                auto ch = *first;
                size_t len = 0;
                auto it = first;
                ++it;
                while (it != last && *it != '"')
                {
                    if (*it == '\\')
                        ++it;
                    ++it;
                    ++len;
                }
                s.reserve(len);
                it = first;
                ++it;
                while (it != last && *it != '"')
                {
                    if (*it == '\\')
                    {
                        ++it;
                        switch (*it)
                        {
                        case '/':
                        case '"':
                        case '\'':
                        case '\\':
                            ch = *it;
                            break;
                        case 'n':
                            ch = '\n';
                            break;
                        case 't':
                            ch = '\t';
                            break;
                        case 'f':
                            ch = '\f';
                            break;
                        case 'v':
                            ch = '\v';
                            break;
                        case 'a':
                            ch = '\a';
                            break;
                        case 'b':
                            ch = '\b';
                            break;
                        case 'u':
                        {
                            // TODO : Unicode->UTF-8
                            break;
                        }
                        default:
                            haserr = true;
                            break;
                        }
                    }
                    else
                        ch = *it;
                    s.push_back(ch);
                    if (haserr)
                        break;
                    ++it;
                }
                break;
            }
            default:
            {
                bool sign = false;
                auto it = first;
                if (*first == '-')
                {
                    ++it;
                    sign = true;
                }
                if (!isdigit(*it))
                    break;

                break;
            }
            }
            return haserr || _type == _empty;
        }
        template <typename _iter_t>
        bool parse(_iter_t first, _iter_t last) { return basic_parse(first, last); }
        template <typename _t>
        bool parse(const _t &x) { return parse(std::begin(x), std::end(x)); }

        inline int type() const { return _type; }
        static const char *basic_type_name(int type)
        {
            switch (type)
            {
            case number_double:
            case number_integer:
                return "number";
            case string:
                return "string";
            case null:
                return "null";
            case boolean:
                return "boolean";
            case _empty:
                return "_empty";
            }
            return "unknown";
        }
        virtual const char *type_name() const { return basic_type_name(_type); }

    private:
        void ensure_is(int type) const
        {
            if (_type != type && _type != null)
                _SJSON_THROW_TYPE_ADJUST_RAW(type_name(), basic_type_name(type));
        }
        friend class json_base::_my_initializer_list;
        int _type;
        _fake_union _data;
    };

    class object : public std::unordered_map<string_t, json_base>
    {
    public:
    };
    class array : public std::vector<json_base>
    {
    public:
        using _base_t = std::vector<json_base>;
        array() : _base_t() {}
        array(std::initializer_list<_my_initializer_list> x)
        {
            for (auto &it : x)
                this->push_back(it.data());
        }
    };

    json_base() : _type(json_type::value), _data(value()) {}
    json_base(const json_base &x) { _assign(x); }
    json_base(const array &x) { _assign(x); }
    json_base(const object &x) { _assign(x); }
    json_base(std::initializer_list<_my_initializer_list> x)
    {
        _assign(_my_initializer_list(x));
    }
    template <
        typename _t,
        typename std::enable_if<
            std::is_constructible<value, _t>::value, int
        >::type = 0
    > json_base(const _t &x) { _assign(value(x)); }

    const json_base &operator=(const json_base &x)
    {
        _assign(x);
        return x;
    }
    const array &operator=(const array &x)
    {
        _assign(x);
        return x;
    }
    const object &operator=(const object &x)
    {
        _assign(x);
        return x;
    }
    const std::initializer_list<_my_initializer_list> &
    operator=(const std::initializer_list<_my_initializer_list> &x)
    {
        _assign(_my_initializer_list(x));
        return x;
    }

    inline operator const array &() const { return as_array(); }
    inline operator const object &() const { return as_object(); }
    template <
        typename _t,
        typename std::enable_if<
            !(
                // 消除 string 歧义
                std::is_pointer<_t>::value // [const] char* [const]
                || std::is_same<_t, string_char_t>::value
                || std::is_same<_t,
                    std::initializer_list<string_char_t>
                    >::value
            ), int
        >::type = 0
    > inline operator _t () const{return _t(as_value());}

    friend std::ostream &operator<<(std::ostream &os, const json_base &j)
    {
        
        return os;
    }

#if defined(_SJSON_DISABLE_AUTO_TYPE_ADJUST)
#define _ENSURE_IS(t) _SJSON_TRY(_ensure_is(t))
#else
#define _ENSURE_IS(t) ((void)0)
#endif

    // 0 会被当成指针造成歧义。。。
    json_base &operator[](int idx) { return this->operator[](size_t(idx)); }
    const json_base &operator[](int idx) const
    { return this->operator[](size_t(idx)); }
    json_base &operator[](size_t idx)
    {
        _ENSURE_IS(json_type::array);
        
        if (!is_array())
        {
            _type = json_type::array;
            _data.set(array());
        }
        return as_array()[idx];
    }
    const json_base &operator[](size_t idx) const
    {
        _ENSURE_IS(json_type::array);
        return as_array()[idx];
    }

    json_base &operator[](const char *const key)
    {
        return this->operator[](std::move(std::string(key)));
    }
    const json_base &operator[](const char *const key)const
    {
        return this->operator[](std::move(string_t(key)));
    }
    json_base &operator[](const string_t &key)
    {
        _ENSURE_IS(json_type::object);
        if (!is_object())
        {
            _type = json_type::object;
            _data.set(object());
        }
        return as_object()[key];
    }
    const json_base &operator[](const string_t &key) const
    {
        _ENSURE_IS(json_type::object);
        static json_base _empty_res;
        const auto &obj = as_object();
        const auto &it = obj.find(key);
        if (it != obj.end())
            return it->second;
        return _empty_res;
    }

#undef _ENSURE_IS

    inline json_type type() const { return _type; }
    inline bool is_value() const { return _type == json_type::value; }
    inline bool is_array() const { return _type == json_type::array; }
    inline bool is_object() const { return _type == json_type::object; }

    inline array &as_array()
    {
        _ENSURE_IS(json_type::array);
        return _data.as<array>();
    }
    inline object &as_object()
    {
        _ENSURE_IS(json_type::object);
        return _data.as<object>();
    }
    inline value &as_value()
    {
        _ENSURE_IS(json_type::value);
        return _data.as<value>();
    }

    inline const array &as_array() const
    {
        _ENSURE_IS(json_type::array);
        return _data.as<array>();
    }
    inline const object &as_object() const
    {
        _ENSURE_IS(json_type::object);
        return _data.as<object>();
    }
    inline const value &as_value() const
    {
        _ENSURE_IS(json_type::value);
        return _data.as<value>();
    }

    bool empty() const
    {
        if (is_array())
            return as_array().empty();
        if (is_object())
            return as_object().empty();
        return _data.empty();
    }
    void clear()
    {
        if (is_array())
            as_array().clear();
        else if (is_object())
            as_object().clear();
        else if (is_value())
            as_value().clear();
        else
            _data.clear();
    }

    inline json_base &at(size_t idx) { return as_array().at(idx); }
    inline const json_base &at(size_t idx) const { return as_array().at(idx); }
    inline json_base &at(string_t key) { return as_object().at(key); }
    inline const json_base &at(string_t key) const { return as_object().at(key); }

    std::string dump(const std::string &tab = "  ") const
    {
        std::string res;
        dump(res, tab);
        return res;
    }
    void dump(std::string &dest, const std::string &tab = "  ", int deep = 0) const
    {
        bool need_tab = !tab.empty();
        auto add_tabs = [&need_tab, &dest, &tab, &deep]()
        {
            if (need_tab)
                for (int i = 0; i < deep; ++i)
                    dest += tab;
        };
        // 规定如果 deep < 0 则此次不输出（用于 object 的输出）
        if (deep < 0)
            deep = -deep;
        else
            add_tabs();

        if (is_array())
        {
            dest += '[';
            const auto &arr = as_array();
            if (need_tab && !arr.empty())
                dest += '\n';
            for (auto it = arr.begin(); it != arr.end(); ++it)
            {
                if (it != arr.begin())
                    dest += need_tab ? ",\n" : ",";
                it->dump(dest, tab, deep + 1);
            }
            if (need_tab && !arr.empty())
            {
                dest += '\n';
                add_tabs();
            }
            dest += ']';
        }
        else if (is_object())
        {
            dest += '{';
            const auto &obj = as_object();
            if (need_tab && !obj.empty())
                dest += '\n';
            deep++;
            for (auto it = obj.begin(); it != obj.end(); ++it)
            {
                if (it != obj.begin())
                    dest += need_tab ? ",\n" : ",";
                add_tabs();
                dest += '"';
                dest += it->first;
                dest += "\": ";
                it->second.dump(dest, tab, -deep);
            }
            deep--;
            if (need_tab && !obj.empty())
            {
                dest += '\n';
                add_tabs();
            }
            dest += '}';
        }
        else if (is_value())
        {
            bool flag = (as_value().type() == value::string);
            if (flag)
                dest += '"';
            dest += as_value().to_string();
            if (flag)
                dest += '"';
        }
        else
        {
            dest += "unknown";
        }
    }

    class parser
    {
    public:
        enum class node_t
        {
            object,
            object_node,
            array,
            value
        };
        enum
        {
            syntax_error,
            range_error // 数字超出范围
        };
        // 返回 true/false 以确定是否保留
        using filter = std::function<bool(node_t, json_base &)>;

        // 返回 true/false 以决定是否继续
        using error_callback_f = std::function<bool(int)>;

        template <typename _iter_t>
        static void parse(
            json_base &res, _iter_t first, _iter_t last)
        {
            using token = std::pair<_iter_t, _iter_t>;

            auto it = first;
            auto skip_blank = [&it, &last]()
            {
                while (it != last && isspace(*it))
                    ++it;
            };
            skip_blank();
            //first
        }
    };

    template <typename _iter_t>
    static json_base parse(_iter_t first, _iter_t last)
    {
        json_base res;
        parser::parse(res, first, last);
        return res;
    }
    static json_base parse(const std::string &x)
    {
        return parse(x.begin(), x.end());
    }

private:
    friend class array;
    friend class _my_initializer_list;

    json_type _type;
    _fake_union _data;

    class _my_initializer_list
    {
    public:
        /*
        * {a,b,c} : 这种情况会对每一个元素调用该函数来初始化
        *  ^
        */
        template <typename _t>
        _my_initializer_list(const _t &x) : _data(x) {}
        /*
        * (a,b,c) 或 {}
        * ^^^^^^^
        * 会调用该函数来初始化
        */
        template <typename... _ts>
        _my_initializer_list(const _ts &...args) : _data(array({json_base(args)...})) {}
        _my_initializer_list(std::initializer_list<_my_initializer_list> x)
        {
            /*
            * 形如 {{"xxx",...},{"yyy",...},...} 的数据会被视为 object
            */
            bool is_object = true;
            for (auto &it : x)
            {
                const auto &node = it._data;
                if ((!node.is_array()) || node.as_array().size() != 2)
                {
                    is_object = false;
                    break;
                }
                const auto &first = node.as_array()[0];
                if (
                    (!first.is_value()) || first.as_value().type() != value::string)
                {
                    is_object = false;
                    break;
                }
            }
            if (is_object)
            {
                _data = object();
                for (auto &it : x)
                {
                    _data.as_object()[
                        it._data.as_array()[0]
                        .as_value()
                        .template as<string_t>()
                    ] = it._data.as_array()[1];
                }
                return;
            }
            _data = array();
            for (auto &it : x)
            {
                _data.as_array().push_back(it._data);
            }
        }

        json_base &data() { return _data; }
        const json_base &data() const { return _data; }

    private:
        json_base _data;
    };

    void _assign(const _my_initializer_list &obj)
    {
        auto &x = obj.data();
        if (x.is_array())
            _assign(x.as_array());
        else if (x.is_object())
            _assign(x.as_object());
        else if (x.is_value())
            _assign(x.as_value());
        else
            _type = json_type::value, _data = value();
    }
    void _assign(const json_base &x)
    {
        _type = x._type;
        _data = x._data;
    }
    template <typename _t, json_type _t_val>
    void _assign(const _t &x)
    {
        if (_type == _t_val)
            _data.as<_t>() = x;
        else
        {
            _data.set(x);
            _type = _t_val;
        }
    }
    inline void _assign(const value &x)
    {
        _assign<value, json_type::value>(x);
    }
    inline void _assign(const array &x)
    {
        _assign<array, json_type::array>(x);
    }
    inline void _assign(const object &x)
    {
        _assign<object, json_type::object>(x);
    }

    inline void _ensure_is(json_type x) const
    {
        if (_type != x)
        {
            if (_type == json_type::value && as_value().type() == value::_empty)
                return;
            _SJSON_THROW_TYPE_ADJUST(_type, x);
        }
    }
};

using json = json_base<std::wstring>;
json operator""_json(const char *s)
{
    return json::parse(s);
}

}