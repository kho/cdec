#ifndef _NEW_DECODER_STAGES_H_
#define _NEW_DECODER_STAGES_H_

#include "pipeline.h"

#include "context.h"

#include <boost/lexical_cast.hpp>

struct Hypergraph;

namespace pipeline {

struct InitForestStage : Pipe<InitForestStage> {
  typedef Void itype;
  typedef Void otype;
  static void Register(OptDesc */*opts*/) {}
  InitForestStage(const VarMap &/*conf*/, Context */*context*/) {}
  otype Apply(const Input &/*input*/, Context *context, itype arg) const {
    context->stage = "Init.";
    return arg;
  }
};

template <int PASS>
struct RescoreStage : Pipe<RescoreStage<PASS> > {
  typedef Hypergraph * itype;
  typedef Maybe<Hypergraph *> otype;
  static void Register(OptDesc */*opts*/) {}
  RescoreStage(const VarMap &/*conf*/, Context */*context*/) {}
  otype Apply(const Input &/*input*/, Context *context, itype arg) const {
    context->stage = "RESCORING PASS #" + boost::lexical_cast<std::string>(PASS);
    return Just(arg);
  }
};

template <int PASS>
struct RescorePruneStage : Pipe<RescorePruneStage<PASS> > {
  typedef Hypergraph * itype;
  typedef Maybe<Hypergraph *> otype;
  static void Register(OptDesc */*opts*/) {}
  RescorePruneStage(const VarMap &/*conf*/, Context */*context*/) {}
  otype Apply(const Input &/*input*/, Context *context, itype arg) const {
    context->stage = "Pruned RESCORING PASS #" + boost::lexical_cast<std::string>(PASS);
    return Just(arg);
  }
};

struct ConstrainForestStage : Pipe<ConstrainForestStage> {
  typedef Hypergraph * itype;
  typedef Maybe<Hypergraph *> otype;
  static void Register(OptDesc */*opts*/) {}
  ConstrainForestStage(const VarMap &/*conf*/, Context */*context*/) {}
  otype Apply(const Input &/*input*/, Context *context, itype arg) const {
    context->stage = "Constr.";
    return Just(arg);
  }
};

struct OutputSentStage : Pipe<OutputSentStage> {
  typedef Hypergraph * itype;
  typedef Maybe<Hypergraph *> otype;
  static void Register(OptDesc */*opts*/) {}
  OutputSentStage(const VarMap &/*conf*/, Context */*context*/) {}
  otype Apply(const Input &/*input*/, Context *context, itype arg) const {
    context->stage = "Output";
    return Just(arg);
  }
};

}      // namespace pipeline

#endif  // _NEW_DECODER_STAGES_H_
