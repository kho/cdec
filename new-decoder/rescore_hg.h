#ifndef _NEW_DECODER_RESCORE_HG_H_
#define _NEW_DECODER_RESCORE_HG_H_

#include <boost/program_options.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include "pipeline.h"
#include "context.h"
#include "input.h"

namespace pipeline {

extern bool SILENT;

bool ShouldRescorePred(const VarMap &, int);
bool ShouldSummarizePred(const VarMap &, int);
bool ShouldPrunePred(const VarMap &, int);

template <int PASS>
struct ShouldRescore : Pipe<ShouldRescore<PASS> > {
  typedef Hypergraph * itype;
  typedef bool otype;

  static void Register(OptDesc */*opts*/) {}

  ShouldRescore(const VarMap &conf, Context */*context*/) :
      result_(ShouldRescorePred(conf, PASS)) {}

  otype Apply(const Input &/*input*/, Context */*context*/, itype /*arg*/) const {
    return result_;
  }

 private:
  bool result_;
};


template <int PASS>
struct ShouldSummarize : Pipe<ShouldSummarize<PASS> > {
  typedef Hypergraph * itype;
  typedef bool otype;

  static void Register(OptDesc */*opts*/) {}

  ShouldSummarize(const VarMap &conf, Context */*context*/) :
      result_(ShouldSummarizePred(conf, PASS)) {}

  otype Apply(const Input &/*input*/, Context */*context*/, itype /*arg*/) const {
    return result_;
  }

 private:
  bool result_;
};


template <int PASS>
struct ShouldPrune : Pipe<ShouldPrune<PASS> > {
  typedef Hypergraph * itype;
  typedef bool otype;

  static void Register(OptDesc */*opts*/) {}

  ShouldPrune(const VarMap &conf, Context */*context*/) :
      result_(ShouldPrunePred(conf, PASS)) {}

  otype Apply(const Input &/*input*/, Context */*context*/, itype /*arg*/) const {
    return result_;
  }

 private:
  bool result_;
};


struct RescoreHgImpl {
  RescoreHgImpl(const VarMap &, Context *, int);
  ~RescoreHgImpl();
  Hypergraph *Apply(const Input &, Context *, Hypergraph *arg) const;

 private:
  struct Rescorer;
  boost::scoped_ptr<Rescorer> r_;
};

template <int PASS>
struct RescoreHg : Pipe<RescoreHg<PASS> > {
  typedef Hypergraph * itype;
  typedef Maybe<Hypergraph *> otype;

  static void Register(OptDesc *opts) {
    static bool added_;
    if (added_) return;
    added_ = true;
    if (PASS == 1) Register1(opts);
    else RegisterN(opts);
  }

  RescoreHg(const VarMap &conf, Context *context) :
      pimpl_(new RescoreHgImpl(conf, context, PASS)) {}

  otype Apply(const Input &input, Context *context, itype arg) const {
    return Just(pimpl_->Apply(input, context, arg));
  }

 private:
  static void Register1(OptDesc *opts) {
    namespace po = boost::program_options;
    opts->add_options()
        ("weights,w", po::value<std::string>(), "Feature weights file (initial forest / pass 1)")
        ("feature_function,F", po::value<std::vector<std::string> >()->composing(), "Pass 1 additional feature function(s) (-L for list)")
        ("intersection_strategy,I", po::value<std::string>()->default_value("cube_pruning"), "Pass 1 intersection strategy for incorporating finite-state features; values include Cube_pruning, Full, Fast_cube_pruning, Fast_cube_pruning_2")
        ("cubepruning_pop_limit,K", po::value<int>()->default_value(200), "Max number of pops from the candidate heap at each node")
        ;
  }

  static void RegisterN(OptDesc *opts) {
    std::string p = boost::lexical_cast<std::string>(PASS);
    namespace po = boost::program_options;
    opts->add_options()
        (("weights" + p).c_str(), po::value<std::string>(), ("Optional pass " + p).c_str())
        (("feature_function" + p).c_str(), po::value<std::vector<std::string> >()->composing(), ("Optional pass " + p).c_str())
        (("intersection_strategy" + p).c_str(), po::value<std::string>()->default_value("cube_pruning"), ("Optional pass " + p).c_str())
        ;
  }

  boost::scoped_ptr<RescoreHgImpl> pimpl_;
};

struct SummaryHgImpl {
  SummaryHgImpl(const VarMap &, Context *, int);
  ~SummaryHgImpl();
  Hypergraph * Apply(const Input &, Context *, Hypergraph *) const;

 private:
  enum SummaryFeature { kNODE_RISK = 1, kEDGE_RISK, kEDGE_PROB };
  struct Summarizer;
  boost::scoped_ptr<Summarizer> s_;
};

template <int PASS>
struct SummaryHg : Pipe<SummaryHg<PASS> > {
  typedef Hypergraph * itype;
  typedef Maybe<Hypergraph *> otype;

  static void Register(OptDesc *opts) {
    static bool added_;
    if (added_) return;
    added_ = true;
    if (PASS == 1) Register1(opts);
    else RegisterN(opts);
  }

  SummaryHg(const VarMap &conf, Context *context) :
      pimpl_(new SummaryHgImpl(conf, context, PASS)) {}

  otype Apply(const Input &input, Context *context, itype arg) const {
    return Just(pimpl_->Apply(input, context, arg));
  }

 private:
  static void Register1(OptDesc *opts) {
    namespace po = boost::program_options;
    opts->add_options()
        ("summary_feature", po::value<std::string>(), "Compute a 'summary feature' at the end of the pass (before any pruning) with name=arg and value=inside-outside/Z")
        ("summary_feature_type", po::value<std::string>()->default_value("node_risk"), "Summary feature types: node_risk, edge_risk, edge_prob")
        ;
  }

  static void RegisterN(OptDesc *opts) {
    std::string p = boost::lexical_cast<std::string>(PASS);
    namespace po = boost::program_options;
    opts->add_options()
        (("summary_feature" + p).c_str(), po::value<std::string>(), ("Optional pass " + p).c_str())
        ;
  }

  boost::scoped_ptr<SummaryHgImpl> pimpl_;
};

struct PruneHgImpl {
  PruneHgImpl(const VarMap &, Context *, int);
  ~PruneHgImpl();
  Hypergraph *Apply(const Input &, Context *, Hypergraph *) const;

 private:
  struct Pruner;
  boost::scoped_ptr<Pruner> p_;
};

template <int PASS>
struct PruneHg : Pipe<PruneHg<PASS> > {
  typedef Hypergraph * itype;
  typedef Maybe<Hypergraph *> otype;

  static void Register(OptDesc *opts) {
    static bool added_;
    if (added_) return;
    added_ = true;
    if (PASS == 1) Register1(opts);
    else RegisterN(opts);
  }

  PruneHg(const VarMap &conf, Context *context) :
      pimpl_(new PruneHgImpl(conf, context, PASS)) {}

  otype Apply(const Input &input, Context *context, itype arg) const {
    return Just(pimpl_->Apply(input, context, arg));
  }

 private:
  static void Register1(OptDesc *opts) {
    namespace po = boost::program_options;
    opts->add_options()
        ("scale_prune_srclen", "scale beams by the input length (in # of tokens; may not be what you want for lattices")
        ("density_prune", po::value<double>(), "Pass 1 pruning: keep no more than this many times the number of edges used in the best derivation tree (>=1.0)")
        ("beam_prune", po::value<double>(), "Pass 1 pruning: Prune paths from scored forest, keep paths within exp(alpha>=0)")
        ;
  }

  static void RegisterN(OptDesc *opts) {
    std::string p = boost::lexical_cast<std::string>(PASS);
    namespace po = boost::program_options;
    opts->add_options()
        (("density_prune" + p).c_str(), po::value<double>(), ("Optional pass " + p).c_str())
        (("beam_prune" + p).c_str(), po::value<double>(), ("Optional pass " + p).c_str())
        ;
  }

  boost::scoped_ptr<PruneHgImpl> pimpl_;
};

} // namespace pipeline


#endif  // _NEW_DECODER_RESCORE_HG_H_
