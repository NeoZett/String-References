#pragma once

#include <utility>

namespace string_ref::detail
{
	// Resolve policies shared by string_map and string_multimap.
	//
	// Unlike a set's element, a map's stored node is `pair<const RawKey, T>`
	// — the mapped value T already lives persistently in the node, so only
	// the key needs pool resolution. That's why mutable and const iterators
	// need genuinely different Value types (pair<Key, T&> vs
	// pair<Key, const T&>) rather than sharing one, the way string_set's
	// single `string` Value does — see resolving_iterator's converting
	// constructor, which was generalized specifically to allow that.
	template <typename Key, typename T>
	struct pair_resolve_mutable
	{
		template <typename Pool, typename RawKey>
		std::pair<Key, T&> operator()(Pool& pool, std::pair<const RawKey, T>& raw) const
		{
			return { pool.get(raw.first), raw.second };
		}
	};

	template <typename Key, typename T>
	struct pair_resolve_const
	{
		template <typename Pool, typename RawKey>
		std::pair<Key, const T&> operator()(Pool& pool, const std::pair<const RawKey, T>& raw) const
		{
			return { pool.get(raw.first), raw.second };
		}
	};
}