#pragma once

#include <cstddef>
#include <cstring>
#include <string_ref/base.hpp>

namespace string_ref
{
    class string_pool;

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
        explicit operator bool() const
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
}