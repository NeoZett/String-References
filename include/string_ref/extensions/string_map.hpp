#pragma once

#include <cassert>
#include <initializer_list>
#include <string_ref/detail/pair_resolve.hpp>
#include <string_ref/detail/resolving_iterator.hpp>
#include <string_ref/detail/map.hpp>
#include <string_ref/string_pool.hpp>

namespace string_ref::extensions
{
	// Unique-key map from interned strings to T. Backed by
	// detail::hash_map<reference_type, T> — no per-key heap allocation.
	template <typename T>
	class string_map
	{
	public:
		using key_type = string;
		using mapped_type = T;
		using reference_type = string_reference;
		using size_type = std::size_t;

		using iterator = detail::resolving_iterator<
			std::pair<string, T&>, string_pool,
			typename detail::hash_map<reference_type, T>::iterator,
			detail::pair_resolve_mutable<string, T>>;

		using const_iterator = detail::resolving_iterator<
			std::pair<string, const T&>, string_pool,
			typename detail::hash_map<reference_type, T>::const_iterator,
			detail::pair_resolve_const<string, T>>;

		explicit string_map(
			string_pool& pool)
			: pool_(&pool)
		{
		}

		string_map(
			string_pool& pool,
			std::initializer_list<std::pair<const char*, T>> init)
			: pool_(&pool)
		{
			reserve(init.size());

			for (const auto& [text, value] : init)
				insert(text, value);
		}

		string_map()
			: pool_(&string_pool::get_global_instance())
		{
		}

		string_map(
			std::initializer_list<std::pair<const char*, T>> init)
		{
			*this = string_map(string_pool::get_global_instance(), init);
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

		// Does not overwrite an existing entry for the same key (matches
		// hash_map's own insert/emplace semantics). Use operator[] or
		// insert_or_assign for upsert behavior.
		reference_type insert(const char* text, const T& value)
		{
			reference_type ref = pool_->append_string(text);

			entries_.emplace(ref, value);

			return ref;
		}

		reference_type insert(const string& str, const T& value)
		{
			assert(str.pool() == pool_ && "string belongs to a different string_pool");

			entries_.emplace(str.reference(), value);

			return str.reference();
		}

		reference_type insert(reference_type ref, const T& value)
		{
			pool_->check_id(ref);

			entries_.emplace(ref, value);

			return ref;
		}

		reference_type insert_or_assign(const char* text, const T& value)
		{
			reference_type ref = pool_->append_string(text);

			entries_.insert_or_assign(ref, value);

			return ref;
		}

		reference_type insert_or_assign(const string& str, const T& value)
		{
			assert(str.pool() == pool_ && "string belongs to a different string_pool");

			entries_.insert_or_assign(str.reference(), value);

			return str.reference();
		}

		T& operator[](const char* text)
		{
			reference_type ref = pool_->append_string(text);

			return entries_[ref];
		}

		T& operator[](const string& str)
		{
			assert(str.pool() == pool_ && "string belongs to a different string_pool");

			return entries_[str.reference()];
		}

		T& operator[](reference_type ref)
		{
			pool_->check_id(ref);

			return entries_[ref];
		}

		bool erase(reference_type ref)
		{
			return entries_.erase(ref) != 0;
		}

		bool erase(const char* text)
		{
			auto ref = pool_->find(text);

			if (ref == base::invalid_reference)
				return false;

			return erase(ref);
		}

		bool erase(const string& str)
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
			auto ref = pool_->find(text);

			if (ref == base::invalid_reference)
				return false;

			return contains(ref);
		}

		[[nodiscard]]
		bool contains(const string& str) const
		{
			return contains(str.reference());
		}

		// Returns nullptr rather than end()/an iterator: callers already
		// have the key (they had to supply it), so what's useful back is
		// access to the value, not something to re-derive the key from.
		[[nodiscard]]
		T* find(reference_type ref)
		{
			auto it = entries_.find(ref);
			return it == entries_.end() ? nullptr : &it->second;
		}

		[[nodiscard]]
		const T* find(reference_type ref) const
		{
			auto it = entries_.find(ref);
			return it == entries_.end() ? nullptr : &it->second;
		}

		[[nodiscard]]
		T* find(const char* text)
		{
			auto ref = pool_->find(text);
			return ref == base::invalid_reference ? nullptr : find(ref);
		}

		[[nodiscard]]
		const T* find(const char* text) const
		{
			auto ref = pool_->find(text);
			return ref == base::invalid_reference ? nullptr : find(ref);
		}

		[[nodiscard]]
		T* find(const string& str)
		{
			return find(str.reference());
		}

		[[nodiscard]]
		const T* find(const string& str) const
		{
			return find(str.reference());
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

		detail::hash_map<reference_type, T> entries_;
	};
}