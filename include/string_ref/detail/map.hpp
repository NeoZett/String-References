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
		class hash_map
		: public hash_table<std::pair<const Key, T>, Key,
		pair_key_of_value<Key, T>, Hash, KeyEqual, /*AllowDuplicates=*/false>
	{
		using base = hash_table<std::pair<const Key, T>, Key,
			pair_key_of_value<Key, T>, Hash, KeyEqual, false>;

	public:
		using base::base;
		using typename base::iterator;

		T& operator[](const Key& key)
		{
			auto [it, inserted] = this->emplace(key, T{});
			return it->second;
		}

		template <typename V>
		std::pair<iterator, bool> insert_or_assign(const Key& key, V&& value)
		{
			auto [it, inserted] = this->emplace(key, std::forward<V>(value));

			if (!inserted)
				it->second = std::forward<V>(value);

			return { it, inserted };
		}
	};
}