// Strictly during testing. Reminder: The project is still early in development.

#include <string_ref/string_ref.hpp>
#include <string_ref/extensions/string_set.hpp>
#include <string_ref/extensions/string_map.hpp>
#include <string_ref/extensions/string_multimap.hpp>

#include <iostream>

int main()
{
	string_ref::extensions::string_set mySet(string_ref::get_global_pool());

	mySet.insert("hello");

	for (const auto& str : mySet)
	{
		std::cout << str.c_str() << std::endl;
	}

	return 0;
}