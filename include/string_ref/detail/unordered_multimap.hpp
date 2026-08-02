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

namespace string_ref::detail
{
    /* @brief A compact unordered multimap using contiguous storage.
     *
     * Buckets contain 32-bit indices into a contiguous node array. There are
     * no per-element heap allocations, unlike a conventional node-based
     * std::unordered_multimap implementation.
     *
     * The container supports duplicate keys and is intentionally focused on
     * the operations needed by `string_pool` and similar data-oriented code.
     * Rehashing invalidates iterators. Insertion may invalidate iterators when
     * the node vector grows. Erasure invalidates all iterators.
     */
    template<
        typename Key,
        typename T,
        typename Hash = std::hash<Key>,
        typename KeyEqual = std::equal_to<Key>>
        class unordered_multimap
    {
    private:
        using index_type = std::uint32_t;

        static constexpr index_type invalid_index =
            std::numeric_limits<index_type>::max();

        struct node
        {
            std::pair<const Key, T> value;
            index_type next = invalid_index;

            template<typename K, typename V>
            node(K&& key, V&& value)
                : value(
                    std::forward<K>(key),
                    std::forward<V>(value))
            {
            }
        };

        static constexpr std::size_t initial_bucket_count = 8;

        [[nodiscard]]
        static std::size_t next_bucket_count(
            std::size_t requested) noexcept
        {
            std::size_t count = initial_bucket_count;

            while (count < requested)
            {
                if (count >
                    std::numeric_limits<std::size_t>::max() / 2)
                {
                    return requested;
                }

                count *= 2;
            }

            return count;
        }

        [[nodiscard]]
        std::size_t bucket_index(const Key& key) const
        {
            return hasher_(key) % buckets_.size();
        }

        [[nodiscard]]
        bool equal(
            const Key& lhs,
            const Key& rhs) const
        {
            return key_equal_(lhs, rhs);
        }

        void initialize_buckets()
        {
            if (buckets_.empty())
            {
                buckets_.assign(
                    initial_bucket_count,
                    invalid_index);
            }
        }

        void ensure_capacity_for_insert()
        {
            initialize_buckets();

            if (nodes_.size() >=
                static_cast<std::size_t>(invalid_index))
            {
                throw std::length_error(
                    "string_ref::detail::unordered_multimap exceeds 32-bit index capacity");
            }

            const float projected_load =
                static_cast<float>(size() + 1) /
                static_cast<float>(buckets_.size());

            if (projected_load > max_load_factor_)
            {
                rehash(buckets_.size() * 2);
            }
        }

        void rebuild_links()
        {
            std::fill(
                buckets_.begin(),
                buckets_.end(),
                invalid_index);

            for (index_type index = 0;
                index < nodes_.size();
                ++index)
            {
                node& current = nodes_[index];

                const std::size_t bucket =
                    bucket_index(current.value.first);

                current.next = buckets_[bucket];
                buckets_[bucket] = index;
            }
        }

        void erase_all_matching(const Key& key)
        {
            std::vector<node> replacement;
            replacement.reserve(nodes_.size());

            for (auto& current : nodes_)
            {
                if (equal(current.value.first, key))
                    continue;

                replacement.emplace_back(
                    current.value.first,
                    std::move(current.value.second));
            }

            nodes_.swap(replacement);
            rebuild_links();
        }

        void erase_single_at(index_type target)
        {
            std::vector<node> replacement;
            replacement.reserve(nodes_.size() - 1);

            for (index_type index = 0;
                index < nodes_.size();
                ++index)
            {
                if (index == target)
                    continue;

                auto& current = nodes_[index];

                replacement.emplace_back(
                    current.value.first,
                    std::move(current.value.second));
            }

            nodes_.swap(replacement);
            rebuild_links();
        }

    public:
        using key_type = Key;
        using mapped_type = T;
        using value_type = std::pair<const Key, T>;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using hasher = Hash;
        using key_equal = KeyEqual;

        class const_iterator;

        class iterator
        {
            friend class unordered_multimap;
            friend class const_iterator;

            unordered_multimap* map_ = nullptr;
            index_type index_ = invalid_index;
            std::optional<Key> filter_;

            iterator(
                unordered_multimap* map,
                index_type index)
                : map_(map), index_(index)
            {
            }

            iterator(
                unordered_multimap* map,
                index_type index,
                const Key& filter)
                : map_(map),
                index_(index),
                filter_(filter)
            {
                skip_non_matching();
            }

            void skip_non_matching()
            {
                if (!map_ || !filter_)
                    return;

                while (index_ != invalid_index &&
                    !map_->equal(
                        map_->nodes_[index_].value.first,
                        *filter_))
                {
                    index_ = map_->nodes_[index_].next;
                }
            }

        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = typename unordered_multimap::value_type;
            using difference_type = std::ptrdiff_t;
            using pointer = value_type*;
            using reference = value_type&;

            iterator() = default;

            reference operator*() const
            {
                return map_->nodes_[index_].value;
            }

            pointer operator->() const
            {
                return &map_->nodes_[index_].value;
            }

            iterator& operator++()
            {
                if (index_ != invalid_index)
                {
                    index_ =
                        map_->nodes_[index_].next;

                    skip_non_matching();
                }

                return *this;
            }

            iterator operator++(int)
            {
                iterator copy = *this;
                ++(*this);
                return copy;
            }

            friend bool operator==(
                const iterator& lhs,
                const iterator& rhs) noexcept
            {
                return lhs.map_ == rhs.map_ &&
                    lhs.index_ == rhs.index_;
            }

            friend bool operator!=(
                const iterator& lhs,
                const iterator& rhs) noexcept
            {
                return !(lhs == rhs);
            }
        };

        class const_iterator
        {
            friend class unordered_multimap;

            const unordered_multimap* map_ = nullptr;
            index_type index_ = invalid_index;
            std::optional<Key> filter_;

            const_iterator(
                const unordered_multimap* map,
                index_type index)
                : map_(map), index_(index)
            {
            }

            const_iterator(
                const unordered_multimap* map,
                index_type index,
                const Key& filter)
                : map_(map),
                index_(index),
                filter_(filter)
            {
                skip_non_matching();
            }

            void skip_non_matching()
            {
                if (!map_ || !filter_)
                    return;

                while (index_ != invalid_index &&
                    !map_->equal(
                        map_->nodes_[index_].value.first,
                        *filter_))
                {
                    index_ = map_->nodes_[index_].next;
                }
            }

        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = typename unordered_multimap::value_type;
            using difference_type = std::ptrdiff_t;
            using pointer = const value_type*;
            using reference = const value_type&;

            const_iterator() = default;

            const_iterator(const iterator& other)
                : map_(other.map_),
                index_(other.index_),
                filter_(other.filter_)
            {
            }

            reference operator*() const
            {
                return map_->nodes_[index_].value;
            }

            pointer operator->() const
            {
                return &map_->nodes_[index_].value;
            }

            const_iterator& operator++()
            {
                if (index_ != invalid_index)
                {
                    index_ =
                        map_->nodes_[index_].next;

                    skip_non_matching();
                }

                return *this;
            }

            const_iterator operator++(int)
            {
                const_iterator copy = *this;
                ++(*this);
                return copy;
            }

            friend bool operator==(
                const const_iterator& lhs,
                const const_iterator& rhs) noexcept
            {
                return lhs.map_ == rhs.map_ &&
                    lhs.index_ == rhs.index_;
            }

            friend bool operator!=(
                const const_iterator& lhs,
                const const_iterator& rhs) noexcept
            {
                return !(lhs == rhs);
            }

            friend bool operator==(
                const const_iterator& lhs,
                const iterator& rhs) noexcept
            {
                return lhs.map_ == rhs.map_ &&
                    lhs.index_ == rhs.index_;
            }

            friend bool operator==(
                const iterator& lhs,
                const const_iterator& rhs) noexcept
            {
                return rhs == lhs;
            }

            friend bool operator!=(
                const const_iterator& lhs,
                const iterator& rhs) noexcept
            {
                return !(lhs == rhs);
            }

            friend bool operator!=(
                const iterator& lhs,
                const const_iterator& rhs) noexcept
            {
                return !(lhs == rhs);
            }
        };

        unordered_multimap()
        {
            initialize_buckets();
        }

        explicit unordered_multimap(
            const hasher& hash)
            : hasher_(hash)
        {
            initialize_buckets();
        }

        unordered_multimap(
            const hasher& hash,
            const key_equal& equal)
            : hasher_(hash),
            key_equal_(equal)
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
            return static_cast<float>(size()) /
                static_cast<float>(buckets_.size());
        }

        [[nodiscard]]
        float max_load_factor() const noexcept
        {
            return max_load_factor_;
        }

        void max_load_factor(float value)
        {
            if (!(value > 0.0f))
                throw std::invalid_argument(
                    "max_load_factor must be greater than zero");

            max_load_factor_ = value;

            if (load_factor() > max_load_factor_)
            {
                rehash(
                    static_cast<size_type>(
                        static_cast<float>(size()) /
                        max_load_factor_) + 1);
            }
        }

        void reserve(size_type count)
        {
            const size_type requested =
                static_cast<size_type>(
                    static_cast<float>(count) /
                    max_load_factor_) + 1;

            rehash(requested);
            nodes_.reserve(count);
        }

        void rehash(size_type count)
        {
            count = next_bucket_count(count);

            buckets_.assign(
                count,
                invalid_index);

            rebuild_links();
        }

        void clear() noexcept
        {
            nodes_.clear();

            std::fill(
                buckets_.begin(),
                buckets_.end(),
                invalid_index);
        }

        iterator begin() noexcept
        {
            return nodes_.empty()
                ? end()
                : iterator(this, 0);
        }

        iterator end() noexcept
        {
            return iterator(this, invalid_index);
        }

        const_iterator begin() const noexcept
        {
            return nodes_.empty()
                ? end()
                : const_iterator(this, 0);
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

        template<typename K, typename V>
        iterator emplace(K&& key, V&& value)
        {
            ensure_capacity_for_insert();

            const std::size_t bucket =
                bucket_index(key);

            const index_type index =
                static_cast<index_type>(nodes_.size());

            nodes_.emplace_back(
                std::forward<K>(key),
                std::forward<V>(value));

            nodes_.back().next = buckets_[bucket];
            buckets_[bucket] = index;

            return iterator(this, index);
        }

        iterator insert(const value_type& value)
        {
            return emplace(value.first, value.second);
        }

        iterator insert(value_type&& value)
        {
            return emplace(value.first, std::move(value.second));
        }

        iterator find(const Key& key)
        {
            return iterator(this, find_index(key));
        }

        const_iterator find(const Key& key) const
        {
            return const_iterator(this, find_index(key));
        }

        [[nodiscard]]
        bool contains(const Key& key) const
        {
            return find_index(key) != invalid_index;
        }

        [[nodiscard]]
        size_type count(const Key& key) const
        {
            size_type result = 0;
            const std::size_t bucket = bucket_index(key);

            index_type index = buckets_[bucket];

            while (index != invalid_index)
            {
                if (equal(nodes_[index].value.first, key))
                    ++result;

                index = nodes_[index].next;
            }

            return result;
        }

        std::pair<iterator, iterator> equal_range(const Key& key)
        {
            const std::size_t bucket = bucket_index(key);

            return {
                iterator(this, buckets_[bucket], key),
                iterator(this, invalid_index, key)
            };
        }

        std::pair<const_iterator, const_iterator>
            equal_range(const Key& key) const
        {
            const std::size_t bucket = bucket_index(key);

            return {
                const_iterator(this, buckets_[bucket], key),
                const_iterator(this, invalid_index, key)
            };
        }

        size_type erase(const Key& key)
        {
            size_type removed = 0;

            for (const auto& current : nodes_)
            {
                if (equal(current.value.first, key))
                    ++removed;
            }

            if (removed != 0)
                erase_all_matching(key);

            return removed;
        }

        iterator erase(iterator position)
        {
            if (position.map_ != this ||
                position.index_ == invalid_index)
            {
                return end();
            }

            erase_single_at(position.index_);
            return end();
        }

    private:
        [[nodiscard]]
        index_type find_index(const Key& key) const
        {
            const std::size_t bucket = bucket_index(key);
            index_type index = buckets_[bucket];

            while (index != invalid_index)
            {
                if (equal(nodes_[index].value.first, key))
                    return index;

                index = nodes_[index].next;
            }

            return invalid_index;
        }

        std::vector<index_type> buckets_;
        std::vector<node> nodes_;
        hasher hasher_{};
        key_equal key_equal_{};
        float max_load_factor_ = 0.75f;
    };
}
