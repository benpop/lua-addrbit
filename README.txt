Bitwise operations on 64-bit integers via void pointer.

Analogous to bit32 library in Lua 5.2, and supports the same operations.
http://www.lua.org/manual/5.2/manual.html#6.7

To turn a pointer-cum-64-bit-integer into an integer, number, or numerical string, I suggest my lua-int64 or lua-capi libraries.
	https://github.com/benpop/lua-int64
	https://github.com/benpop/lua-capi

Very likely needs C99 for an assured 64-bit integral type.  This is the default C version in the Makefile.

You need a 64-bit machine and OS for this to work.

Supports Lua 5.1 and 5.2.

NOTE:  This is copied and slightly modified from the source of the Lua 5.2 bitwise operations library*.  I do not claim this code as my own original work, and use it by the freedom of Lua's license.

* http://www.lua.org/source/5.2/lbitlib.c.html

((benpop))
