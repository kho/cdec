#ifndef _NEW_DECODER_WRITE_HG_H_
#define _NEW_DECODER_WRITE_HG_H_

#include "pipeline.h"
#include "context.h"
#include "input.h"
#include "silent.h"

#include "forest_writer.h"
#include "hg.h"
#include "hg_io.h"
#include "hg_union.h"
#include "viterbi.h"
#include "tdict.h"

#include <iostream>
#include <string>
#include <vector>

#include <boost/program_options.hpp>


namespace pipeline {

struct ShouldWriteForest : Pipe<ShouldWriteForest> {
  typedef Hypergraph * itype;
  typedef bool otype;
  static void Register(OptDesc *opts) {
    static bool added_;
    if (added_) return;
    added_ = true;
    opts->add_options()
        ("forest_output,O",boost::program_options::value<std::string>(),"Directory to write forests to")
        ;
  }
  ShouldWriteForest(const VarMap &conf, Context */*context*/) :
      result_(conf.count("forest_output")) {}
  otype Apply(const Input &/*input*/, Context */*context*/, itype /*arg*/) const {
    return result_;
  }
 private:
  bool result_;
};


struct WriteForestToFile : Pipe<WriteForestToFile> {
  typedef Hypergraph * itype;
  typedef Hypergraph * otype;
  static void Register(OptDesc */*opts*/) {}
  WriteForestToFile(const VarMap &conf, Context */*context*/) :
      out_path_(conf["forest_output"].as<std::string>()) {}
  otype Apply(const Input &/*input*/, Context *context, itype arg) const {
    using namespace std;
    const Hypergraph &forest = *arg;
    ForestWriter writer(out_path_, context->sent_id);
    if (FileExists(writer.fname_)) {
      if (!SILENT) cerr << "  Unioning...\n";
      Hypergraph new_hg;
      {
        ReadFile rf(writer.fname_);
        bool succeeded = HypergraphIO::ReadFromJSON(rf.stream(), &new_hg);
        if (!succeeded) abort();
      }
      HG::Union(forest, &new_hg);
      bool succeeded = writer.Write(new_hg, false);
      if (!succeeded) abort();
    } else {
      bool succeeded = writer.Write(forest, false);
      if (!succeeded) abort();
    }
    return arg;
  }
 private:
  std::string out_path_;
};


struct OptPrintGraphviz : Pipe<OptPrintGraphviz> {
  typedef Hypergraph * itype;
  typedef Hypergraph * otype;
  static void Register(OptDesc *opts) {
    static bool added_;
    if (added_) return;
    added_ = true;
    opts->add_options()
        ("graphviz","Show (constrained) translation forest in GraphViz format")
        ;
  }
  OptPrintGraphviz(const VarMap &conf, Context */*context*/) : do_(conf.count("graphviz")) {}
  otype Apply(const Input &/*input*/, Context */*context*/, itype arg) const {
    if (do_) arg->PrintGraphviz();
    return arg;
  }
 private:
  bool do_;
};


struct OptJoshuaViz : Pipe<OptJoshuaViz> {
  typedef Hypergraph * itype;
  typedef Hypergraph * otype;
  static void Register(OptDesc *opts) {
    static bool added_;
    if (added_) return;
    added_ = true;
    opts->add_options()
        ("show_joshua_visualization,J", "Produce output compatible with the Joshua visualization tools")
        ;
  }
  OptJoshuaViz(const VarMap &conf, Context */*context*/) : do_(conf.count("show_joshua_visualization")) {}
  otype Apply(const Input &/*input*/, Context *context, itype arg) const {
    if (do_)
      std::cout << context->sent_id << " ||| "
                << JoshuaVisualizationString(*arg) << " ||| 1.0 ||| " << -1.0
                << std::endl;
    return arg;
  }
 private:
  bool do_;
};


struct ShouldWriteKBest : Pipe<ShouldWriteKBest> {
  typedef Maybe<Hypergraph *> itype;
  typedef bool otype;
  static void Register(OptDesc */*opts*/) {}
  ShouldWriteKBest(const VarMap &conf, Context */*context*/) :
      result_(conf.count("k_best")) {}
  otype Apply(const Input &/*input*/, Context */*context*/, itype /*arg*/) const {
    return result_;
  }
 private:
  bool result_;
};


struct ShouldWrite1Best : Pipe<ShouldWrite1Best> {
  typedef Maybe<Hypergraph *> itype;
  typedef bool otype;
  static void Register(OptDesc */*opts*/) {}
  ShouldWrite1Best(const VarMap &conf, Context */*context*/) :
      result_(!conf.count("k_best") && !conf.count("graphviz") && !conf.count("show_joshua_visualization") && !SILENT) {}
  otype Apply(const Input &input, Context */*context*/, itype /*arg*/) const {
    return result_ && !input.ref.size(); // only write 1-best when `ref` is not given in input.
  }
 private:
  bool result_;
};


struct WriteKBest : Pipe<WriteKBest> {
  typedef Maybe<Hypergraph *> itype;
  typedef Maybe<Hypergraph *> otype;
  static void Register(OptDesc *opts) {
    static bool added_;
    if (added_) return;
    added_ = true;
    namespace po = boost::program_options;
    opts->add_options()
        ("show_derivations", po::value<std::string>(), "Directory to print the derivation structures to")
        ("k_best,k",po::value<int>(),"Extract the k best derivations")
        ("unique_k_best,r", "Unique k-best translation list")
        ;
  }
  WriteKBest(const VarMap &conf, Context *context) :
      k_best_(conf["k_best"].as<int>()), unique_(conf.count("unique_k_best")) {
    deriv_ = conf.count("show_derivations") ? conf["show_derivations"].as<std::string>() : "-";
    context->oracle.show_derivation = conf.count("show_derivations");
  }
  otype Apply(const Input &/*input*/, Context *context, itype arg) const {
    if (arg.IsNothing())
      std::cout << std::endl;
    else
      context->oracle.DumpKBest(context->sent_id, *arg.Value(), k_best_, unique_, "-" , deriv_);
    return arg;
  }
 private:
  int k_best_;
  bool unique_;
  std::string deriv_;
};


struct Write1Best : Pipe<Write1Best> {
  typedef Maybe<Hypergraph *> itype;
  typedef Maybe<Hypergraph *> otype;
  static void Register(OptDesc */*opts*/) {}
  Write1Best(const VarMap &/*conf*/, Context */*context*/) {}
  otype Apply(const Input &/*input*/, Context */*context*/, itype arg) const {
    if (arg.IsNothing()) {
      std::cout << std::endl;
    } else {
      std::vector<WordID> trans;
      ViterbiESentence(*arg.Value(), &trans);
      std::cout << TD::GetString(trans) << std::endl;
    }
    return arg;
  }
};




} // namespace pipeline

#endif  // _NEW_DECODER_WRITE_HG_H_
