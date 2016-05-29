// Please don't add a copyright notice to this file. It contains source code
// owned by other copyright holders (used under license).

#ifndef BASE_MACROS_H_
#define BASE_MACROS_H_

// A macro to disallow the copy constructor and operator= functions.
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&) = delete; \
  void operator=(const TypeName&) = delete

// A macro to compute the number of elements in a statically allocated array.
#define ARRAYSIZE(a) (sizeof(a) / sizeof((a)[0]))

#endif  // BASE_MACROS_H_
