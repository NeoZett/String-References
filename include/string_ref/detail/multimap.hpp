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
		typename T,
		typename Hash = std::hash<Key>,
		typename KeyEqual = std::equal_to<Key>>
		class hash_multimap
		: public hash_table<std::pair<const Key, T>, Key,
		pair_key_of_value<Key, T>, Hash, KeyEqual, /*AllowDuplicates=*/true>
	{
		using base = hash_table<std::pair<const Key, T>, Key,
			pair_key_of_value<Key, T>, Hash, KeyEqual, true>;

	public:
		using base::base;

		auto insert(const Key& key, const T& value) { return this->emplace(key, value); }
	};
}