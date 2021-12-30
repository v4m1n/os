#pragma once

#define NULL nullptr
typedef decltype(nullptr) nullptr_t;

using size_t = unsigned long int;
static_assert(sizeof(size_t) == 8);

