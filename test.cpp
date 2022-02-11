#include <iostream>
#include "sjson/sjson.hpp"

using namespace sjson;

class A
{
public:
	template<typename _t,
	typename std::enable_if<
		!(
			std::is_pointer<_t>::value
		||	std::is_same<_t,typename std::string::value_type>::value
		|| std::is_same<_t,std::initializer_list<typename std::string::value_type> >::value
		),
	int>::type=0>
	operator _t()const
	{
		puts(typeid(_t).name());
		return _t();
	}
	void xxx(int sss,A foo)
	{
		
	}
};

int main()
{	
	json x =
	{
		{"title", "Title A"},
		{"author", "Author A"},
		{"price", 114.514},
		{"restricted", false},
		{"tag", {"tag1", "tag2",1}}
	}; 
	x["title"] = "Title A";
	x["author"] = "Author A";
	x["price"] = 114.514;
	x["restricted"] = false;
	x["tag"] = {"tag1", "tag2"};

	std::string v;
	try
	{
		v=x[0];
		//a.
		//a.as_value().as<std::wstring>();
		//std::string s= a[0];
		//std::cout<<s;
	}
	catch(const json_error& e)
	{
		std::cout<<e.what();
	}
	std::cout<<v<<" "<<x["xxx"].type_name();

	//std::cout << x.dump();

	json arr_test = json::array{
		{L"xxx", 1},
		{L"yyy", true},
		{L"zzz", nullptr},
		{L"aaa", 0.342}};
	std::string s = "str";
	double d = 0.114;

	json xx =
	{
		1, 2, 3, 4, d, s, arr_test,
		{
			{"pi", 3.141}, {"happy", true},
			{"name", "Niels"}, {"nothing", nullptr}
		},
		{"answer",
			{
				{{"everything", 42}, "ffff"},
				{1111}, {}, {}, {}, {}, {}
			}
		},
		{"list", {1, 0, 2}},
		{
			"object",
		 	{{"currency", "USD"}, {"value", 42.99}},
			"object2",
			{{"currency", "USD"}, {"value", 42.99}},
			"ssss", "object3",
			{{"currency", "USD"}, {"value", 42.99}},
			{{"fu((", "USD"}, {"value", 42.99}},
			{{"thtttt", "USD"}, {"value", 42.99}}
		}
	};
	//std::cout.width(1);
	std::cout << xx;
	
	return 0;
}
