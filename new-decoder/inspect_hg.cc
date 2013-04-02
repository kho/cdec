#include "inspect_hg.h"

#include <boost/program_options.hpp>

#include "exp_semiring.h"
#include "hg_io.h"
#include "inside_outside.h"

using namespace std;
using boost::scoped_ptr;
namespace po = boost::program_options;

struct ELengthWeightFunction {
  double operator()(const Hypergraph::Edge& e) const {
    return e.rule_->ELength() - e.rule_->Arity();
  }
};

namespace pipeline {
Hypergraph *ShowExpectedLength::Apply(const Input &/*input*/, Context */*context*/, itype arg) const {
  const PRPair<prob_t, prob_t> res =
      Inside<PRPair<prob_t, prob_t>,
             PRWeightFunction<prob_t, EdgeProb, prob_t, ELengthWeightFunction> >(*arg);
  cerr << "  Expected length  (words): " << (res.r / res.p).as_float() << "\t" << res << endl;
  return arg;
}


Hypergraph *ShowPartition::Apply(const Input &/*input*/, Context *context, itype arg) const {
  const prob_t z = Inside<prob_t, EdgeProb>(*arg);
  cerr << "  " << context->stage << " partition log(Z): " << log(z) << endl;
  return arg;
}

// void ShowTargetGraph::Register(OptDesc *opts) {
//   opts->add_options()
//       ("show_target_graph", po::value<string>(), "Directory to write the target hypergraphs to")
//       ;
// }

// struct ShowTargetGraphImpl {
//   typedef ShowTargetGraph::itype itype;
//   typedef ShowTargetGraph::otype otype;
//   ShowTargetGraphImpl(const VarMap &conf) {}
//   ~ShowTargetGraphImpl() {}
//   otype Apply(const Input &input, Context *context, itype arg) const {
//     if (context->conf.count("show_target_graph")) {
//       HypergraphIO::WriteTarget(context->conf["show_target_graph"].as<string>(), context->sent_id, *arg);
//     }
//     return Just(arg);
//   }
// };

// ShowTargetGraph::ShowTargetGraph(const VarMap &conf) : pimpl_(new ShowTargetGraphImpl(conf)) {}
// ShowTargetGraph::otype ShowTargetGraph::Apply(const Input &input, Context *context, itype arg) const {
//   return pimpl_->Apply(input, context, arg);
// }
} // namespace pipeline
