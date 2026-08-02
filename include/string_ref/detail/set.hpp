#pragma once

#include <string_ref/detail/hash_table.hpp>

// ---- friendly aliases ------------------------------------------------
//
// These are what you actually reach for day to day; they just pin down
// Value/KeyOfValue/AllowDuplicates for the three shapes you need and add
// the small conveniences each container flavor is expected to have.

namespace string_ref::detail
{
	template <
		typename Key,
		typename Hash = std::hash<Key>,
		typename KeyEqual = std::equal_to<Key>>
		class hash_set
		: public hash_table<Key, Key,
		identity_key_of_value<Key>, Hash, KeyEqual, /*AllowDuplicates=*/false>
	{
		using base = hash_table<Key, Key, identity_key_of_value<Key>, Hash, KeyEqual, false>;

	public:
		using base::base;
	};
}