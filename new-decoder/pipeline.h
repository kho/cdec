#ifndef _NEW_DECODER_PIPELINE_H_
#define _NEW_DECODER_PIPELINE_H_

#include <iostream>
#include <string>

#include <boost/scoped_ptr.hpp>
#include <boost/utility.hpp>

// Define in "conf.h"
struct Conf;
// Defined in "context.h".
struct Context;
// Defined in "input.h".
struct Input;

namespace pipeline {

// Dummy argument
struct Void {};

// Result that may fail; `T` must be copyable and copy-assignable.
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

// Allow operator style composition, bind and conditionals.
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

// Identity function from `T` to `T`
template <class T>
struct Identity : Pipe<Identity<T> > {
  typedef T itype;
  typedef T otype;
  static void Register(Conf *) {}
  explicit Identity(const Conf &) {}
  T Apply(const Input &, Context *, itype i) const { return i; }
};

// Function composition; `F` is applied before `G`. Thus `F::otype`
// should be compatible with `G::itype`.
template <class F, class G>
struct Compose : Pipe<Compose<F, G> > {
  typedef typename F::itype itype;
  typedef typename G::otype otype;
  static void Register(Conf *conf) {
    F::Register(conf);
    G::Register(conf);
  }
  // Store `conf` for delayed construction
  explicit Compose(const Conf &conf) : conf_(conf) {}
  otype Apply(const Input &input, Context *context, itype i) const {
    LazyInit();
    typename F::otype t = f_->Apply(input, context, i);
    return g_->Apply(input, context, t);
  }
 private:
  void LazyInit() const {
    if (!f_) f_.reset(new F(conf_));
    if (!g_) g_.reset(new G(conf_));
  }
  const Conf &conf_;
  mutable boost::scoped_ptr<F> f_;
  mutable boost::scoped_ptr<G> g_;
};

// Unit is just the monadic identity from `T` to `Maybe<T>`
template <class T>
struct Unit : Pipe<Unit<T> > {
  typedef T itype;
  typedef Maybe<T> otype;
  static void Register(Conf *) {}
  explicit Unit(const Conf &) {}
  otype Apply(const Input &, Context *, itype i) const { return Just(i); }
};

// Bind is just the `Maybe` monadic composition; `F` is first applied;
// if the result is not `Nothing`, its value will be then applied to
// `G`, returning another `Maybe`. Otherwise, `Nothing` with proper
// type is propagated. Therefore, `F::otype` must be compatible with
// `Maybe<G::itype>`.
template <class F, class G>
struct Bind : Pipe<Bind<F, G> > {
  typedef typename F::itype itype;
  typedef typename G::otype otype;
  static void Register(Conf *conf) {
    F::Register(conf);
    G::Register(conf);
  }
  // Store `conf` for delayed construction
  explicit Bind(const Conf &conf) : conf_(conf) {}
  otype Apply(const Input &input, Context *context, itype i) const {
    LazyInitF();
    typename F::otype t = f_->Apply(input, context, i);
    if (t.IsNothing())
      return Nothing<typename G::otype::value_type>();
    LazyInitG();
    return g_->Apply(input, context, t.Value());
  }
 private:
  void LazyInitF() const { if (!f_) f_.reset(new F(conf_)); }
  void LazyInitG() const { if (!g_) g_.reset(new G(conf_)); }
  const Conf &conf_;
  mutable boost::scoped_ptr<F> f_;
  mutable boost::scoped_ptr<G> g_;
};

// Conditional branching; `If` is predicate, i.e. `If::otype` should
// be convertible to a bool. `Then`and `Else` should have compatible
// `itype`s and `otype`s. Further, we are forwarding `itype` and
// `otype` from `Then`, therefore recursive pipes can only take the
// `Else` branch for the type system to work.
template <class If, class Then, class Else>
struct Cond : Pipe<Cond<If, Then, Else> > {
  typedef typename Then::itype itype;
  typedef typename Then::otype otype;
  static void Register(Conf *conf) {
    If::Register(conf);
    Then::Register(conf);
    Else::Register(conf);
  }
  explicit Cond(const Conf &conf) : conf_(conf) {}
  otype Apply(const Input &input, Context *context, itype i) const {
    LazyInitIf();
    bool pred = if_->Apply(input, context, i);
    if (pred) {
      LazyInitThen();
      return then_->Apply(input, context, i);
    } else {
      LazyInitElse();
      return else_->Apply(input, context, i);
    }
  }
 private:
  void LazyInitIf() const { if (!if_) if_.reset(new If(conf_)); }
  void LazyInitThen() const { if (!then_) then_.reset(new Then(conf_)); }
  void LazyInitElse() const { if (!else_) else_.reset(new Else(conf_)); }
  const Conf &conf_;
  mutable boost::scoped_ptr<If> if_;
  mutable boost::scoped_ptr<Then> then_;
  mutable boost::scoped_ptr<Else> else_;
};

// Repeated application of a integer parameterized function `F`,
// i.e. F<1>::PIPE_COMP(F<2>)::...::PIPE_COMP(F<n>). `F` should have
// compatible itype and otype, so that the calls can be chained.
template <int n, template <int> class F>
struct Repeat : Compose<Repeat<n - 1, F>, F<n> > {
  typedef typename F<n>::itype itype;
  typedef typename F<n>::otype otype;
  explicit Repeat(const Conf &conf) : Compose<Repeat<n - 1, F>, F<n> >(conf) {}
};

template <template <int> class F>
struct Repeat<0, F> : Identity<typename F<0>::itype> {
  typedef typename F<0>::itype itype;
  typedef typename F<0>::otype otype;
  explicit Repeat(const Conf &conf) : Identity<typename F<0>::itype>(conf) {}
};

// Repeated application of a integer parameterized monadic function
// `F`, i.e. F<1>::PIPE_BIND(F<2>)::...::PIPE_BIND(F<n>).
template <int n, template <int> class F>
struct Loop : Bind<Loop<n - 1, F>, F<n> > {
  typedef typename F<n>::itype itype;
  typedef typename F<n>::otype otype;
  explicit Loop(const Conf &conf) : Bind<Loop<n - 1, F>, F<n> >(conf) {}
};

template <template <int> class F>
struct Loop<0, F> : Unit<typename F<0>::itype> {
  typedef typename F<0>::itype itype;
  typedef typename F<0>::otype otype;
  explicit Loop(const Conf &conf) : Unit<typename F<0>::itype>(conf) {}
};

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
