# C++ String References

*An API to efficiently and structurally handle strings, using 4 byte allocations on a 64-bit ABI.*

## Abstract 💡

This project was made for fun, becoming a github repository after many
iterations. The project consists of three layers:

1. String references and identifications. These allocate only 4 bytes
   on a 64-bit ABI, which is because they are aliases for an unsigned
   32-bit integer.

2. The string object itself coupled with the pool; in other terms, objects
   that handle string interaction and lifetime. The string object stores both 
   the id of the string and a pointer to its pool, and thusly it allocates
   around 16 bytes on a 64-bit ABI instead of only 4 bytes like the string
   reference does.

3. String- and pool views. The pool view is intended for iterations,
   introspections, and searches. Likewise, the string-view stores
   a string reference and string pointer. It also stores the string
   length as an unsigned 32-bit integer.
   
   Because these fields are on the same object, the string-view allocates
   around 24 bytes on a 64-bit ABI. If you don't use the string-view object,
   you have to manually retrieve relevant information using the string pool
   and/or string pool view objects. This is more efficient because you are
   only retrieving relevant information.

## String Pool and String Pool View 📊

The string pool is a container that stores strings and their corresponding
references. It is responsible for managing the lifetime of the strings and
providing access to them through their references. The string pool is designed
to be efficient and minimize memory usage, while also providing fast access to
strings. Using a multi-map, it deduces the string reference from the string hash,
and then retrieves the string from the pool using the string reference.

The string pool view is an object that provides a view of the strings in a string
pool. It allows for iteration over the strings in the pool and provides access to
their references and data. The string pool view is designed to be efficient and
minimize memory usage, while also providing fast access to strings.

It is important to note that the string pool view does not own the strings in the
pool, and therefore does not manage their lifetime. Instead, it provides a read-only
view of the strings in the pool, allowing for efficient access to their data without
the overhead of copying or managing the strings themselves.

## String Reference and String View 🔎

The string reference is an alias for an unsigned 32-bit integer that represents
a string in a string pool. It is used to identify and access strings and
information about them without storing the actual string data. This saves many
allocations whereas a string can allocate around 24 bytes on a 64-bit ABI, while
a string reference only allocates 4 bytes.

The string objects is much alike the string reference, but it stores a pointer to
the pool and the identification of the string. This allows for efficient access to
the string's contents and information about it, while also minimizing memory usage.
Because it has both fields, the string reference and a pointer to the pool, it
allocates around 16 bytes on a 64-bit ABI, which is more than the string reference
but still less than a string object.

The string view is an object that provides a view of a string by owning information
about the string in itself. It stores a string reference, a pointer to the string
data, and the length of the string. This allows for efficient access to the string's
contents and information about it, while also minimizing memory usage. Because it has
all three fields, the string view allocates around 24 bytes on a 64-bit ABI. This is
often an unnecessary allocation and unwanted, but it is useful if you don't want to
retrieve information about the string yourself.

## String set, String Map, and String Multi-Map 🗺️

These containers are built on top of the string pool and provide efficient
access to items through their references.

The string set is a container that stores unique string references, while
the string map is a container that stores key-value pairs of string references
and a template type name.

The string multi-map is a container that allows multiple values to be associated
with a single key, and is built on top of the hash-table implementation like
the other containers.

These containers are designed to be efficient and provide fast access to
strings, or through string, using their references, while also minimizing
memory usage.

## License 📄

Find more information about the license at https://opensource.org/licenses/MIT

MIT License

Copyright (c) 2006 Neo-Erik Östlund-Zetterberg

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

---

For more information, see the documentation in the [`documentation.txt`](documentation.txt) file.
This file was purposefully made a text file for easy reference and compatibility.

You can also see the documentation on this wiki-page.