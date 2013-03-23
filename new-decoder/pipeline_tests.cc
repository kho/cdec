#include "pipeline.h"

#include <cassert>
#include <iostream>
#include <string>
#include <sstream>

using namespace std;
using namespace pipeline;

void TestMaybe() {
  typedef Maybe<int> mi;
  ostringstream o;
  mi t;
  o << t << " " << t.IsNothing() << " " << t.IsJust() << endl;
  t = Just(0);
  o << t << " " << t.IsNothing() << " " << t.IsJust() << " " << t.Value() << endl;
  t.Value() = 2;
  o << t << " " << t.IsNothing() << " " << t.IsJust() << " " << t.Value() << endl;;
  string s(o.str());
  assert(s == "Nothing 1 0\nJust 0 0 1 0\nJust 2 0 1 2\n");
  istringstream i("Nothing Just 1 Just 2 Nothing Fail");
  i >> t;
  assert(i.good() && t.IsNothing());
  i >> t;
  assert(i.good() && t == Just(1));
  i >> t;
  assert(i.good() && t == Just(2));
  i >> t;
  assert(i.good() && t == Nothing<int>());
  i >> t;
  assert(i.fail());
  cerr << "TestMaybe: pass" << endl;
}

// Mock input and context
namespace boost { namespace program_options {
struct options_description {};
struct variables_map {};
} }

struct Input {};
struct Context {};

// Shorter macros
#define COMP PIPE_COMP
#define ON PIPE_ON
#define BIND PIPE_BIND
#define WHEN PIPE_WHEN

struct IntPair {
  int a, b;
  IntPair(int c, int d) : a(c), b(d) {}
};

struct First : Pipe<First> {
  typedef IntPair itype;
  typedef int otype;
  static void Register(OptDesc *) {}
  explicit First(const VarMap &, Context *) {}
  otype Apply(const Input &input, Context *context, itype arg) const { return arg.a; }
};

struct Second : Pipe<Second> {
  typedef IntPair itype;
  typedef int otype;
  static void Register(OptDesc *) {}
  explicit Second(const VarMap &, Context *) {}
  otype Apply(const Input &input, Context *context, itype arg) const { return arg.b; }
};

struct IsZero : Pipe<IsZero> {
  typedef int itype;
  typedef bool otype;
  static void Register(OptDesc *conf) {}
  explicit IsZero(const VarMap &conf, Context *context) {}
  otype Apply(const Input &input, Context *context, itype arg) const { return arg == 0; }
};

typedef First::COMP(IsZero) FirstIsZero;

struct IsOrdered : Pipe<IsOrdered> {
  typedef IntPair itype;
  typedef bool otype;
  static void Register(OptDesc *) {}
  explicit IsOrdered(const VarMap &, Context *) {}
  otype Apply(const Input &input, Context *context, itype arg) const {
    return arg.a <= arg.b;
  }
};

template <class T, class S>
struct MkNothing : Pipe<MkNothing<T, S> > {
  typedef T itype;
  typedef Maybe<S> otype;
  static void Register(OptDesc *) {}
  explicit MkNothing(const VarMap &, Context *) {}
  otype Apply(const Input &input, Context *context, itype arg) const {
    return Nothing<S>();
  }
};

template <class F>
typename F::otype run(typename F::itype arg) {
  OptDesc opts;
  VarMap vm;
  Context context;
  F::Register(&opts);
  F f(vm, &context);
  return f.Apply(Input(), &context, arg);
}

struct Add1 : Pipe<Add1> {
  typedef int itype;
  typedef int otype;
  static void Register(OptDesc *conf) {}
  Add1(const VarMap &conf, Context *context) {}
  otype Apply(const Input &input, Context *context, itype arg) const { return arg + 1; }
};

struct Times2 : Pipe<Times2> {
  typedef int itype;
  typedef int otype;
  static void Register(OptDesc *conf) {}
  Times2(const VarMap &conf, Context *context) {}
  otype Apply(const Input &input, Context *context, itype arg) const { return arg * 2; }
};

struct IsEven : Pipe<IsEven> {
  typedef int itype;
  typedef bool otype;
  static void Register(OptDesc *conf) {}
  IsEven(const VarMap &conf, Context *context) {}
  otype Apply(const Input &input, Context *context, itype arg) const { return arg % 2 == 0; }
};

struct IsOdd : Pipe<IsOdd> {
  typedef int itype;
  typedef bool otype;
  static void Register(OptDesc *conf) {}
  IsOdd(const VarMap &conf, Context *context) {}
  otype Apply(const Input &input, Context *context, itype arg) const { return arg % 2 != 0; }
};

template <int n>
struct AddN : Pipe<AddN<n> > {
  typedef int itype;
  typedef int otype;
  static void Register(OptDesc *conf) {}
  AddN(const VarMap &conf, Context *context) {}
  otype Apply(const Input &input, Context *context, itype arg) const { return arg + n; }
};

template <int n>
struct AddNTop60 : Pipe<AddNTop60<n> > {
  typedef int itype;
  typedef Maybe<int> otype;
  static void Register(OptDesc *conf) {}
  AddNTop60(const VarMap &conf, Context *context) {}
  otype Apply(const Input &input, Context *context, itype arg) const {
    if (arg + n < 60) return Just(arg + n);
    else return Nothing<int>();
  }
};

// The following should result in compile error because of recursive call.
// struct Rec : Cond<IsZero, Add1, Rec> {
//   typedef Cond<IsZero, Add1, Rec> base;
//   Rec(const VarMap &conf, Context *context) : base(conf) {}
// };

void TestPipe() {
  assert(run<FirstIsZero>(IntPair(0, 1)));
  assert(!run<FirstIsZero>(IntPair(1, 0)));
  int a[] = {1, 2, 3, 4, 5, 11, 22, 33, 44};
  int n = sizeof(a) / sizeof(int);
  for (int i = 0; i < n; ++i) {
    assert(run<Add1::ON(IsOdd)::COMP(IsEven)>(a[i]));
    assert(run<Add1::ON(IsEven)::COMP(IsOdd)>(a[i]));
    int v = a[i] + 1;
    if (v % 2 == 0) v *= 2;
    if (v % 2) v += 1;
    assert(run<Add1::COMP(Times2::ON(IsEven))::COMP(Add1::ON(IsOdd))>(a[i]) == v);
    assert(run<Add1::COMP(Times2::ON(IsEven)::COMP(Add1::ON(IsOdd)))>(a[i]) == v);
    v = a[i] + 1;
    if (v % 2) v += 1;
    assert(run<Add1::COMP(Times2::ON(IsEven)::COMP(Add1)::ON(IsOdd))>(a[i]) == v);
  }

  do {
    int x = run<Repeat<10, AddN> >(0);
    assert(x == 55);
  } while (false);

  do {
    Maybe<int> x = run<Loop<10, AddNTop60> >(0);
    assert(x == Just(55));
    x = run<Loop<10, AddNTop60> >(5);
    assert(x.IsNothing());
  } while (false);

  for (int i = 0; i < n; ++i) {
    Maybe<int> t1, t2;

    t1 = Just(a[i]);
    if (a[i] % 2)
      t1.Value() += 1;
    t2 = run<Add1::COMP(Unit<int>)::WHEN(IsOdd)>(a[i]);
    assert(t1 == t2);

    t1 = Nothing<int>();
    t2 = run<MkNothing<int, int>::BIND(Add1::COMP(Unit<int>)::WHEN(IsOdd))>(a[i]);
    assert(t1 == t2);
  }
  cerr << "TestFunc: pass" << endl;
}


int main(int argc, char *argv[]) {
  TestMaybe();
  TestPipe();
  return 0;
}
