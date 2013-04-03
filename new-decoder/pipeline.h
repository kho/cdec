#ifndef _NEW_DECODER_PIPELINE_H_
#define _NEW_DECODER_PIPELINE_H_

#include <iostream>
#include <string>

#include <boost/scoped_ptr.hpp>
#include <boost/utility.hpp>

namespace boost { namespace program_options {
struct options_description;
struct variables_map;
} // namespace program_options
} // namespace boost

namespace pipeline {

// Types requently used in pipe definitions
typedef boost::program_options::options_description OptDesc;
typedef boost::program_options::variables_map VarMap;

struct Context;
struct Input;

// Dummy argument; plain old `void` does not play well with template
// arguments.
struct Void {};

// Result that may fail; `T` must be copyable and
// copy-assignable. Consider use it whenever the computation fails but
// the program should not crash. See the description of pipe
// operations to see the benefits and an example.
template <class T>
class Maybe {
 public:
  typedef T value_type;
  // Constructs a `Nothing`; don't use it; use `Nothing<T>()`.
  Maybe() : value_ptr_(NULL) {}
  // Constructs a `Just` by making a copy of the value; don't use it;
  // use `Just(value)`.
  explicit Maybe(const T &value) : value_ptr_(new T(value)) {}
  ~Maybe() { delete value_ptr_; }
  // Copy
  Maybe(const Maybe<T> &that) : value_ptr_(that.IsJust() ? new T(that.Value()) : NULL) {}
  // Swap
  void swap(Maybe<T> &that) {
    T *tmp = value_ptr_;
    value_ptr_ = that.value_ptr_;
    that.value_ptr_ = tmp;
  }
  // Assignment
  Maybe<T> &operator=(const Maybe<T> &that) {
    Maybe<T> tmp(that);
    this->swap(tmp);
    return *this;
  }
  // Tests
  bool IsNothing() const { return !value_ptr_; }
  bool IsJust() const { return value_ptr_; }
  // Access the value
  const T &Value() const { return *value_ptr_; }
  T &Value() { return *value_ptr_; }
  // Comparison
  bool operator==(const Maybe<T> that) const {
    if (IsNothing()) {
      return that.IsNothing();
    } else {
      return that.IsJust() && Value() == that.Value();
    }
  }
 private:
  T *value_ptr_;
};

template <class T>
std::ostream &operator<<(std::ostream &o, const Maybe<T> v) {
  if (v.IsNothing()) o << "Nothing";
  else o << "Just " << v.Value();
  return o;
}

template <class T>
std::istream &operator>>(std::istream &i, Maybe<T> &v) {
  std::string kind;
  i >> kind;
  if (kind == "Nothing") {
    if (v.IsJust()) {
      Maybe<T> t;
      v.swap(t);
    }
  } else if (kind == "Just") {
    if (v.IsJust()) {
      i >> v.Value();
    } else {
      int t;
      i >> t;
      Maybe<T> mt(t);
      v.swap(mt);
    }
  } else {
    i.setstate(std::ios_base::failbit);
  }
  return i;
}

// Factory functions of `Maybe`.
template <class T> inline Maybe<T> Nothing() { return Maybe<T>(); }
template <class T> inline Maybe<T> Just(const T &value) { return Maybe<T>(value); }

// Forward declarations
// Function
template <class T> struct Identity;     // t -> t
template <class F, class G> struct Compose; // (a -> b) -> (b -> c) -> (a -> c)
// Maybe monad
template <class T> struct Unit;         // t -> Maybe t
template <class F, class G> struct Bind; // (a -> Maybe b) -> (b -> Maybe c) -> (a -> Maybe c)
// template <class F> struct Fmap;         // Do we need this?
// Conditional branching
template <class If, class Then, class Else> struct Cond; // (a -> bool) -> (a -> b) -> (a -> b) -> (a -> b)

// Conceptually a pipe is a piece of code that receives some data,
// does some transformation and returns the result. More concretely, a
// pipe struct should define the following public members,
//
// - `itype`, the type of input data; and `otype`, the type of output
// data.
//
// - A `static void Register(OptDesc *)` function that registers program
// options. It is important to ensure `Register` only executes once,
// even when called multiple times. This can be achieved by defining a
// static bool variable inside the function to mark whether it has
// been executed.
//
// - One only constructor that takes `(const VarMap &, Context *)`,
// any preprocessing or loading side-data should happen here.
//
// - A `otype Apply(const Input &, Context *, itype) const` function
// that does the actual data processing. If `itype` is a pointer type,
// then the pipe owns the memory when `Apply` is called. Therefore, if
// the data is not further forwarded by returning, it should be freed
// before `Apply` returns. In most cases, a pipe does in-place
// modification of the input and returns it right away, where no
// freeing would be necessary. The `Apply` function should be as
// context independent as possible, which is one of the reason it is
// marked as `const`.
//
// Inheriting from `Pipe` does not guarantee any of the above
// requirements. Instead, it allows operator style composition, bind
// and conditionals, for defining composed pipes.
//
// Example: the following struct defines a pipe that keeps odd numbers
// but fails on even numbers.
//
// struct OddOnly : Pipe<OddOnly> {
//   typedef int itype;
//   typedef Maybe<int> otype;
//   static void Register(OptDesc *) {}
//   OddOnly(const VarMap &, Context *) {}
//   otype Apply(const Input &, Context *, itype arg) const {
//     if (arg % 2) return Just(arg);
//     return Nothing<int>();
//   }
// };
//
// See also: the declaration of `Input` and `Context`.
template <class F>
struct Pipe : boost::noncopyable {
  // Functional composition
  template <class G> struct comp { typedef Compose<F, G> type; };
  // Conditioned execution; when `If` does not hold, the input is
  // passed through.
  template <class If> struct on { typedef Cond<If, F, Identity<typename F::itype> > type; };
  // Monadic bind
  template <class G> struct bind { typedef Bind<F, G> type; };
  // Monadic conditioned execution; when `If` does not hold, the input
  // is passed through.
  template <class If> struct when { typedef Cond<If, F, Unit<typename F::itype> > type; };
};

// Identity pipe from `T` to `T`; simply passes the input data.
template <class T>
struct Identity : Pipe<Identity<T> > {
  typedef T itype;
  typedef T otype;
  static void Register(OptDesc */*opts*/) {}
  Identity(const VarMap &/*conf*/, Context */*context*/) {}
  T Apply(const Input &/*input*/, Context */*context*/, itype arg) const { return arg; }
};

// Pipe composition; results in a composed pipe that first processes
// data with `F` and then feeds the intermediate result to `G`. Since
// `F` is applied before `G`, `F::otype` should be compatible with
// `G::itype`.
template <class F, class G>
struct Compose : Pipe<Compose<F, G> > {
  typedef typename F::itype itype;
  typedef typename G::otype otype;
  static void Register(OptDesc *opts) {
    F::Register(opts);
    G::Register(opts);
  }
  Compose(const VarMap &conf, Context *context) : f_(conf, context), g_(conf, context) {}
  otype Apply(const Input &input, Context *context, itype arg) const {
    typename F::otype t = f_.Apply(input, context, arg);
    return g_.Apply(input, context, t);
  }
 private:
  F f_;
  G g_;
};

// Unit pipe wraps a value `x` of type `T` in the form of `Just(x)`,
// of type `Maybe<T>`.
template <class T>
struct Unit : Pipe<Unit<T> > {
  typedef T itype;
  typedef Maybe<T> otype;
  static void Register(OptDesc */*opts*/) {}
  Unit(const VarMap &/*conf*/, Context */*context*/) {}
  otype Apply(const Input &/*input*/, Context */*context*/, itype arg) const { return Just(arg); }
};

// Bind is just the `Maybe` monadic composition; `F` is first applied;
// if the result is not `Nothing`, the value inside `Just` will be
// then applied to `G`, returning another `Maybe`. Otherwise,
// `Nothing` with proper type is propagated. Therefore, `F::otype`
// must be compatible with `Maybe<G::itype>`.
//
// `Bind` is very useful for describing computation that might
// fail. For example, suppose we have defined a `PrintInt` pipe that
// prints an integer and passes the integer through in a `Just`. Then
// combined with `OddOnly` defined above, a pipe that prints only the
// odd numbers can be implemented as:
//
// Bind<OddOnly, PrintInt> (or equivalently, OddOnly::bind<PrintInt>::type)
//
// An actual use case is in the translator, where the input may not
// have any parse, and then no further operation should be applied;
// while if the input can be parsed, further operations such as
// computing statistics of the forest, or rescoring the forest should
// be carried out. The translator pipe (`FirstPassTranslate`) is a
// `Void` to `Maybe<Hypergraph *>` pipe. Pipes following it are from
// `Hypergraph *` to `Maybe<Hypergraph *>`. This allows us to assume
// valid input when implementing subsequent pipes and by binding the
// pipes, they are guarranteed to only execute when translator
// succeeds. (In the final stage though, we need to output an empty
// line, in the event of no translation. We should then use `Comp` to
// compose a pipe from `Maybe<Hypergraph *>` to `Maybe<Hypergraph *>`,
// to make sure we print the empty line when the translator gives
// `Nothing`.
template <class F, class G>
struct Bind : Pipe<Bind<F, G> > {
  typedef typename F::itype itype;
  typedef typename G::otype otype;
  static void Register(OptDesc *opts) {
    F::Register(opts);
    G::Register(opts);
  }
  Bind(const VarMap &conf, Context *context) : f_(conf, context), g_(conf, context) {}
  otype Apply(const Input &input, Context *context, itype arg) const {
    typename F::otype t = f_.Apply(input, context, arg);
    if (t.IsNothing())
      return Nothing<typename G::otype::value_type>();
    return g_.Apply(input, context, t.Value());
  }
 private:
  F f_;
  G g_;
};

// Lifts a pipe so that it can be bound, by converting `F::otype` to
// `Maybe<F::otype>`. For pipes that will not fail, it is better to
// define them as is and then 'lift' them to convert the output type
// to `Maybe` with this.
template <class F>
struct Lift : Pipe<Lift<F> > {
  typedef typename F::itype itype;
  typedef Maybe<typename F::otype> otype;
  static void Register(OptDesc *opts) { F::Register(opts); }
  Lift(const VarMap &conf, Context *context) : f_(conf, context) {}
  otype Apply(const Input &input, Context *context, itype arg) const {
    return Just(f_.Apply(input, context, arg));
  }
 private:
  F f_;
};

// Delays the construction of the pipe till the first call of
// `Apply`. Used in `Cond` to make sure `Then` and `Else` pipes are
// only constructed in accordance with branching.
template <class F>
struct Delayed : Pipe<Delayed<F> > {
  typedef typename F::itype itype;
  typedef typename F::otype otype;
  static void Register(OptDesc *opts) {
    F::Register(opts);
  }
  Delayed(const VarMap &conf, Context *context) : save_conf_(conf), save_context_(context), f_() {}
  otype Apply(const Input &input, Context *context, itype arg) const {
    if (!f_) f_.reset(new F(save_conf_, save_context_));
    return f_->Apply(input, context, arg);
  }
 private:
  const VarMap &save_conf_;
  Context *save_context_;
  mutable boost::scoped_ptr<F> f_;
};

// Conditional branching; `If` is the predicate, i.e. `If::otype`
// should be convertible to a bool. `Then`and `Else` should have
// identical `otype`s. All three pipes should have identical
// `itype`s. When `If` evaluates to true, the input is passed onto
// `Then`; otherwise it is passed onto `Else`. Further, the
// construction of `Then` and `Else` are delayed until their first
// application.
template <class If, class Then, class Else>
struct Cond : Pipe<Cond<If, Then, Else> > {
  typedef typename Then::itype itype;
  typedef typename Then::otype otype;
  static void Register(OptDesc *opts) {
    If::Register(opts);
    Then::Register(opts);
    Else::Register(opts);
  }
  Cond(const VarMap &conf, Context *context) : if_(conf, context), then_(conf, context), else_(conf, context) {}
  otype Apply(const Input &input, Context *context, itype arg) const {
    return if_.Apply(input, context, arg) ? then_.Apply(input, context, arg) : else_.Apply(input, context, arg);
  }
 private:
  If if_;
  Delayed<Then> then_;
  Delayed<Else> else_;
};

// // Repeated application of a integer parameterized function `F`,
// // i.e. F<1>::PIPE_COMP(F<2>)::...::PIPE_COMP(F<n>). `F` should have
// // compatible itype and otype, so that the calls can be chained.
// template <int n, template <int> class F>
// struct Repeat : Compose<Repeat<n - 1, F>, F<n> > {
//   typedef typename F<n>::itype itype;
//   typedef typename F<n>::otype otype;
//   Repeat(const VarMap &conf, Context *context) : Compose<Repeat<n - 1, F>, F<n> >(conf, context) {}
// };

// template <template <int> class F>
// struct Repeat<0, F> : Identity<typename F<0>::itype> {
//   typedef typename F<0>::itype itype;
//   typedef typename F<0>::otype otype;
//   Repeat(const VarMap &conf, Context *context) : Identity<typename F<0>::itype>(conf, context) {}
// };

// // Repeated application of a integer parameterized monadic function
// // `F`, i.e. F<1>::PIPE_BIND(F<2>)::...::PIPE_BIND(F<n>).
// template <int n, template <int> class F>
// struct Loop : Bind<Loop<n - 1, F>, F<n> > {
//   typedef typename F<n>::itype itype;
//   typedef typename F<n>::otype otype;
//   Loop(const VarMap &conf, Context *context) : Bind<Loop<n - 1, F>, F<n> >(conf, context) {}
// };

// template <template <int> class F>
// struct Loop<0, F> : Unit<typename F<0>::itype> {
//   typedef typename F<0>::itype itype;
//   typedef typename F<0>::otype otype;
//   Loop(const VarMap &conf, Context *context) : Unit<typename F<0>::itype>(conf, context) {}
// };

} // namespace pipeline

// Macros for easy use of operators; use carefully for obvious
// reasons.
#ifndef PIPE_COMP
#define PIPE_COMP(x) comp< x >::type
#endif  // PIPE_COMP

#ifndef PIPE_BIND
#define PIPE_BIND(x) bind< x >::type
#endif  // PIPE_BIND

#ifndef PIPE_ON
#define PIPE_ON(x) on< x >::type
#endif  // PIPE_ON

#ifndef PIPE_WHEN
#define PIPE_WHEN(x) when< x >::type
#endif  // PIPE_WHEN

#endif  // _NEW_DECODER_PIPELINE_H_
