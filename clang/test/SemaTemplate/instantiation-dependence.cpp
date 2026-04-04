// RUN: %clang_cc1 -std=c++26 -verify %s

// Ensure we substitute into instantiation-dependent but non-dependent
// constructs. The poster-child for this is...
template<class ...> using void_t = void;

namespace PR24076 {
  template<class T> T declval();
  struct s {};

  template<class T,
           class = void_t<decltype(declval<T>() + 1)>>
    void foo(T) {} // expected-note {{invalid operands to binary expression}}

  void f() {
    foo(s{}); // expected-error {{no matching function}}
  }

  template<class T,
           class = void_t<decltype(declval<T>() + 1)>> // expected-error {{invalid operands to binary expression}}
  struct bar {};

  bar<s> bar; // expected-note {{in instantiation of}}
}

namespace PR33655 {
  struct One { using x = int; };
  struct Two { using y = int; };

  template<typename T, void_t<typename T::x> * = nullptr> int &func() {}
  template<typename T, void_t<typename T::y> * = nullptr> float &func() {}

  int &test1 = func<One>();
  float &test2 = func<Two>();

  template<class ...Args> struct indirect_void_t_imp { using type = void; };
  template<class ...Args> using indirect_void_t = typename indirect_void_t_imp<Args...>::type;

  template<class T> void foo() {
    int check1[__is_void(indirect_void_t<T>) == 0 ? 1 : -1]; // "ok", dependent
    int check2[__is_void(void_t<T>) == 0 ? 1 : -1]; // expected-error {{array with a negative size}}
  }
}

namespace PR46791 { // also PR45782
  template<typename T, typename = void>
  struct trait {
    static constexpr int specialization = 0;
  };

  template<typename T>
  struct trait<T, void_t<typename T::value_type>> { // expected-note {{matches}}
    static constexpr int specialization = 1;
  };

  template<typename T>
  struct trait<T, void_t<typename T::element_type>> { // expected-note {{matches}}
    static constexpr int specialization = 2;
  };

  struct A {};
  struct B { typedef int value_type; };
  struct C { typedef int element_type; };
  struct D : B, C {};

  static_assert(trait<A>::specialization == 0);
  static_assert(trait<B>::specialization == 1);
  static_assert(trait<C>::specialization == 2);
  static_assert(trait<D>::specialization == 0); // expected-error {{ambiguous partial specialization}}
}

namespace TypeQualifier {
  // Ensure that we substitute into an instantiation-dependent but
  // non-dependent qualifier.
  template<int> struct A { using type = int; };
  template<typename T> A<sizeof(sizeof(T::error))>::type f() {} // expected-note {{'int' cannot be used prior to '::'}}
  int k = f<int>(); // expected-error {{no matching}}
}

namespace MemberOfInstantiationDependentBase {
  template<typename T> struct A { template<int> void f(int); };
  template<typename T> struct B { using X = A<T>; };
  template<typename T> struct C1 : B<int> {
    using X = typename C1::X;
    void f(X *p) {
      p->f<0>(0);
      p->template f<0>(0);
    }
  };
  template<typename T> struct C2 : B<int> {
    using X = typename C2<T>::X;
    void f(X *p) {
      p->f<0>(0);
      p->template f<0>(0);
    }
  };
  void q(C1<int> *c) { c->f(0); }
  void q(C2<int> *c) { c->f(0); }
}

namespace GH8740 {
  struct A { typedef int T; };
  template<int> struct U { typedef int T; };
  template<typename> struct S {
    A a;
    int n = decltype(a)::T();
    int m = U<sizeof(a)>::T();
  };
  S<char> s;
} // namespace GH8740

namespace NonInstDependentArgs1 {
  template<typename T, typename = void> struct X;
  template<typename T> struct X<T, void_t<char>> {}; // expected-note  {{previous}}
  template<typename T> struct X<T, void_t<void>> {}; // expected-error {{redefinition}}
} // namespace NonInstDependentArgs1

namespace NonInstDependentArgs2 {
  template<typename T, typename = void> struct X;
  template<typename T> struct X<T, void_t<T, void>> {};
  template<typename T> struct X<T, void_t<T, char>> {};
} // namespace NonInstDependentArgs2

namespace Level1 {
  template<typename T, typename = void> struct X;
  template<typename T> struct X<T, void_t<T>> {};
  template<typename T> struct X<T, void_t<T*>> {};
} // namespace Level1

namespace Level2 {
  template<typename T, typename = void> struct X;
  template<typename T> struct X<T, void_t<void_t<T>>> {};
  template<typename T> struct X<T, void_t<void_t<T*>>> {};
} // namespace Level2

namespace IndirectAlias1 {
  template<class T> using alias = void_t<T>;
  template<typename T, typename = void> struct X;
  template<typename T> struct X<T, void_t<T>> {}; // expected-note  {{previous}}
  template<typename T> struct X<T, alias<T>> {};  // expected-error {{redefinition}}
} // namspace IndirectAlias1

namespace IndirectAlias2 {
  template<class T, class> using alias1 = T;
  template<class T, class U> using alias2 = alias1<T, U>;
  template<typename T, typename = void> struct X;
  template<typename T> struct X<T, T> {};
  template<typename T> struct X<T, alias1<T, T>> {}; // expected-note  {{previous}}
  template<typename T> struct X<T, alias2<T, T>> {}; // expected-error {{redefinition}}
} // namespaceIndirectAlias2

namespace PackIndexing {
  // FIXME: This should not be a redefinition.
  template<class ...Ts> using alias = Ts...[0];
  template<typename T, typename = void> struct X;
  template<typename T> struct X<T, T> {};                          // expected-note  {{previous}}
  template<typename T> struct X<T, alias<T, typename T::type>> {}; // expected-error {{redefinition}}
} // namespace PackIndexing

namespace DeclType {
  template<typename T, typename = void> struct X;
  template<typename T> struct X<T, decltype(void())> {};
  template<typename T> struct X<T, decltype(void_t<typename T::type>())> {}; // FIXME-note  {{previous}}
  template<typename T> struct X<T, decltype(void_t<typename T::type>())> {}; // FIXME-error {{redefinition}}
} // namespace DeclType

namespace UnaryTransformDecay {
  template<class T, class U = void> struct X;
  template<class T> struct X<T, __decay(int[T()])> {}; // FIXME-note  {{previous}}
  template<class T> struct X<T, __decay(int[T()])> {}; // FIXME-error {{redefinition}}
} // namespace UnaryTransformDecay
