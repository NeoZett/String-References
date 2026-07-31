#pragma once

#include <unordered_set>
#include <string_ref/string_pool.hpp>

namespace string_ref::extensions
{
	class string_set
	{
	public:

		string_set(
			string_pool& pool)
			: pool_(&pool)
		{
		}

	private:

		string_pool* pool_;
		std::unordered_set
	};
}