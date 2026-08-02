#pragma once

#include <cstdint>
#include <cstring>
#include <vector>
#include <stdexcept>


/*************************\
* The single include file *
* implements all features *
\*************************/

// This is the amalgamated header. Find the repository here: 
// https://github.com/NeoZett/String-References


/**********************************************************************\
*    Overhead map:      | Reason:                                      *
* 1. `string_view`      | Stores reference, length, and text pointer.  *
*                       | It is intended to find results for searches  *
*                       | and iterations, without having to manually   *
*                       | load each object. Because of these fields,   *
*                       | the `string_view` can reach a size of around *
*                       | 24 bytes on a 64-bit ABI.                    *
* -------------------------------------------------------------------- *
* 2. `string`           | In its implementation, it is very simple;    *
*                       | it stores the reference (identification) and *
*                       | a pointer to its pool. Furthermore, it is    *
*                       | intendend to be used normally for any        *
*                       | function. It does, however, provide more     *
*                       | information and utilities than necessary.    *
*                       | Due to both fields, for the reference and    *
*                       | the pool, the size can reach around          *
*                       | 16 bytes on a 64-bit machine.                *
* -------------------------------------------------------------------- *
* 3. `string_reference` | This is an alias for a 32-bit unsigned       *
*                       | integer. Therefore it is optimal for light-  *
*                       | weight and efficient implementation, such    *
*                       | as a lexer or parser. A string_reference     *
*                       | (32-bit integer) should usually only         *
*                       | allocate 4 bytes on a 64-bit machine.        *
* ******************************************************************** *
*    Name:              | When to use:                                 *
* 1. `string_view`      | If performance does not matter, you can use  *
*                       | it for a unified interface to a search or    *
*                       | iteration. Otherwise, use a `string_pool`    *
*                       | or `string_pool_view` to retrieve relevant   *
*                       | information.                                 *
* -------------------------------------------------------------------- *
* 2. `string_pool`      | When you want to specify different scopes,   *
*                       | environments, or namespaces for strings.     *
*                       | If you use a reference on the wrong pool,    *
*                       | it will most likely point towards the wrong  *
*                       | string, which it will yield. Ensure that you *
*                       | use the correct references and strings on    *
*                       | the correct pool. Note that references       *
*                       | become invalid after clearing the pool.      *
* -------------------------------------------------------------------- *
* 3. `string_pool_view` | It is used for transformations, but is also  *
*                       | intended for pool iterations.                *
\**********************************************************************/


/// <summary>
/// An interned string pool providing compact 32-bit string references and zero-copy retrieval.
/// </summary>
namespace string_ref
{
    namespace detail
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
                        "string_ref::unordered_multimap exceeds 32-bit index capacity");
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

    /* @brief The `string_ref::base` implements all the base types
     *        that are used throughout the `string_ref` library.
     */
    namespace base
    {
        /* @brief The `id_type` is the base-type of references and
         *        identifications; it describes the type of every
         *        type identifying a string in a `string_pool`.
         */
        using id_type = std::uint32_t;

        /* @brief The `invalid_id` is the equivalent of the value of
         *        an invalid identification.
         */
        static constexpr id_type invalid_id =
            static_cast<id_type>(-1);

        class string_pool;

        /* @brief The `string_reference` is the minimal presence
         *        and identification of a string definition within
         *        a pool.
         */
        using string_reference = id_type;

        /* @brief The `invalid_reference` is the equivalent of the value of
         *        an invalid reference.
         */
        static constexpr id_type invalid_reference =
            static_cast<id_type>(-1);

        /* @brief The `string_hash` type is an unsigned 64-bit integer
         *        which enables deduplication logic in a C-string lookup
         *        map, using the string-hash as key to its identifier.
         */
        using string_hash = std::uint64_t;

        /* @brief The `lookup_table` type is the type by which the pool
         *        looks up a C-string for deduplication logic. It uses
         *        a string-hash as key to its identifier.
         */
        using lookup_type = detail::unordered_map<string_hash, string_reference>;
    }
    
    /* @brief The `string_reference` is the minimal presence
    *        and identification of a string definition within
    *        a pool.
    */
    using string_reference = base::string_reference;

    /* @brief The string points to a C-style string inside a pool.
     *        It provides features to retrieve the string and information
     *        about it.
     */
    class string
    {
    public:

        string() noexcept :
            id_(base::invalid_id),
            pool_(nullptr)
        {
        }

        string(
            const char* text);

        string(
            const char* text,
            string_pool& pool);

        [[nodiscard]]
        string_reference
            reference() const noexcept
        {
            return id_;
        }

        [[nodiscard]]
        base::id_type id() const noexcept
        {
            return id_;
        }

        [[nodiscard]]
        bool empty() const
        {
            return size() == 0;
        }

        [[nodiscard]]
        std::size_t size() const;

        [[nodiscard]]
        const char* c_str() const;

        [[nodiscard]]
        int compare(const char* other) const
        {
            return strcmp(c_str(), other);
        }

        [[nodiscard]]
        operator const char* () const
        {
            return c_str();
        }

        [[nodiscard]]
        operator bool() const
        {
            return !empty();
        }

        [[nodiscard]]
        explicit operator base::id_type() const noexcept
        {
            return id_;
        }

        [[nodiscard]]
        bool operator == (const char* other) const
        {
            if (!other) return false;
            return strcmp(c_str(), other) == 0;
        }

        [[nodiscard]]
        bool operator != (const char* other) const
        {
            if (!other) return false;
            return strcmp(c_str(), other) != 0;
        }

        [[nodiscard]]
        bool operator == (const string& other) const noexcept
        {
            return id_ == other.id_;
        }

        [[nodiscard]]
        bool operator != (const string& other) const noexcept
        {
            return id_ != other.id_;
        }

        [[nodiscard]]
        bool operator == (const base::id_type& id) const noexcept
        {
            return id_ == id;
        }

        [[nodiscard]]
        bool operator != (const base::id_type& id) const noexcept
        {
            return id_ != id;
        }

    private:

        string(
            base::id_type id,
            string_pool* pool)
            :
            id_(id),
            pool_(pool)
        {
        }

        base::id_type id_;
        string_pool* pool_;

        friend class string_pool;
    };

    /* @brief The `string_pool` optimizes and unifies into
     *        a singular namespace to retrieve strings.
     *        This minimizes allocations by storing each
     *        unique string only once.
     */
    class string_pool
    {
    public:

        string_pool() = default;

        struct string_definition
        {
            std::uint32_t offset;
            std::uint32_t length;
        };

        [[nodiscard]]
        static string_pool& instance() noexcept
        {
            static string_pool pool;
            return pool;
        }

        [[nodiscard]]
        string reconstruct_reference(
            const string_reference ref)
        {
            check_id(ref);

            return string(ref, this);
        }

        string_reference append_string(
            const char* text)
        {
            if (!text)
                throw std::invalid_argument("text cannot be null");

            const base::string_hash hash =
                hash_string(text);

            auto range =
                lookup_.equal_range(hash);

            for (auto it = range.first;
                it != range.second;
                ++it)
            {
                const string_reference ref =
                    it->second;

                const char* existing =
                    pool_.data() +
                    definitions_[ref].offset;

                if (std::strcmp(existing, text) == 0)
                    return ref;
            }

            const std::uint32_t length =
                static_cast<std::uint32_t>(std::strlen(text));

            const std::uint32_t offset =
                static_cast<std::uint32_t>(pool_.size());

            definitions_.push_back(
                {
                    offset,
                    length
                });

            pool_.insert(
                pool_.end(),
                text,
                text + length);

            pool_.push_back('\0');

            string_reference id =
                static_cast<std::uint32_t>(
                    definitions_.size() - 1);

            lookup_.emplace(hash, id);

            return id;
        }

        [[nodiscard]]
        string create_string(
            const char* text)
        {
            return string(
                append_string(text),
                this);
        }

        [[nodiscard]]
        const char* get(
            const string& str) const
        {
            check_id(str);

            return pool_.data() +
                definitions_[str.id_].offset;
        }

        [[nodiscard]]
        const char* get(
            const string_reference ref) const
        {
            check_id(ref);

            return pool_.data() +
                definitions_[ref].offset;
        }

        [[nodiscard]]
        std::size_t length(
            const string& str) const
        {
            check_id(str);

            return definitions_[str.id_].length;
        }

        [[nodiscard]]
        std::size_t length(
            const string_reference ref) const
        {
            check_id(ref);

            return definitions_[ref].length;
        }

        void reserve_strings(std::size_t count)
        {
            definitions_.reserve(count);
        }

        void reserve_pool(std::size_t bytes)
        {
            pool_.reserve(bytes);
        }

        [[nodiscard]]
        bool has_string(const string& str)
        {
            if (!str.pool_ || str.pool_ != this)
                return false;

            return true;
        }

        [[nodiscard]]
        bool contains(
            const char* text) const noexcept
        {
            if (!text)
                return false;

            const base::string_hash hash =
                hash_string(text);

            auto range =
                lookup_.equal_range(hash);

            for (auto it = range.first;
                it != range.second;
                ++it)
            {
                const string_reference ref =
                    it->second;

                const char* existing =
                    pool_.data() +
                    definitions_[ref].offset;

                if (std::strcmp(existing, text) == 0)
                    return true;
            }

            return false;
        }

        [[nodiscard]]
        string_reference find(
            const char* text) const noexcept
        {
            if (!text)
                return base::invalid_reference;

            const string_hash hash =
                hash_string(text);

            auto range =
                lookup_.equal_range(hash);

            for (auto it = range.first;
                it != range.second;
                ++it)
            {
                const string_reference ref =
                    it->second;

                const char* existing =
                    pool_.data() +
                    definitions_[ref].offset;

                if (std::strcmp(existing, text) == 0)
                    return ref;
            }

            return base::invalid_reference;
        }

        [[nodiscard]]
        std::size_t string_count() const noexcept
        {
            return definitions_.size();
        }

        [[nodiscard]]
        std::size_t size() const noexcept
        {
            return pool_.size();
        }

        [[nodiscard]]
        bool empty() const noexcept
        {
            return definitions_.empty();
        }

        void clear()
        {
            definitions_.clear();
            pool_.clear();
            lookup_.clear();
        }

    private:

        static base::string_hash hash_string(
            const char* text) noexcept
        {
            constexpr base::string_hash offset =
                14695981039346656037ull;

            constexpr base::string_hash prime =
                1099511628211ull;

            base::string_hash hash = offset;

            while (*text)
            {
                hash ^= static_cast<unsigned char>(*text);
                hash *= prime;
                ++text;
            }

            return hash;
        }

        void check_id(
            const string& str) const
        {
            if (!str.pool_)
                throw std::runtime_error(
                    "string doesn't have an pool");

            if (str.pool_ != this)
                throw std::runtime_error(
                    "string belongs to another pool");

            if (str.id_ >= definitions_.size())
                throw std::out_of_range(
                    "invalid string identifier");
        }

        void check_id(
            const string_reference ref) const
        {
            if (ref >= definitions_.size())
                throw std::out_of_range(
                    "invalid reference identifier");
        }

        std::vector<char> pool_;
        std::vector<string_definition> definitions_;
        base::lookup_type lookup_;

        friend class string;
        friend class string_pool_view;
        friend struct string_view;
    };

    inline string::string(
        const char* text)
        : id_(base::invalid_id),
        pool_(nullptr)
    {
        if (!text)
            throw std::invalid_argument("text cannot be null");

        *this = string_pool::instance().create_string(text);
    }

    inline string::string(
        const char* text,
        string_pool& pool)
        : id_(invalid_id),
        pool_(nullptr)
    {
        if (!text)
            throw std::invalid_argument("text cannot be null");

        *this = pool.create_string(text);
    }

    inline std::size_t string::size() const
    {
        if (!pool_)
            return 0;

        return pool_->length(*this);
    }

    inline const char* string::c_str() const
    {
        if (!pool_)
            return "";

        return pool_->get(*this);
    }

    inline string_reference append_string(
        const char* text)
    {
        return string_pool::instance().append_string(text);
    }

    inline string_reference append_string(
        const char* text,
        string_pool& pool)
    {
        return pool.append_string(text);
    }

    [[nodiscard]]
    inline string create_string(
        const char* text)
    {
        return string_pool::instance().create_string(text);
    }

    [[nodiscard]]
    inline string create_string(
        const char* text,
        string_pool& pool)
    {
        return pool.create_string(text);
    }

    [[nodiscard]]
    inline string reconstruct_reference(
        const string_reference ref)
    {
        return string_pool::instance().reconstruct_reference(ref);
    }

    [[nodiscard]]
    inline string reconstruct_reference(
        const string_reference ref,
        string_pool& pool)
    {
        return pool.reconstruct_reference(ref);
    }

    inline const char* get(
        const string& str)
    {
        return string_pool::instance().get(str);
    }

    inline const char* get(
        const string_reference ref)
    {
        return string_pool::instance().get(ref);
    }

    inline const char* get(
        const string& str,
        string_pool& pool)
    {
        return pool.get(str);
    }

    inline const char* get(
        const string_reference ref,
        string_pool& pool)
    {
        return pool.get(ref);
    }

    inline std::size_t length(
        const string& str)
    {
        return string_pool::instance().length(str);
    }

    inline std::size_t length(
        const string_reference ref)
    {
        return string_pool::instance().length(ref);
    }

    inline std::size_t length(
        const string& str,
        string_pool& pool)
    {
        return pool.length(str);
    }

    inline std::size_t length(
        const string_reference ref,
        string_pool& pool)
    {
        return pool.length(ref);
    }

    inline void clear()
    {
        string_pool::instance().clear();
    }

    inline void clear(
        string_pool& pool)
    {
        pool.clear();
    }

    inline string_pool& static_pool()
    {
        return string_pool::instance();
    }

    struct string_view;

    /* @brief The `string_pool_view` is made to iterate and view all
     *        strings in a `string_pool`.
     */
    class string_pool_view
    {
    public:

        using string_definition = string_pool::string_definition;
        using container_type = std::vector<string_definition>;
        using const_iterator = container_type::const_iterator;
        using const_reverse_iterator = container_type::const_reverse_iterator;

        string_pool_view() noexcept
        {
            pool_ = &string_pool::instance();
        }

        string_pool_view(
            string_pool& pool) noexcept
            : pool_(&pool)
        {
        }

        [[nodiscard]]
        const char* retrieve_string(
            const string_definition& def) const noexcept
        {
            return pool_->pool_.data() + def.offset;
        }

        [[nodiscard]]
        const char* retrieve_string(
            const_iterator& def) const noexcept
        {
            return pool_->pool_.data() + def->offset;
        }

        [[nodiscard]]
        const char* retrieve_string(
            const_reverse_iterator& def) const noexcept
        {
            return pool_->pool_.data() + def->offset;
        }

        [[nodiscard]]
        string_reference
            find_reference(const string_definition& def) const noexcept
        {
            return pool_->find(retrieve_string(def));
        }

        [[nodiscard]]
        string_reference
            find_reference(const_iterator& def) const noexcept
        {
            return pool_->find(retrieve_string(def));
        }

        [[nodiscard]]
        string_reference
            find_reference(const_reverse_iterator& def) const noexcept
        {
            return pool_->find(retrieve_string(def));
        }

        [[nodiscard]]
        string_reference
            find_reference(const char* text) const noexcept
        {
            return pool_->find(text);
        }

        template <typename T>
        string_view make_string_view(T& value) const;

        [[nodiscard]]
        const string_definition& front() noexcept
        {
            return pool_->definitions_.front();
        }

        [[nodiscard]]
        const string_definition& back() noexcept
        {
            return pool_->definitions_.back();
        }

        [[nodiscard]]
        const_iterator begin() const noexcept
        {
            return pool_->definitions_.begin();
        }

        [[nodiscard]]
        const_reverse_iterator rbegin() const noexcept
        {
            return pool_->definitions_.rbegin();
        }

        [[nodiscard]]
        const_iterator end() const noexcept
        {
            return pool_->definitions_.end();
        }

        [[nodiscard]]
        const_reverse_iterator rend() const noexcept
        {
            return pool_->definitions_.rend();
        }

        [[nodiscard]]
        std::size_t size() noexcept
        {
            return pool_->definitions_.size();
        }

        [[nodiscard]]
        bool empty() noexcept
        {
            return pool_->definitions_.empty();
        }

    private:

        string_pool* pool_;

        friend struct string_view;
    };

    /* @brief The `string_view` object is made to introspect
     *        search or iteration results.
     */
    struct string_view
    {
        using string_definition = string_pool::string_definition;
        using const_iterator = string_pool_view::const_iterator;
        using const_reverse_iterator =
            string_pool_view::const_reverse_iterator;

        bool found = false;
        string_reference ref = base::invalid_reference;
        const char* text = nullptr;
        std::uint32_t length = 0;

        string_view(
            const char* text,
            string_pool_view& view) noexcept
        {
            string_reference ref = view.find_reference(text);

            if (ref == base::invalid_reference)
                return;

            found = true;
            this->ref = ref;
            this->text = view.pool_->get(ref);
            length = view.pool_->definitions_[ref].length;
        }

        string_view(
            string_reference ref,
            string_pool_view& view) noexcept
        {
            if (ref >= view.pool_->definitions_.size())
                return;

            found = true;
            this->ref = ref;
            this->text = view.pool_->get(ref);
            length = view.pool_->definitions_[ref].length;
        }

        string_view(
            const string_definition& def,
            string_pool_view& view) noexcept
        {
            text = view.retrieve_string(def);

            string_reference ref = view.find_reference(text);

            if (ref == base::invalid_reference)
            {
                text = nullptr;
                return;
            }

            found = true;
            this->ref = ref;
            length = def.length;
        }

        string_view(
            const_iterator value,
            string_pool_view& view) noexcept
        {
            const string_definition& def = *value;

            text = view.retrieve_string(def);

            string_reference ref = view.find_reference(text);

            if (ref == base::invalid_reference)
            {
                text = nullptr;
                return;
            }

            found = true;
            this->ref = ref;
            length = def.length;
        }

        string_view(
            const_reverse_iterator value,
            string_pool_view& view) noexcept
        {
            const string_definition& def = *value;

            text = view.retrieve_string(def);

            string_reference ref = view.find_reference(text);

            if (ref == base::invalid_reference)
            {
                text = nullptr;
                return;
            }

            found = true;
            this->ref = ref;
            length = def.length;
        }

        string_view(
            const char* text,
            string_pool& pool) noexcept
        {
            string_pool_view view(pool);
            *this = string_view(text, view);
        }

        string_view(
            string_reference ref,
            string_pool& pool) noexcept
        {
            string_pool_view view(pool);
            *this = string_view(ref, view);
        }

        string_view(
            const string_definition& def,
            string_pool& pool) noexcept
        {
            string_pool_view view(pool);
            *this = string_view(def, view);
        }

        string_view(
            const_iterator value,
            string_pool& pool) noexcept
        {
            string_pool_view view(pool);
            *this = string_view(value, view);
        }

        string_view(
            const_reverse_iterator value,
            string_pool& pool) noexcept
        {
            string_pool_view view(pool);
            *this = string_view(value, view);
        }

        string_view(
            const char* text) noexcept
            : string_view(
                text,
                string_pool::instance())
        {
        }

        string_view(
            string_reference ref) noexcept
            : string_view(
                ref,
                string_pool::instance())
        {
        }

        string_view(
            const string_definition& def) noexcept
            : string_view(
                def,
                string_pool::instance())
        {
        }

        string_view(
            const_iterator value) noexcept
            : string_view(
                value,
                string_pool::instance())
        {
        }

        string_view(
            const_reverse_iterator value) noexcept
            : string_view(
                value,
                string_pool::instance())
        {
        }
    };

    template <typename T>
    string_view string_pool_view::make_string_view(T& value) const
    {
        return string_view(value, *this);
    }
}