#ifndef _NEW_DECODER_GENERATE_HG_H_
#define _NEW_DECODER_GENERATE_HG_H_

#include <boost/scoped_ptr.hpp>

#include "pipeline.h"

struct Hypergraph;

namespace pipeline {
struct FirstPassTranslateImpl;
struct FirstPassTranslate : Pipe<FirstPassTranslate> {
 public:
  typedef Void itype;
  typedef Maybe<Hypergraph *> otype;
  static void Register(OptDesc *);
  FirstPassTranslate(const VarMap &, Context *);
  ~FirstPassTranslate();
  otype Apply(const Input &, Context *, itype) const;
 private:
  boost::scoped_ptr<FirstPassTranslateImpl> pimpl_;
};

} // namespace pipeline

#endif  // _NEW_DECODER_GENERATE_HG_H_
