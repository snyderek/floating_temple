#ifndef BASE_MACROS_H_
#define BASE_MACROS_H_

#ifdef __GNUC__
#define UNUSED_ATTRIBUTE __attribute__((unused))
#else
#define UNUSED_ATTRIBUTE
#endif

// From Google C++ Style Guide
// http://google-styleguide.googlecode.com/svn/trunk/cppguide.xml
// Accessed February 8, 2013
//
// A macro to disallow the copy constructor and operator= functions
// This should be used in the private: declarations for a class
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&);               \
  void operator=(const TypeName&)

// Example from the Google C++ Style Guide:
//
// class Foo {
//  public:
//   Foo(int f);
//   ~Foo();
//
//  private:
//   DISALLOW_COPY_AND_ASSIGN(Foo);
// };

// A macro to compute the number of elements in a statically allocated array.
#define ARRAYSIZE(a) (sizeof (a) / sizeof (a)[0])

namespace floating_temple {

template<bool>
struct CompileAssert {
};

}  // namespace floating_temple

// Copied from googletest (third_party/gmock-1.7.0/fused-src/gtest/gtest.h).
#define COMPILE_ASSERT(expr, msg) \
  typedef floating_temple::CompileAssert<static_cast<bool>(expr)> \
      msg[static_cast<bool>(expr) ? 1 : -1] UNUSED_ATTRIBUTE

#endif  // BASE_MACROS_H_
