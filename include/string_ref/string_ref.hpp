#pragma once


/*******************************************************\
* The `string_ref` include file implements all features *
* through their respective header files                 *
\*******************************************************/

// Find the repository here: https://github.com/NeoZett/String-References


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
*                       | as a lexer or parser. A `string_reference`   *
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
*                       | intended for pool iterations. It can be used *
*                       | for both.                                    *
\**********************************************************************/


#include <string_ref/base.hpp>
#include <string_ref/string.hpp>
#include <string_ref/string_pool.hpp>
#include <string_ref/string_pool_view.hpp>
#include <string_ref/string_view.hpp>