#ifndef _NEW_DECODER_CONSTRAIN_HG_H_
#define _NEW_DECODER_CONSTRAIN_HG_H_

#include "pipeline.h"

class Hypergraph;

namespace pipeline {
struct InputHasRef : Pipe<InputHasRef> {
  typedef Hypergraph * itype;
  typedef bool otype;
  static void Register(OptDesc */*opts*/) {}
  InputHasRef(const VarMap &/*conf*/, Context */*context*/) {}
  otype Apply(const Input &input, Context *context, itype arg) const;
};

struct ConstrainHgWithRefImpl;
struct ConstrainHgWithRef : Pipe<ConstrainHgWithRef> {
  typedef Hypergraph * itype;
  typedef Maybe<Hypergraph *> otype;
  static void Register(OptDesc *);
  ConstrainHgWithRef(const VarMap &, Context *);
  ~ConstrainHgWithRef();
  otype Apply(const Input &, Context *, itype) const;
 private:
  boost::scoped_ptr<ConstrainHgWithRefImpl> pimpl_;
};
} // namespace pipeline

#endif  // _NEW_DECODER_CONSTRAIN_HG_H_
