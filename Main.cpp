// Strictly during testing. Reminder: The project is still early in development.

#include <string_ref/string_ref.hpp>

#include <iostream>

int main()
{
	string_ref::string test("Hello, world!");

	std::cout << test;

	return 0;
}