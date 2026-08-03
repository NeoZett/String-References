#pragma once

#include <cassert>
#include <initializer_list>
#include <string_ref/detail/resolving_iterator.hpp>
#include <string_ref/detail/set.hpp>
#include <string_ref/string_pool.hpp>

namespace string_ref::extensions
{
	class string_set
	{
	public:
		using value_type = string;
		using reference_type = string_reference;
		using size_type = std::size_t;

	private:
		// Turns a stored reference_type back into a `string` on demand.
		// This is the only set-specific knowledge resolving_iterator needs.
		struct resolve_string
		{
			string operator()(string_pool& pool, reference_type ref) const
			{
				return pool.get(ref);
			}
		};

	public:
		using iterator = detail::resolving_iterator<
			string, string_pool,
			detail::hash_set<reference_type>::iterator,
			resolve_string>;

		using const_iterator = detail::resolving_iterator<
			string, string_pool,
			detail::hash_set<reference_type>::const_iterator,
			resolve_string>;

		explicit string_set(
			string_pool& pool)
			: pool_(&pool)
		{
		}

		string_set(
			string_pool& pool,
			std::initializer_list<const char*> init)
			: pool_(&pool)
		{
			reserve(init.size());

			for (const char* text : init)
				insert(text);
		}

		template <class Iterator>
		string_set(
			string_pool& pool,
			Iterator first,
			Iterator last)
			: pool_(&pool)
		{
			for (; first != last; ++first)
				insert(*first);
		}

		string_set()
			: pool_(&string_pool::get_global_instance())
		{
		}

		string_set(
			std::initializer_list<const char*> init)
		{
			*this = string_set(string_pool::get_global_instance(), init);
		}

		template <class Iterator>
		string_set(
			Iterator first,
			Iterator last)
		{
			*this = string_set(string_pool::get_global_instance(), first, last);
		}

		[[nodiscard]]
		bool empty() const noexcept
		{
			return references_.empty();
		}

		[[nodiscard]]
		size_type size() const noexcept
		{
			return references_.size();
		}

		void reserve(size_type count)
		{
			references_.reserve(count);
		}

		void clear() noexcept
		{
			references_.clear();
		}

		reference_type insert(const char* text)
		{
			reference_type ref = pool_->append_string(text);

			references_.insert(ref);

			return ref;
		}

		reference_type insert(const string& str)
		{
			assert(str.pool() == pool_ && "string belongs to a different string_pool");

			references_.insert(str.reference());

			return str.reference();
		}

		reference_type insert(reference_type ref)
		{
			pool_->check_id(ref);

			references_.insert(ref);

			return ref;
		}

		bool erase(reference_type ref)
		{
			return references_.erase(ref) != 0;
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
			return references_.contains(ref);
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

		[[nodiscard]]
		string find(reference_type ref) const
		{
			if (!contains(ref))
				return {};

			return pool_->reconstruct_reference(ref);
		}

		[[nodiscard]]
		string find(const char* text) const
		{
			auto ref = pool_->find(text);

			if (ref == base::invalid_reference)
				return {};

			return find(ref);
		}

		[[nodiscard]]
		string find(const string& str) const
		{
			return find(str.reference());
		}

		void merge(const string_set& other)
		{
			assert(other.pool_ == pool_ && "cannot merge sets from different string_pools");

			for (auto ref : other.references_)
				references_.insert(ref);
		}

		string_set union_with(const string_set& other) const
		{
			assert(other.pool_ == pool_ && "cannot union with sets from different string_pools");

			string_set result(*pool_);

			result.merge(*this);
			result.merge(other);

			return result;
		}

		string_set intersection(const string_set& other) const
		{
			assert(other.pool_ == pool_ && "cannot intersect with sets from different string_pools");

			string_set result(*pool_);

			for (auto ref : references_)
			{
				if (other.contains(ref))
					result.references_.insert(ref);
			}

			return result;
		}

		string_set difference(const string_set& other) const
		{
			assert(other.pool_ == pool_ && "cannot differentiate from different string_pools");

			string_set result(*pool_);

			for (auto ref : references_)
			{
				if (!other.contains(ref))
					result.references_.insert(ref);
			}

			return result;
		}

		string_set& operator+=(const char* text)
		{
			insert(text);
			return *this;
		}

		string_set& operator+=(const string& str)
		{
			insert(str);
			return *this;
		}

		string_set& operator+=(reference_type ref)
		{
			insert(ref);
			return *this;
		}

		string_set& operator-=(const char* text)
		{
			erase(text);
			return *this;
		}

		string_set& operator-=(const string& str)
		{
			erase(str);
			return *this;
		}

		string_set& operator-=(reference_type ref)
		{
			erase(ref);
			return *this;
		}

		[[nodiscard]] 
		iterator begin() noexcept 
		{ 
			return { pool_, references_.begin() }; 
		}

		[[nodiscard]] 
		iterator end() noexcept 
		{ 
			return { pool_, references_.end() }; 
		}

		[[nodiscard]] 
		const_iterator begin() const noexcept 
		{ 
			return { pool_, references_.begin() }; 
		}

		[[nodiscard]] 
		const_iterator end() const noexcept 
		{ 
			return { pool_, references_.end() }; 
		}

	private:
		string_pool* pool_;

		detail::hash_set<reference_type> references_;
	};
}