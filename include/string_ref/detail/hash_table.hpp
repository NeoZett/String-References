#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <limits>
#include <new>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

// Comments will gradually be removed when they are moved to the documentation. The code is still in early development and subject to change.

// Understand me when I say this, and I will make this clear; I am learning about this very much as I am making this.
// Early adaptations may be subject to change through later development.

namespace string_ref::detail
{
	// ---- key extraction policies -------------------------------------
	//
	// A hash_table doesn't store "keys and mapped values" directly, it
	// stores a single Value per node and knows how to pull a Key back out
	// of that Value. That one indirection is what lets the same engine
	// back a map, a multimap and a set:
	//
	//   map/multimap  : Value = std::pair<const Key, T>, key = value.first
	//   set           : Value = Key,                     key = value itself

	// **** Simplification: Defines storage unit of a node and defines how to extract the key.

	template <typename Key>
	struct identity_key_of_value
	{
		[[nodiscard]]
		const Key& operator()(const Key& value) const noexcept
		{
			return value;
		}
	};

	template <typename Key, typename T>
	struct pair_key_of_value
	{
		[[nodiscard]]
		const Key& operator()(const std::pair<const Key, T>& value) const noexcept
		{
			return value.first;
		}
	};

	// ---- the engine -----------------------------------------------------
	//
	// Value          : what a node actually stores (Key, or pair<const Key, T>)
	// Key            : the lookup key type
	// KeyOfValue     : Value -> const Key& extractor (see above)
	// AllowDuplicates: false => map/set semantics (unique keys)
	//                  true  => multimap semantics (duplicate keys allowed)
	//
	// Storage is two flat vectors (buckets_ of indices, nodes_ of nodes) —
	// there is no per-element heap allocation, insertion/removal only ever
	// touches vector storage and intrusive singly-linked chains within it.

	// **** Simplification: Implements a hash table with open addressing and separate chaining using vectors for storage.

	template <
		typename Value, // Type inhibits both key and type; each node stores both.
		typename Key, // The type the key must be and is retrieved by.
		typename KeyOfValue, // A functor that extracts the key from a value.
		typename Hash = std::hash<Key>, // A functor that hashes the key.
		typename KeyEqual = std::equal_to<Key>, // A functor that compares two keys for equality.
		bool AllowDuplicates = false> // Whether the hash table allows duplicate keys (multimap semantics) or not (map/set semantics).
	class hash_table
	{
	private:
		using index_type = std::uint32_t; // The index key is a 32-bit unsigned integer, which is enough for most use cases and allows for a maximum of 4 billion elements in the hash table.

		static constexpr index_type invalid_index =
			std::numeric_limits<index_type>::max(); // Around 4 billion elements, the hash table will throw an exception if it exceeds this limit.

		struct node
		{
			Value value; // The value stored in the node, which can be either a key or a pair of key and value, depending on the type of hash table.
			index_type next = invalid_index; // The index of the next node in the chain, or invalid_index if this is the last node in the chain.

			template <typename... Args>
			explicit node(Args&&... args)
				: value(std::forward<Args>(args)...)
			{
			}

			// Value is frequently not *assignable* (e.g. std::pair<const Key, T>
			// has a const first member, so its operator= is deleted) — but it
			// is still copy- and move-*constructible*, so node's implicitly
			// generated copy/move constructors work fine and are left alone.
			// node's copy/move assignment operators end up implicitly deleted
			// automatically (because Value's are deleted); that's fine, since
			// relocation (erase_single_at) uses destroy + placement-new
			// construction rather than assignment.
		};

		static constexpr std::size_t initial_bucket_count = 8;


		// Calculates the next power of two greater than or equal to the requested bucket count, starting from the initial bucket count.
		// This ensures that the number of buckets is always a power of two, which allows for efficient hashing and indexing.
		[[nodiscard]]
		static std::size_t next_bucket_count(std::size_t requested) noexcept 
		{
			std::size_t count = initial_bucket_count;

			while (count < requested)
			{
				if (count > std::numeric_limits<std::size_t>::max() / 2)
					return requested;

				count *= 2;
			}

			return count;
		}

		[[nodiscard]]
		static const Key& key_of(const Value& value) noexcept
		{
			return KeyOfValue{}(value);
		}

		[[nodiscard]]
		std::size_t bucket_index(const Key& key) const
		{
			return hasher_(key) % buckets_.size();
		}

		[[nodiscard]]
		std::size_t bucket_index_of(const Value& value) const
		{
			return bucket_index(key_of(value));
		}

		[[nodiscard]]
		bool equal(const Key& lhs, const Key& rhs) const
		{
			return key_equal_(lhs, rhs);
		}

		void initialize_buckets()
		{
			if (buckets_.empty())
				buckets_.assign(initial_bucket_count, invalid_index);
		}

		void rebuild_links()
		{
			std::fill(buckets_.begin(), buckets_.end(), invalid_index);

			for (index_type index = 0; index < nodes_.size(); ++index)
			{
				node& current = nodes_[index];
				const std::size_t bucket = bucket_index_of(current.value);

				current.next = buckets_[bucket];
				buckets_[bucket] = index;
			}
		}

		void ensure_capacity_for_insert()
		{
			initialize_buckets();

			if (nodes_.size() >= static_cast<std::size_t>(invalid_index))
			{
				throw std::length_error(
					"string_ref::detail::hash_table exceeds 32-bit index capacity");
			}

			const float projected_load =
				static_cast<float>(size() + 1) /
				static_cast<float>(buckets_.size());

			if (projected_load > max_load_factor_)
				rehash(buckets_.size() * 2);
		}

		// Finds the link (either a bucket head or a node's `next` member)
		// that currently points at `target`, so it can be repointed.
		[[nodiscard]]
		index_type* find_link_to(index_type target)
		{
			const std::size_t bucket = bucket_index_of(nodes_[target].value);
			index_type* link = &buckets_[bucket];

			while (*link != target)
				link = &nodes_[*link].next;

			return link;
		}

		// O(chain length) removal: unlink `target`, then fill the hole by
		// moving the last node into its place (fixing up the one chain
		// that pointed at the old last index). No full-table rebuild.
		void erase_single_at(index_type target)
		{
			*find_link_to(target) = nodes_[target].next;

			const index_type last = static_cast<index_type>(nodes_.size() - 1);

			if (target != last)
			{
				*find_link_to(last) = target;

				nodes_[target].~node();
				::new (static_cast<void*>(&nodes_[target])) node(std::move(nodes_[last]));
			}

			nodes_.pop_back();
		}

		[[nodiscard]]
		index_type find_index(const Key& key) const
		{
			if (buckets_.empty())
				return invalid_index;

			index_type index = buckets_[bucket_index(key)];

			while (index != invalid_index)
			{
				if (equal(key_of(nodes_[index].value), key))
					return index;

				index = nodes_[index].next;
			}

			return invalid_index;
		}

	public:
		using key_type = Key;
		using value_type = Value;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using hasher = Hash;
		using key_equal = KeyEqual;

		class const_iterator;

		class iterator
		{
			friend class hash_table;
			friend class const_iterator;

			hash_table* table_ = nullptr;
			index_type index_ = invalid_index;
			std::optional<Key> filter_;

			iterator(hash_table* table, index_type index)
				: table_(table), index_(index)
			{
			}

			iterator(hash_table* table, index_type index, const Key& filter)
				: table_(table), index_(index), filter_(filter)
			{
				skip_non_matching();
			}

			void skip_non_matching()
			{
				if (!table_ || !filter_)
					return;

				while (index_ != invalid_index &&
					!table_->equal(key_of(table_->nodes_[index_].value), *filter_))
				{
					index_ = table_->nodes_[index_].next;
				}
			}

		public:
			using iterator_category = std::forward_iterator_tag;
			using value_type = typename hash_table::value_type;
			using difference_type = std::ptrdiff_t;
			using pointer = value_type*;
			using reference = value_type&;

			iterator() = default;

			reference operator*() const 
			{ 
				return table_->nodes_[index_].value; 
			}

			pointer operator->() const 
			{ 
				return &table_->nodes_[index_].value; 
			}

			// Unfiltered iterators (begin()/end()/find()) walk nodes_ in
			// flat storage order — cheap and correct regardless of which
			// bucket each node hashes to. Filtered iterators (produced by
			// equal_range) instead follow the intrusive bucket chain, so
			// they only ever visit nodes sharing that bucket/key.
			iterator& operator++()
			{
				if (index_ == invalid_index)
					return *this;

				if (filter_)
				{
					index_ = table_->nodes_[index_].next;
					skip_non_matching();
				}
				else
				{
					++index_;

					if (index_ >= table_->nodes_.size())
						index_ = invalid_index;
				}

				return *this;
			}

			iterator operator++(int)
			{
				iterator copy = *this;
				++(*this);
				return copy;
			}

			friend bool operator==(const iterator& lhs, const iterator& rhs) noexcept
			{
				return lhs.table_ == rhs.table_ && lhs.index_ == rhs.index_;
			}

			friend bool operator!=(const iterator& lhs, const iterator& rhs) noexcept
			{
				return !(lhs == rhs);
			}
		};

		class const_iterator
		{
			friend class hash_table;

			const hash_table* table_ = nullptr;
			index_type index_ = invalid_index;
			std::optional<Key> filter_;

			const_iterator(const hash_table* table, index_type index)
				: table_(table), index_(index)
			{
			}

			const_iterator(const hash_table* table, index_type index, const Key& filter)
				: table_(table), index_(index), filter_(filter)
			{
				skip_non_matching();
			}

			void skip_non_matching()
			{
				if (!table_ || !filter_)
					return;

				while (index_ != invalid_index &&
					!table_->equal(key_of(table_->nodes_[index_].value), *filter_))
				{
					index_ = table_->nodes_[index_].next;
				}
			}

		public:
			using iterator_category = std::forward_iterator_tag;
			using value_type = typename hash_table::value_type;
			using difference_type = std::ptrdiff_t;
			using pointer = const value_type*;
			using reference = const value_type&;

			const_iterator() = default;

			const_iterator(const iterator& other)
				: table_(other.table_), index_(other.index_), filter_(other.filter_)
			{
			}

			reference operator*() const 
			{
				return table_->nodes_[index_].value;
			}

			pointer operator->() const
			{
				return &table_->nodes_[index_].value;
			}

			const_iterator& operator++()
			{
				if (index_ == invalid_index)
					return *this;

				if (filter_)
				{
					index_ = table_->nodes_[index_].next;
					skip_non_matching();
				}
				else
				{
					++index_;

					if (index_ >= table_->nodes_.size())
						index_ = invalid_index;
				}

				return *this;
			}

			const_iterator operator++(int)
			{
				const_iterator copy = *this;
				++(*this);
				return copy;
			}

			friend bool operator==(const const_iterator& lhs, const const_iterator& rhs) noexcept
			{
				return lhs.table_ == rhs.table_ && lhs.index_ == rhs.index_;
			}

			friend bool operator!=(const const_iterator& lhs, const const_iterator& rhs) noexcept
			{
				return !(lhs == rhs);
			}

			friend bool operator==(const const_iterator& lhs, const iterator& rhs) noexcept
			{
				return lhs.table_ == rhs.table_ && lhs.index_ == rhs.index_;
			}

			friend bool operator==(const iterator& lhs, const const_iterator& rhs) noexcept
			{
				return rhs == lhs;
			}

			friend bool operator!=(const const_iterator& lhs, const iterator& rhs) noexcept
			{
				return !(lhs == rhs);
			}

			friend bool operator!=(const iterator& lhs, const const_iterator& rhs) noexcept
			{
				return !(lhs == rhs);
			}
		};

		hash_table() 
		{ 
			initialize_buckets(); 
		}

		explicit hash_table(const hasher& hash) 
			: hasher_(hash) 
		{ 
			initialize_buckets(); 
		}

		hash_table(const hasher& hash, const key_equal& equal)
			: hasher_(hash), key_equal_(equal)
		{
			initialize_buckets();
		}

		[[nodiscard]] 
		bool empty() const noexcept 
		{ 
			return nodes_.empty(); 
		}
		[[nodiscard]] 
		size_type size() const noexcept 
		{ 
			return nodes_.size(); 
		}

		[[nodiscard]] 
		size_type bucket_count() const noexcept 
		{ 
			return buckets_.size(); 
		}

		[[nodiscard]]
		float load_factor() const noexcept
		{
			return static_cast<float>(size()) / static_cast<float>(buckets_.size());
		}

		[[nodiscard]] 
		float max_load_factor() const noexcept 
		{
			return max_load_factor_;
		}

		void max_load_factor(float value)
		{
			if (!(value > 0.0f))
				throw std::invalid_argument("max_load_factor must be greater than zero");

			max_load_factor_ = value;

			if (load_factor() > max_load_factor_)
			{
				rehash(static_cast<size_type>(
					static_cast<float>(size()) / max_load_factor_) + 1);
			}
		}

		void reserve(size_type count)
		{
			const size_type requested =
				static_cast<size_type>(static_cast<float>(count) / max_load_factor_) + 1;

			rehash(requested);
			nodes_.reserve(count);
		}

		void rehash(size_type count)
		{
			count = next_bucket_count(count);
			buckets_.assign(count, invalid_index);
			rebuild_links();
		}

		void clear() noexcept
		{
			nodes_.clear();
			std::fill(buckets_.begin(), buckets_.end(), invalid_index);
		}

		iterator begin() noexcept 
		{
			return nodes_.empty() ? end() : iterator(this, 0);
		}

		iterator end() noexcept
		{
			return iterator(this, invalid_index);
		}

		const_iterator begin() const noexcept
		{
			return nodes_.empty() ? end() : const_iterator(this, 0);
		}

		const_iterator end() const noexcept
		{
			return const_iterator(this, invalid_index);
		}

		const_iterator cbegin() const noexcept
		{ 
			return begin();
		}

		const_iterator cend() const noexcept
		{
			return end();
		}

		// Constructs Value(args...) in place. If AllowDuplicates is false
		// and the resulting key already exists, nothing is inserted and
		// {iterator-to-existing, false} is returned — exactly like
		// std::unordered_map::emplace / std::unordered_set::emplace.
		// If AllowDuplicates is true, the element is always inserted and
		// the returned bool is always true (std::unordered_multimap style).
		template <typename... Args>
		std::pair<iterator, bool> emplace(Args&&... args)
		{
			node candidate(std::forward<Args>(args)...);

			if constexpr (!AllowDuplicates)
			{
				initialize_buckets();
				const index_type existing = find_index(key_of(candidate.value));

				if (existing != invalid_index)
					return { iterator(this, existing), false };
			}

			ensure_capacity_for_insert();

			const std::size_t bucket = bucket_index_of(candidate.value);
			const index_type index = static_cast<index_type>(nodes_.size());

			candidate.next = buckets_[bucket];
			nodes_.push_back(std::move(candidate));
			buckets_[bucket] = index;

			return { iterator(this, index), true };
		}

		std::pair<iterator, bool> insert(const value_type& value) { return emplace(value); }
		std::pair<iterator, bool> insert(value_type&& value) { return emplace(std::move(value)); }

		iterator find(const Key& key) { return iterator(this, find_index(key)); }
		const_iterator find(const Key& key) const { return const_iterator(this, find_index(key)); }

		[[nodiscard]] bool contains(const Key& key) const { return find_index(key) != invalid_index; }

		[[nodiscard]]
		size_type count(const Key& key) const
		{
			if (buckets_.empty())
				return 0;

			size_type result = 0;
			index_type index = buckets_[bucket_index(key)];

			while (index != invalid_index)
			{
				if (equal(key_of(nodes_[index].value), key))
					++result;

				index = nodes_[index].next;
			}

			return result;
		}

		std::pair<iterator, iterator> equal_range(const Key& key)
		{
			if (buckets_.empty())
				return { end(), end() };

			const std::size_t bucket = bucket_index(key);
			return { iterator(this, buckets_[bucket], key), iterator(this, invalid_index, key) };
		}

		std::pair<const_iterator, const_iterator> equal_range(const Key& key) const
		{
			if (buckets_.empty())
				return { end(), end() };

			const std::size_t bucket = bucket_index(key);
			return {
				const_iterator(this, buckets_[bucket], key),
				const_iterator(this, invalid_index, key)
			};
		}

		// Removes every element matching `key` (0 or 1 of them when
		// AllowDuplicates is false; 0..N when true). Returns the count
		// removed. Each removal is an O(chain length) unlink+swap-pop,
		// not a full rebuild.
		size_type erase(const Key& key)
		{
			size_type removed = 0;
			index_type index = find_index(key);

			while (index != invalid_index)
			{
				erase_single_at(index);
				++removed;
				index = find_index(key);
			}

			return removed;
		}

		// Erases a single element by iterator. Because removal is
		// implemented as unlink + swap-with-last, the erased slot may now
		// be occupied by a different (previously-last) element, so this
		// does not attempt to return "the next" iterator — it returns
		// end(), same as before.
		iterator erase(iterator position)
		{
			if (position.table_ != this || position.index_ == invalid_index)
				return end();

			erase_single_at(position.index_);
			return end();
		}

	private:
		std::vector<index_type> buckets_;
		std::vector<node> nodes_;
		hasher hasher_{};
		key_equal key_equal_{};
		float max_load_factor_ = 0.75f;
	};
}