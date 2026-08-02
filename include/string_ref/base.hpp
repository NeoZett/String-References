#pragma once

#include <cstdint>
#include <string_ref/detail/multimap.hpp>

/// <summary>
/// An interned string pool providing compact 32-bit string references and zero-copy retrieval.
/// </summary>
namespace string_ref
{
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
        using lookup_type = detail::hash_multimap<string_hash, string_reference>;
    }

    /* @brief The `string_reference` is the minimal presence
     *        and identification of a string definition within
     *        a pool.
     */
    using string_reference = base::string_reference;
}