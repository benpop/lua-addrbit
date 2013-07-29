#ifndef _MSC_VER
#include <stdint.h>
#endif

#include <stddef.h>
#include <stdlib.h>

#include "lua.h"
#include "lauxlib.h"


#define ADDR_NBITS 64


#ifndef strtoui64
#ifdef _MSC_VER
#define strtoui64 _strtoui64
#else
#define strtoui64 strtoull
#endif
#endif


typedef uintptr_t ab_uint;


#define ALLONES   (~(((~(ab_uint)0) << (ADDR_NBITS - 1)) << 1))

/* macro to trim extra bits */
#define trim(x)   ((x) & ALLONES)


/* builds a number with 'n' ones (1 <= n <= ADDR_NBITS) */
#define mask(n)   (~((ALLONES << 1) << ((n) - 1)))


#define push_addr(L,r) lua_pushlightuserdata(L, (void *)(r))


static ab_uint to_addr (lua_State *L, int idx) {
  switch (lua_type(L, idx)) {
    case LUA_TNUMBER: {
      return (ab_uint)lua_tonumber(L, idx);
    }
    case LUA_TSTRING: {
      size_t l;
      const char *s = lua_tolstring(L, idx, &l);
      int base = luaL_optint(L, 2, 0);
      return strtoui64(s, NULL, base);
    }
    default: {
      return (ab_uint)lua_topointer(L, idx);
    }
  }
}


static ab_uint andaux (lua_State *L) {
  int i, n = lua_gettop(L);
  ab_uint r = ~(ab_uint)0;
  for (i = 1; i <= n; i++)
    r &= to_addr(L, i);
  return trim(r);
}


static int ab_and (lua_State *L) {
  ab_uint r = andaux(L);
  push_addr(L, r);
  return 1;
}


static int ab_test (lua_State *L) {
  ab_uint r = andaux(L);
  lua_pushboolean(L, r != 0);
  return 1;
}


static int ab_or (lua_State *L) {
  int i, n = lua_gettop(L);
  ab_uint r = 0;
  for (i = 1; i <= n; i++)
    r |= to_addr(L, i);
  push_addr(L, trim(r));
  return 1;
}


static int ab_xor (lua_State *L) {
  int i, n = lua_gettop(L);
  ab_uint r = 0;
  for (i = 1; i <= n; i++)
    r ^= to_addr(L, i);
  push_addr(L, trim(r));
  return 1;
}


static int ab_not (lua_State *L) {
  ab_uint r = ~to_addr(L, 1);
  push_addr(L, trim(r));
  return 1;
}


static int ab_shift (lua_State *L, ab_uint r, int i) {
  if (i < 0) {  /* shift right? */
    i = -i;
    r = trim(r);
    if (i >= ADDR_NBITS) r = 0;
    else r >>= i;
  }
  else {  /* shift left */
    if (i >= ADDR_NBITS) r = 0;
    else r <<= i;
    r = trim(r);
  }
  push_addr(L, r);
  return 1;
}


static int ab_lshift (lua_State *L) {
  return ab_shift(L, to_addr(L, 1), luaL_checkint(L, 2));
}


static int ab_rshift (lua_State *L) {
  return ab_shift(L, to_addr(L, 1), -luaL_checkint(L, 2));
}


static int ab_arshift (lua_State *L) {
  ab_uint r = to_addr(L, 1);
  int i = luaL_checkint(L, 2);
  if (i < 0 || !(r & ((ab_uint)1 << (ADDR_NBITS - 1))))
    return ab_shift(L, r, -i);
  else {  /* arithmetic shift for 'negative' number */
    if (i >= ADDR_NBITS) r = ALLONES;
    else
      r = trim((r >> i) | ~(~(ab_uint)0 >> i));  /* add signal bit */
    push_addr(L, r);
    return 1;
  }
}


static int ab_rot (lua_State *L, int i) {
  ab_uint r = to_addr(L, 1);
  i &= (ADDR_NBITS - 1);  /* i = i % NBITS */
  r = trim(r);
  r = (r << i) | (r >> (ADDR_NBITS - i));
  push_addr(L, trim(r));
  return 1;
}


static int ab_lrot (lua_State *L) {
  return ab_rot(L, luaL_checkint(L, 2));
}


static int ab_rrot (lua_State *L) {
  return ab_rot(L, -luaL_checkint(L, 2));
}


/*
** get field and width arguments for field-manipulation functions,
** checking whether they are valid.
** ('luaL_error' called without 'return' to avoid later warnings about
** 'width' being used uninitialized.)
*/
static int fieldargs (lua_State *L, int farg, int *width) {
  int f = luaL_checkint(L, farg);
  int w = luaL_optint(L, farg + 1, 1);
  luaL_argcheck(L, 0 <= f, farg, "field cannot be negative");
  luaL_argcheck(L, 0 < w, farg + 1, "width must be positive");
  if (f + w > ADDR_NBITS)
    luaL_error(L, "trying to access non-existent bits");
  *width = w;
  return f;
}


static int ab_extract (lua_State *L) {
  int w;
  ab_uint r = to_addr(L, 1);
  int f = fieldargs(L, 2, &w);
  r = (r >> f) & mask(w);
  push_addr(L, r);
  return 1;
}


static int ab_replace (lua_State *L) {
  int w;
  ab_uint r = to_addr(L, 1);
  ab_uint v = to_addr(L, 2);
  int f = fieldargs(L, 3, &w);
  int m = mask(w);
  v &= m;  /* erase bits outside given width */
  r = (r & ~(m << f)) | (v << f);
  push_addr(L, r);
  return 1;
}


static const luaL_Reg lib[] = {
  {"arshift", ab_arshift},
  {"band", ab_and},
  {"bnot", ab_not},
  {"bor", ab_or},
  {"bxor", ab_xor},
  {"btest", ab_test},
  {"extract", ab_extract},
  {"lrotate", ab_lrot},
  {"lshift", ab_lshift},
  {"replace", ab_replace},
  {"rrotate", ab_rrot},
  {"rshift", ab_rshift},
  {NULL, NULL}
};


#if LUA_VERSION_NUM == 502
#define luaL_register(L,n,l) luaL_newlib(L,l)
#endif


int luaopen_addrbit (lua_State *L) {
  luaL_register(L, "addrbit", lib);
  return 1;
}
