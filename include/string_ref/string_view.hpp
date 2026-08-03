#pragma once

#include <cstdint>
#include <string_ref/base.hpp>
#include <string_ref/string_pool.hpp>
#include <string_ref/string_pool_view.hpp>

namespace string_ref
{
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
                string_pool::get_global_instance())
        {
        }

        string_view(
            string_reference ref) noexcept
            : string_view(
                ref,
                string_pool::get_global_instance())
        {
        }

        string_view(
            const string_definition& def) noexcept
            : string_view(
                def,
                string_pool::get_global_instance())
        {
        }

        string_view(
            const_iterator value) noexcept
            : string_view(
                value,
                string_pool::get_global_instance())
        {
        }

        string_view(
            const_reverse_iterator value) noexcept
            : string_view(
                value,
                string_pool::get_global_instance())
        {
        }
    };

    template <typename T>
    string_view string_pool_view::make_string_view(T& value) const
    {
        return string_view(value, *this);
    }
}