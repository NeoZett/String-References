#pragma once

#include <cstddef>
#include <iterator>
#include <type_traits>

namespace string_ref::detail
{
	// A forward iterator adaptor for containers whose *stored* elements are
	// lightweight handles (e.g. interned string references) but whose
	// *presented* value is something resolved on demand from an external
	// table (e.g. a string_pool).
	//
	// The presented Value isn't stored anywhere stable — dereferencing has
	// to materialize it — so operator*() must return by value (never a
	// reference into a temporary) and operator->() needs a small proxy to
	// make `it->member` work despite that. Both are handled here once, so
	// string_set / string_map / string_multimap don't each need to
	// rediscover why `Value&` or `&temporary` from operator*/-> is a
	// dangling-reference bug.
	//
	// UnderlyingIterator : iterator over the elements actually stored
	//                       (e.g. detail::hash_set<reference_type>::iterator,
	//                       or detail::hash_map<reference_type, T>::iterator)
	// Resolve             : a callable `Value operator()(Pool&, auto&&) const`
	//                       that turns *underlying_it into the presented Value.
	//                       Store whatever state it needs; it's kept by value.
	template <
		typename Value,
		typename Pool,
		typename UnderlyingIterator,
		typename Resolve>
	class resolving_iterator
	{
		template <typename, typename, typename, typename>
		friend class resolving_iterator;

		Pool* pool_ = nullptr;
		UnderlyingIterator it_{};
		[[no_unique_address]] Resolve resolve_{};

		struct arrow_proxy
		{
			Value value;

			[[nodiscard]] const Value* operator->() const noexcept { return &value; }
		};

	public:
		using iterator_category = std::forward_iterator_tag;
		using value_type = Value;
		using difference_type = std::ptrdiff_t;
		using pointer = arrow_proxy;
		using reference = Value; // proxy iterator: *it produces a value, not a stored reference

		resolving_iterator() = default;

		resolving_iterator(Pool* pool, UnderlyingIterator it)
			: pool_(pool), it_(it)
		{
		}

		// Enables iterator -> const_iterator conversion whenever the
		// underlying iterator types are themselves convertible (mirrors how
		// hash_table::iterator converts to hash_table::const_iterator).
		template <
			typename OtherIterator,
			typename = std::enable_if_t<
			!std::is_same_v<OtherIterator, UnderlyingIterator>&&
			std::is_convertible_v<OtherIterator, UnderlyingIterator>>>
			resolving_iterator(const resolving_iterator<Value, Pool, OtherIterator, Resolve>& other)
			: pool_(other.pool_), it_(other.it_)
		{
		}

		[[nodiscard]] reference operator*() const
		{
			return resolve_(*pool_, *it_);
		}

		[[nodiscard]] pointer operator->() const
		{
			return pointer{ **this };
		}

		// The raw stored element (a reference_type for a set, a
		// pair<const reference_type, T>& for a map) without paying for
		// resolution. Prefer this over operator* when you just need the id.
		[[nodiscard]] decltype(auto) raw() const
		{
			return *it_;
		}

		resolving_iterator& operator++()
		{
			++it_;
			return *this;
		}

		resolving_iterator operator++(int)
		{
			resolving_iterator copy = *this;
			++(*this);
			return copy;
		}

		[[nodiscard]] friend bool operator==(
			const resolving_iterator& lhs,
			const resolving_iterator& rhs) noexcept
		{
			return lhs.it_ == rhs.it_;
		}

		[[nodiscard]] friend bool operator!=(
			const resolving_iterator& lhs,
			const resolving_iterator& rhs) noexcept
		{
			return !(lhs == rhs);
		}
	};
}