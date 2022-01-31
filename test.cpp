#include <iostream>

#include "sjson/sjson.hpp"

using namespace sjson;

int main()
{
    json arr_test=json::array{
        {"xxx",1},
        {"yyy",true},
        {"zzz",nullptr},
        {"aaa",0.342}  
    }; 
    std::string s="str";
    double d=0.114;
    arr_test=
	{
		{"aa",1},
		{"x",2}
	};
   	json xx = 
	{
		1,2,3,4,d,
		s,
		arr_test,
		{
			{"pi", 3.141},
			{"happy", true},
			{"name", "Niels"},
			{"nothing", nullptr}
		},
		{"answer",
			{
				{
					{"everything", 42},
					"ffff"
				},
				{
					1111
				},
				{},{},{},{},{}
			}
		},
		{"list", {1, 0, 2}},
		{
			"object",
			{
				{"currency", "USD"},
				{"value", 42.99}
			},
			"object2",
			{
				{"currency", "USD"},
				{"value", 42.99}
			},
			"ssss",
			"object3",
			{
				{"currency", "USD"},
				{"value", 42.99}
			},
			{
				{"fu((", "USD"},
				{"value", 42.99}
			},
			{
				{"thtttt", "USD"},
				{"value", 42.99}
			}
		}
	};
    std::cout<<xx.dump();
    system("pause");
    return 0;
}