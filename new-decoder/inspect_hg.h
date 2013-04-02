#ifndef _NEW_DECODER_INSPECT_HG_H_
#define _NEW_DECODER_INSPECT_HG_H_

#include "pipeline.h"
#include "context.h"
#include "silent.h"

#include "inside_outside.h"
#include "viterbi.h"

#include <iostream>
#include <boost/scoped_ptr.hpp>

namespace pipeline {

struct ShowForestStats : Pipe<ShowForestStats> {
  typedef Hypergraph * itype;
  typedef Hypergraph * otype;

  static void Register(OptDesc *opts) {
    namespace po = boost::program_options;
    opts->add_options()
        ("show_tree_structure", "Show the Viterbi derivation structure")
        ("show_derivations", po::value<std::string>(), "Directory to print the derivation structures to")
        ("extract_rules", po::value<std::string>(), "Extract the rules used in translation (not de-duped!) to a file in this directory")
        ;
  }

  ShowForestStats(const VarMap &conf, Context */*context*/)
      : show_tree_(conf.count("show_tree_structure")),
        show_deriv_(conf.count("show_derivations")),
        extract_rules_(conf.count("extract_rules")) {
    if (extract_rules_)
      extract_path_ = conf["extract_rules"].as<std::string>();
  }

  otype Apply(const Input &, Context *context, itype arg) const {
    if (!SILENT) {
      std::stringstream ss;
      ss << context->sent_id;
      boost::shared_ptr<WriteFile> extract_file(new WriteFile(extract_path_ + "/" + ss.str()));
      std::cerr << viterbi_stats(*arg, "  " + context->stage + " forest", true, show_tree_, show_deriv_, extract_rules_, extract_file)
                << std::endl;
    }
    return arg;
  }

 private:
  bool show_tree_, show_deriv_, extract_rules_;
  std::string extract_path_;
};

struct ShouldShowExpectedLength : Pipe<ShouldShowExpectedLength> {
  typedef Hypergraph * itype;
  typedef bool otype;
  static void Register(OptDesc *opts) {
    opts->add_options()
        ("show_expected_length", "Show the expected translation length under the model")
        ;
  }
  ShouldShowExpectedLength(const VarMap &conf, Context */*context*/)
      : result_(conf.count("show_expected_length")) {}
  otype Apply(const Input &/*input*/, Context */*context*/, itype /*arg*/) const {
    return result_;
  }
 private:
  bool result_;
};

struct ShowExpectedLength : Pipe<ShowExpectedLength> {
  typedef Hypergraph * itype;
  typedef Hypergraph * otype;
  static void Register(OptDesc *) {}
  ShowExpectedLength(const VarMap &/*conf*/, Context */*context*/) {}
  otype Apply(const Input &, Context *, itype) const;
};

struct ShouldShowPartition : Pipe<ShouldShowPartition> {
  typedef Hypergraph * itype;
  typedef bool otype;
  static void Register(OptDesc *opts) {
    opts->add_options()
        ("show_partition,z", "Compute and show the partition (inside score)")
        ;
  }
  ShouldShowPartition(const VarMap &conf, Context */*context*/)
      : result_(conf.count("show_partition")) {}
  otype Apply(const Input &/*input*/, Context */*context*/, itype /*arg*/) const {
    return result_;
  }
 private:
  bool result_;
};

struct ShowPartition : Pipe<ShowPartition> {
  typedef Hypergraph * itype;
  typedef Hypergraph * otype;
  static void Register(OptDesc *) {}
  ShowPartition(const VarMap &, Context *) {}
  otype Apply(const Input &, Context *, itype) const;
};

// struct ShowTargetGraphImpl;
// struct ShowTargetGraph : Pipe<ShowTargetGraph> {
//   typedef Hypergraph * itype;
//   typedef Maybe<Hypergraph *> otype;
//   static void Register(OptDesc *);
//   ShowTargetGraph(const VarMap &);
//   otype Apply(const Input &, Context *, itype) const;
//  private:
//   boost::scoped_ptr<ShowTargetGraphImpl> pimpl_;
// };

struct OptShowConditionalProb : Pipe<OptShowConditionalProb> {
  typedef Maybe<Hypergraph *> itype;
  typedef Maybe<Hypergraph *> otype;
  static void Register(OptDesc *opts) {
    opts->add_options()
        ("show_conditional_prob", "Output the conditional log prob to STDOUT instead of a translation")
        ;
  }
  OptShowConditionalProb(const VarMap &conf, Context */*context*/) : do_(conf.count("show_conditional_prob")) {}
  otype Apply(const Input &/*input*/, Context */*context*/, itype arg) const {
    using namespace std;
    if (do_) {
      if (arg.IsNothing()) {
        cout << "-Inf" << endl;
      } else {
        const Hypergraph &forest = *(arg.Value());
        prob_t first_z;
        first_z = Inside<prob_t, EdgeProb>(forest);
        const prob_t ref_z = Inside<prob_t, EdgeProb>(forest);
        cout << (log(ref_z) - log(first_z)) << endl << flush;
      }
    }
    return arg;
  }
 private:
  bool do_;
};

struct ShowViterbiFTree : Pipe<ShowViterbiFTree> {
  typedef Hypergraph * itype;
  typedef Hypergraph * otype;
  static void Register(OptDesc */*opts*/) {}
  ShowViterbiFTree(const VarMap &/*conf*/, Context */*context*/) {}
  otype Apply(const Input &/*input*/, Context */*context*/, itype arg) const {
    if (!SILENT) std::cerr << "  Constr. VitTree: " << ViterbiFTree(*arg) << std::endl;
    return arg;
  }
};
} // namespace pipeline

#endif  // _NEW_DECODER_INSPECT_HG_H_
