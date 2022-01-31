#pragma once

#include <string>
#include <vector>

#include <unordered_map>

#include <functional>
#include <type_traits>
#include <stdexcept>

#define _SJSON_DISABLE_AUTO_TYPE_ADJUST

#if defined(_MSC_VER)
#define _HASCPP14 (_MSVC_LANG>=201402L)
#else
#define _HASCPP14 (__cplusplus>=201402L)
#endif

namespace sjson
{

class _ref_union
{
public:
    constexpr _ref_union() :_data(nullptr) {};
    constexpr _ref_union(const _ref_union& x) :_data(x._data) {}
    template<typename T>
    inline constexpr void set(const T* x) { _data = (void*)x; };
    template<typename T>
    inline constexpr T* get() { return (T*)_data; }
    template<typename T>
    inline constexpr const T* get()const { return (T*)_data; }
private:
    void* _data;
}; 
class _fake_union
{
public:

    _fake_union() :_data(nullptr) {}
    _fake_union(const _fake_union& x)
    {
        _data = nullptr;
        _assign(x);
    }
    template<typename _t>
    _fake_union(const _t& x)
    {
        _data = nullptr;
        set(x);
    }
    const _fake_union& operator=(const _fake_union& x)
    {
        _assign(x);
        return x;
    }

    ~_fake_union()
    {
        _destroy_data();
    }

    inline bool empty()const
    {
        return _data == nullptr;
    }
    inline void clear()
    {
        _destroy_data();
        _data = nullptr;
    }

    template<typename _t>
    inline _t& get()
    {
        return *((_t*)_data);
    }
    template<typename _t>
    inline const _t& get()const
    {
        return *((const _t*)_data);
    }
    // 如果原先为空则创建一个新对象再返回
    template<typename _t>
    inline _t& as()
    {
        if (empty())set(_t());
        return get<_t>();
    }
    template<typename _t>
    inline const _t& as()const
    {
        if (empty())
        {
            static _t x;
            return x;
        }
        return get<_t>();
    }

    template<typename _t>
    void set(const _t& x)
    {
        _destroy_data();
        _destructor = [](void* p) {delete (_t*)p; };
        _copy_constructor = [](void* x) {return new _t(*((_t*)x)); };
        _data = new _t(x);
    }

private:

    void* _data = nullptr;
    std::function<void(void*)> _destructor;
    std::function<void* (void*)> _copy_constructor;

    void _destroy_data()
    {
        if (!empty()) _destructor(_data);
        // 可以不设为空指针，因为该函数在内部使用，可确保调用后会赋值
    }

    void _assign(const _fake_union& x)
    {
        _destroy_data();
        _destructor = x._destructor;
        _copy_constructor = x._copy_constructor;
        if (x._data != nullptr)
            _data = _copy_constructor(x._data);
        else _data = nullptr;
    }
};

enum class json_type
{
    value,
    array,
    object
};
constexpr const char* json_type_name(json_type x)
{
    /* 为了兼容 c++11 。。。 */
    return x == json_type::array
     ? "array"
     : (x == json_type::object ? "object" : "value");
}

class basic_value_type
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

    basic_value_type() :_type(_empty),_data(nullptr) {}
    basic_value_type(std::nullptr_t) { assign(nullptr); }
    basic_value_type(bool x) { assign(x); }
    basic_value_type(double x) { assign(x); }
    basic_value_type(int x) { assign(x); }
    basic_value_type(const char* x) { assign(x); }
    basic_value_type(const std::string& x) { assign(x); }

    virtual ~basic_value_type()=default;

    operator float()const { return _data.as<float>(); }
    operator double ()const { return _data.as<double>(); }
    operator int ()const { return _data.as<int>(); }
    operator bool ()const { return _data.as<bool>(); }
    operator std::string()const { return _data.as<std::string>(); }

    template<typename _t> _t& as() { return _data.as<_t>(); }
    template<typename _t> const _t& as()const { return _data.as<_t>(); }

    void assign() { _type = _empty; _data.clear(); }
    void assign(std::nullptr_t) { _type = null; }
    void assign(bool x) { _type = boolean; _data = x; }
    void assign(double x) { _type = number_double; _data = x; }
    void assign(int x) { _type = number_integer; _data = x; }
    void assign(const char* x) { _type = string; _data = std::string(x); }
    void assign(const std::string& x)
    { _type = string; _data = x; }

    void basic_clear()
    {
        switch (_type)
        {
        case number_double:as<double>() = 0;
        case number_integer:as<int>() = 0;
        case string:as<std::string>().clear();
        case boolean:as<bool>() = 0;
        }
    }
    virtual void clear()
    {
        return basic_clear();
    }

    std::string basic_to_string()const
    {
        switch (_type)
        {
        case number_double:
            return std::to_string(_data.as<double>());
        case number_integer:
            return std::to_string(_data.as<int>());
        case string:return _data.as<std::string>();
        case null:return "null";
        case boolean:return _data.as<bool>() ? "true" : "false";
        case _empty:return "_empty";
        }
        return "unknown";
    }
    virtual std::string to_string()const
    {
        return basic_to_string();
    }

    template<typename _iter_t>
    bool basic_parse(_iter_t first, _iter_t last)
    {
        assign();
        bool haserr = false;
        auto is_str = [&first, &last](const std::string& s)
        {
            auto it = first;
            for (auto ch : s)
            {
                if (!(it != last))return false; // #1
                if (ch != *it)return false;
                ++it;
                // #1 不能置于此处，因为会导致提前结束
            }
            /* 确保末尾没有非空字符 */
            if (it != last && !isspace(*it))return false;
            return true;
        };
        switch (*first)
        {
        case 't':if (is_str("true"))assign(true); break;
        case 'f':if (is_str("false"))assign(false); break;
        case 'n':if (is_str("null"))assign(nullptr); break;
        case '"':
        {
            assign(std::string());
            auto& s = as<std::string>();
            auto ch = *first;
            size_t len = 0;
            auto it = first;
            ++it;
            while (it != last && *it != '"')
            {
                if (*it == '\\')++it;
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
                    case '\\':ch = *it; break;
                    case 'n':ch = '\n'; break;
                    case 't':ch = '\t'; break;
                    case 'f':ch = '\f'; break;
                    case 'v':ch = '\v'; break;
                    case 'a':ch = '\a'; break;
                    case 'b':ch = '\b'; break;
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
                else ch = *it;
                s.push_back(ch);
                if (haserr)break;
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
            if (!isdigit(*it))break;

            break;
        }
        }
        return haserr || _type == _empty;
    }
    template<typename _iter_t>
    bool parse(_iter_t first, _iter_t last) { return basic_parse(first, last); }
    template<typename _t>
    bool parse(const _t& x) { return parse(std::begin(x), std::end(x)); }

    inline int type()const { return _type; }
    static const char* basic_type_name(int type)
    {
        switch (type)
        {
        case number_double:
        case number_integer:
            return "number";
        case string:return "string";
        case null:return "null";
        case boolean:return "boolean";
        case _empty:return "_empty";
        }
        return "unknown";
    }
    virtual const char* type_name(int type)
    {
        return basic_type_name(type);
    }
    virtual std::string type_name()const
    {
        return basic_type_name(_type);
    }

private:

    int _type;
    _fake_union _data;
};

template<typename _value_type>
class json_base
{
private:
    class _my_initializer_list;
    static const std::function<std::string(int, const char*, const std::string&)>
        _basic_error_info;
public:

    class error
    {
    public:

#define _MAKE(name) class name\
        {\
        public:\
            name(const char*const str):_msg(str){}\
            const char*const what()const{return _msg;}\
        private:\
            const char*const _msg;\
        }

        _MAKE(type_adjust);

        /*
        * 禁用意外类型的自动类型调整
        * 如：对 array 对象使用 ["key"]
        *   如果是 const array 则会返回空的 json
        *   如果是 array 则会将该结点转换成 object 再返回
        */
#ifndef _SJSON_DISABLE_AUTO_TYPE_ADJUST
#define _SJSON_THROW_TYPE_ADJUST(dest,need) ((void)0)
#else

#define _SJSON_THROW_TYPE_ADJUST(dest,need)\
    _SJSON_THROW(error::type_adjust,\
        std::string("call method of type(")\
        + json_type_name(need) + ") on type(" + json_type_name(dest) + ")")
#endif

#undef _MAKE
    };

#define _SJSON_THROW(err,str) throw typename err (_basic_error_info(__LINE__,__FUNCTION__,str).c_str())

    using object_key_t = std::string;

    using value = _value_type;

    using object = std::unordered_map<object_key_t, json_base>;
    class array :public std::vector<json_base>
    {
    public:
        using _base_t = std::vector<json_base>;
        array() : _base_t() {}
        array(std::initializer_list<_my_initializer_list> x)
        {
            for (auto& it : x)
            {
                this->push_back(it.data());
            }
        }
    };

    json_base() :_type(json_type::value), _data(value()) {}
    json_base(const json_base& x) { _assign(x); }
    json_base(const array& x) { _assign(x); }
    json_base(const object& x) { _assign(x); }
    json_base(std::initializer_list<_my_initializer_list> x)
    {
        _assign(_my_initializer_list(x));
    }
    template<
        typename _t,
        typename std::enable_if <
        std::is_constructible<value, _t>::value, int
        >::type = 0
    >
        json_base(const _t& x)
    {
        _assign(_value_type(x));
    }

    const json_base& operator=(const json_base& x)
    {
        _assign(x); return x;
    }
    const array& operator=(const array& x)
    {
        _assign(x); return x;
    }
    const object& operator=(const object& x)
    {
        _assign(x); return x;
    }
    const std::initializer_list<_my_initializer_list>&
     operator=(const std::initializer_list<_my_initializer_list>& x)
    {
        _assign(_my_initializer_list(x));return x;
    }

    template<typename _t>
    explicit inline operator const _t()const
    {
        return as_value();
    }

    json_base& operator [](size_t idx)
    {
        _ensure_is(json_type::array);
        if (!is_array())
        {
            _type = json_type::array;
            _data.set(array());
        }
        return as_array()[idx];
    }
    const json_base& operator [](size_t idx)const
    {
        _ensure_is(json_type::array);
        return as_array()[idx];
    }

    json_base& operator [](const object_key_t& key)
    {
        _ensure_is(json_type::object);
        if (!is_object())
        {
            _type = json_type::object;
            _data.set(object());
        }
        return as_object()[key];
    }
    const json_base& operator[](const object_key_t& key)const
    {
        _ensure_is(json_type::object);
        static json_base _empty_res;
        const auto& obj = as_object();
        const auto& it = obj.find(key);
        if (it != obj.end())return it->second;
        return _empty_res;
    }

    inline json_type type()const { return _type; }
    inline bool is_value()const { return _type == json_type::value; }
    inline bool is_array()const { return _type == json_type::array; }
    inline bool is_object()const { return _type == json_type::object; }

    inline array& as_array()
    {
        _ensure_is(json_type::array);
        return _data.as<array>();
    }
    inline object& as_object()
    {
        _ensure_is(json_type::object);
        return _data.as<object>();
    }
    inline value& as_value()
    {
        _ensure_is(json_type::value);
        return _data.as<value>();
    }

    inline const array& as_array()const
    {
        _ensure_is(json_type::array);
        return _data.as<array>();
    }
    inline const object& as_object()const
    {
        _ensure_is(json_type::object);
        return _data.as<object>();
    }
    inline const value& as_value()const
    {
        _ensure_is(json_type::value);
        return _data.as<value>();
    }

    bool empty()const
    {
        if (is_array())return as_array().empty();
        if (is_object())return as_object().empty();
        return _data.empty();
    }
    void clear()
    {
        if (is_array())as_array().clear();
        else if (is_object())as_object().clear();
        else if(is_value())as_value().clear();
        else _data.clear();
    }

    inline json_base& at(size_t idx) { return as_array().at(idx); }
    inline const json_base& at(size_t idx)const { return as_array().at(idx); }
    inline json_base& at(object_key_t key) { return as_object().at(key); }
    inline const json_base& at(object_key_t key)const { return as_object().at(key); }

    std::string dump(const std::string& tab="  ")const
    {
        std::string res;
        dump(res, tab);
        return res;
    }
    void dump(std::string& dest, const std::string& tab = "  ", int deep = 0)const
    {
        bool need_tab = !tab.empty();
        auto add_tabs = [&need_tab, &dest, &tab, &deep]()
        {
            if (need_tab)
                for (int i = 0; i < deep; ++i)dest += tab;
        };
        // 规定如果 deep < 0 则此次不输出（用于 object 的输出）
        if (deep < 0)deep = -deep;
        else add_tabs();

        if (is_array())
        {
            dest += '[';
            const auto& arr = as_array();
            if (need_tab && !arr.empty())dest += '\n';
            for (auto it = arr.begin(); it != arr.end(); ++it)
            {
                if (it != arr.begin())dest += need_tab ? ",\n" : ",";
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
            const auto& obj = as_object();
            if (need_tab && !obj.empty())dest += '\n';
            deep++;
            for (auto it = obj.begin(); it != obj.end(); ++it)
            {
                if (it != obj.begin())dest += need_tab ? ",\n" : ",";
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
            if (flag)dest += '"';
            dest += as_value().to_string();
            if (flag)dest += '"';
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
        using filter = std::function<bool(node_t, json_base&)>;

        // 返回 true/false 以决定是否继续
        using error_callback_f = std::function<bool(int)>;

        template <typename _iter_t>
        static void parse(
            json_base& res, _iter_t first, _iter_t last
        )
        {
            using token = std::pair<_iter_t, _iter_t>;

            auto it = first;
            auto skip_blank = [&it, &last]()
            {
                while (it != last && isspace(*it))++it;
            };
            skip_blank();
            //first
        }
    };

    template<typename _iter_t>
    static json_base parse(_iter_t first, _iter_t last)
    {
        json_base res;
        parser::parse(res, first, last);
        return res;
    }
    static json_base parse(const std::string& x)
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
        template<typename _t>
        _my_initializer_list(const _t& x) :_data(x) {}
        /*
        * (a,b,c) 或 {}
        * ^^^^^^^
        * 会调用该函数来初始化
        */
        template<typename ..._ts>
        _my_initializer_list(const _ts& ...args) :
            _data(array({ json_base(args)... })) {}
        _my_initializer_list(std::initializer_list<_my_initializer_list> x)
        {
            /*
            * 形如 {{"xxx",...},{"yyy",...},...} 的数据会被视为 object
            */
            bool is_object = true;
            for (auto& it : x)
            {
                const auto& node = it._data;
                if ((!node.is_array()) || node.as_array().size() != 2)
                {
                    is_object = false;
                    break;
                }
                const auto& first = node.as_array()[0];
                if (
                    (!first.is_value())
                    || first.as_value().type() != _value_type::string
                ){
                    is_object = false;
                    break;
                }
            }
            if (is_object)
            {
                _data = object();
                for (auto& it : x)
                {
                    _data.as_object()[
                        it._data.as_array()[0].as_value().to_string()
                    ] = it._data.as_array()[1];
                }
                return;
            }
            _data = array();
            for (auto& it : x)
            {
                _data.as_array().push_back(it._data);
            }
        }

        json_base& data() { return _data; }
        const json_base& data()const { return _data; }

    private:

        json_base _data;
    };

    void _assign(const _my_initializer_list& obj)
    {
        auto& x = obj.data();
        if (x.is_array())_assign(x.as_array());
        else if (x.is_object())_assign(x.as_object());
        else if (x.is_value())_assign(x.as_value());
        else _type = json_type::value, _data = value();
    }
    void _assign(const json_base& x)
    {
        _type = x._type;
        _data = x._data;
    }
    template<typename _t, json_type _t_val>
    void _assign(const _t& x)
    {
        if (_type == _t_val)_data.as<_t>() = x;
        else
        {
            _data.set(x);
            _type = _t_val;
        }
    }
    inline void _assign(const value& x)
    {
        _assign<value, json_type::value>(x);
    }
    inline void _assign(const array& x)
    {
        _assign<array, json_type::array>(x);
    }
    inline void _assign(const object& x)
    {
        _assign<object, json_type::object>(x);
    }

    inline void _ensure_is(json_type x)const
    {
        if (_type != x)_SJSON_THROW_TYPE_ADJUST(_type, x);
    }
};
template<typename _value_type>
const std::function<std::string(int, const char*, const std::string&)>
json_base<_value_type>::_basic_error_info = [](int line, const char* func, const std::string& str)
{
    return std::to_string(line) + ':' + func + ':' + str;
};

using json = json_base<basic_value_type>;
json operator ""_json(const char* s)
{
    return json::parse(s);
}

}