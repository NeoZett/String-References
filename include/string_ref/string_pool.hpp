#pragma once

#include <cstdint>
#include <cstring>
#include <vector>
#include <stdexcept>
#include <string_ref/base.hpp>
#include <string_ref/string.hpp>

namespace string_ref
{
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
        : id_(base::invalid_id),
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
}