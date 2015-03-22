#ifndef BASE_MACROS_H_
#define BASE_MACROS_H_

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

#endif  // BASE_MACROS_H_
