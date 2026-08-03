#pragma once

#include <cassert>
#include <initializer_list>
#include <string_ref/detail/pair_resolve.hpp>
#include <string_ref/detail/resolving_iterator.hpp>
#include <string_ref/detail/multimap.hpp>
#include <string_ref/string_pool.hpp>

namespace string_ref::extensions
{
	// Multi-key map from interned strings to T: the same string may map to
	// several values. Backed by detail::hash_multimap<reference_type, T>.
	// No operator[] or insert_or_assign here — both are ambiguous once a
	// key can have more than one value (same reasoning as std::multimap).
	template <typename T>
	class string_multimap
	{
	public:
		using key_type = string;
		using mapped_type = T;
		using reference_type = string_reference;
		using size_type = std::size_t;

		using iterator = detail::resolving_iterator<
			std::pair<string, T&>, string_pool,
			typename detail::hash_multimap<reference_type, T>::iterator,
			detail::pair_resolve_mutable<string, T>>;

		using const_iterator = detail::resolving_iterator<
			std::pair<string, const T&>, string_pool,
			typename detail::hash_multimap<reference_type, T>::const_iterator,
			detail::pair_resolve_const<string, T>>;

		explicit string_multimap(
			string_pool& pool)
			: pool_(&pool)
		{
		}

		string_multimap(
			string_pool& pool,
			std::initializer_list<std::pair<const char*, T>> init)
			: pool_(&pool)
		{
			reserve(init.size());

			for (const auto& [text, value] : init)
				insert(text, value);
		}

		[[nodiscard]]
		bool empty() const noexcept
		{
			return entries_.empty();
		}

		[[nodiscard]]
		size_type size() const noexcept
		{
			return entries_.size();
		}

		void reserve(size_type count)
		{
			entries_.reserve(count);
		}

		void clear() noexcept
		{
			entries_.clear();
		}

		reference_type insert(const char* text, const T& value)
		{
			reference_type ref = pool_->append_string(text);

			entries_.insert(ref, value);

			return ref;
		}

		reference_type insert(const string& str, const T& value)
		{
			assert(str.pool() == pool_ && "string belongs to a different string_pool");

			entries_.insert(str.reference(), value);

			return str.reference();
		}

		reference_type insert(reference_type ref, const T& value)
		{
			pool_->check_id(ref);

			entries_.insert(ref, value);

			return ref;
		}

		// Removes every value stored under `key`. Returns the number
		// removed.
		size_type erase(reference_type ref)
		{
			return entries_.erase(ref);
		}

		size_type erase(const char* text)
		{
			auto ref = pool_->find_reference(text);

			if (ref == base::invalid_reference)
				return 0;

			return erase(ref);
		}

		size_type erase(const string& str)
		{
			return erase(str.reference());
		}

		[[nodiscard]]
		bool contains(reference_type ref) const
		{
			return entries_.contains(ref);
		}

		[[nodiscard]]
		bool contains(const char* text) const
		{
			auto ref = pool_->find_reference(text);

			if (ref == base::invalid_reference)
				return false;

			return contains(ref);
		}

		[[nodiscard]]
		bool contains(const string& str) const
		{
			return contains(str.reference());
		}

		[[nodiscard]]
		size_type count(reference_type ref) const
		{
			return entries_.count(ref);
		}

		[[nodiscard]]
		size_type count(const char* text) const
		{
			auto ref = pool_->find_reference(text);
			return ref == base::invalid_reference ? 0 : count(ref);
		}

		[[nodiscard]]
		size_type count(const string& str) const
		{
			return count(str.reference());
		}

		[[nodiscard]]
		std::pair<iterator, iterator> equal_range(reference_type ref)
		{
			auto [first, last] = entries_.equal_range(ref);
			return { iterator(pool_, first), iterator(pool_, last) };
		}

		[[nodiscard]]
		std::pair<const_iterator, const_iterator> equal_range(reference_type ref) const
		{
			auto [first, last] = entries_.equal_range(ref);
			return { const_iterator(pool_, first), const_iterator(pool_, last) };
		}

		[[nodiscard]]
		std::pair<iterator, iterator> equal_range(const char* text)
		{
			auto ref = pool_->find_reference(text);

			if (ref == base::invalid_reference)
				return { end(), end() };

			return equal_range(ref);
		}

		[[nodiscard]]
		std::pair<const_iterator, const_iterator> equal_range(const char* text) const
		{
			auto ref = pool_->find_reference(text);

			if (ref == base::invalid_reference)
				return { end(), end() };

			return equal_range(ref);
		}

		[[nodiscard]]
		std::pair<iterator, iterator> equal_range(const string& str)
		{
			return equal_range(str.reference());
		}

		[[nodiscard]]
		std::pair<const_iterator, const_iterator> equal_range(const string& str) const
		{
			return equal_range(str.reference());
		}

		[[nodiscard]]
		iterator begin() noexcept
		{
			return { pool_, entries_.begin() };
		}

		[[nodiscard]]
		iterator end() noexcept
		{
			return { pool_, entries_.end() };
		}
		
		[[nodiscard]]
		const_iterator begin() const noexcept
		{
			return { pool_, entries_.begin() };
		}

		[[nodiscard]]
		const_iterator end() const noexcept
		{
			return { pool_, entries_.end() };
		}

	private:
		string_pool* pool_;

		detail::hash_multimap<reference_type, T> entries_;
	};
}