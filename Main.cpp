// Strictly during testing. Reminder: The project is still early in development.

#include <string_ref/string_ref.hpp>
#include <string_ref/extensions/string_set.hpp>
#include <string_ref/extensions/string_map.hpp>
#include <string_ref/extensions/string_multimap.hpp>

#include <iostream>

int main()
{
	string_ref::string_pool pool;

	string_ref::extensions::string_set mySet(pool);

	string_ref::string text("hello", pool);

	mySet.insert(text);

	for (const auto& str : mySet)
	{
		std::cout << str.reference() << ": " << str.c_str() << std::endl;
	}

	std::cout << mySet.contains(text.reference()) << std::endl;

	return 0;
}