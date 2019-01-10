#ifndef PZE_BASE_HPP
#define PZE_BASE_HPP

#pragma once

#include <SDKDDKVer.h>

#ifndef _WINDOWS_
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#endif

// C RunTime Header Files
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include <string>
#include <optional>
#include <algorithm>

/*
 * Compute the length of an array with constant length.  (Use of this method
 * with a non-array pointer will not compile.)
 *
 * Beware of the implicit trailing '\0' when using this with string constants.
 */
template <typename T, size_t N> constexpr size_t ArrayLength(T (&aArr)[N]) {
  return N;
}

template <typename T, size_t N> constexpr T *ArrayEnd(T (&aArr)[N]) {
  return aArr + ArrayLength(aArr);
}


/**
 * std::equal has subpar ergonomics.
 */

template <typename T, typename U, size_t N>
bool ArrayEqual(const T (&a)[N], const U (&b)[N]) {
  return std::equal(a, a + N, b);
}

template <typename T, typename U>
bool ArrayEqual(const T *const a, const U *const b, const size_t n) {
  return std::equal(a, a + n, b);
}

#endif