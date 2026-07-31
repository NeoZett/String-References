#pragma once

#include <vector>
#include <string_ref/base.hpp>
#include <string_ref/string_pool.hpp>

namespace string_ref
{
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
}