#pragma once

#include <initializer_list>
#include <string_ref/detail/set.hpp>
#include <string_ref/string_pool.hpp>

namespace string_ref::extensions
{
    // We are introducing a custom iterator where the *opterator gives a string object
    // and a reference() function gives the reference

    class string_set
    {
    public:

        using value_type = string;
        using reference_type = string_reference;
        using size_type = std::size_t;

    public:

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
            auto ref = pool_->find_reference(text);

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
        string find(reference_type ref) const
        {
            if (!contains(ref))
                return {};

            return string(ref, *pool_);
        }

        [[nodiscard]]
        string find(const char* text) const
        {
            auto ref = pool_->find_reference(text);

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
            for (auto ref : other.references_)
                references_.insert(ref);
        }

        string_set union_with(const string_set& other) const
        {
            string_set result(*pool_);

            result.merge(*this);
            result.merge(other);

            return result;
        }

        string_set intersection(const string_set& other) const
        {
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

        auto begin() noexcept
        {
            return references_.begin();
        }

        auto end() noexcept
        {
            return references_.end();
        }

        auto begin() const noexcept
        {
            return references_.begin();
        }

        auto end() const noexcept
        {
            return references_.end();
        }

    private:

        string_pool* pool_;

        detail::hash_set<reference_type> references_;
    };
}