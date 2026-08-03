// Strictly during testing. Reminder: The project is still early in development.

#include <string_ref/string_ref.hpp>
#include <string_ref/extensions/string_set.hpp>
#include <string_ref/extensions/string_map.hpp>
#include <string_ref/extensions/string_multimap.hpp>

#include <iostream>

int main()
{
	string_ref::string_pool pool;

	string_ref::extensions::string_map<int> myMap(pool);

	string_ref::string text("hello", pool);

	myMap["hello"] = 42;

	std::cout << myMap["hello"] << std::endl; // Output: 42

	return 0;
}