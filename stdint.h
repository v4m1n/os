#pragma once

using int8_t   = char;
using int16_t  = short;
using int32_t  = int;
using int64_t  = long int;
using ssize_t  = long int;
using intptr_t = long int;


using uint8_t   = unsigned char;
using uint16_t  = unsigned short;
using uint32_t  = unsigned int;
using uint64_t  = unsigned long int;
using size_t    = unsigned long int;
using uintptr_t = unsigned long int;

static_assert(sizeof(int8_t) == 1);
static_assert(sizeof(int16_t) == 2);
static_assert(sizeof(int32_t) == 4);
static_assert(sizeof(int64_t) == 8);
static_assert(sizeof(ssize_t) == 8);
static_assert(sizeof(intptr_t) == 8);

static_assert(sizeof(uint8_t) == 1);
static_assert(sizeof(uint16_t) == 2);
static_assert(sizeof(uint32_t) == 4);
static_assert(sizeof(uint64_t) == 8);
static_assert(sizeof(size_t) == 8);
static_assert(sizeof(uintptr_t) == 8);
