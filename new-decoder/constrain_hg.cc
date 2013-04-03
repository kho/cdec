#include "constrain_hg.h"

#include "pipeline.h"
#include "input.h"
#include "context.h"
#include "silent.h"

#include "hg.h"
#include "hg_intersect.h"
#include "weights.h"

#include <boost/shared_ptr.hpp>
#include <boost/program_options.hpp>

using namespace std;
using boost::shared_ptr;

namespace pipeline {
bool InputHasRef::Apply(const Input &input, Context *, itype) const {
  return input.ref.size();
}

void ConstrainHgWithRef::Register(OptDesc *opts) {
  static bool added_;
  if (added_) return;
  opts->add_options()
      ("remove_intersected_rule_annotations", "After forced decoding is completed, remove nonterminal annotations (i.e., the source side spans)")
      ;
  added_ = true;
}

struct ConstrainHgWithRefImpl {
  typedef ConstrainHgWithRef::itype itype;
  typedef ConstrainHgWithRef::otype otype;
  ConstrainHgWithRefImpl(const VarMap &conf, Context */*context*/) :
      remove_intersected_rule_annotations(conf.count("remove_intersected_rule_annotations")) {}
  ~ConstrainHgWithRefImpl() {}
  otype Apply(const Input &input, Context *context, itype arg) const {
    const Lattice &ref = input.ref;
    Hypergraph &forest = *arg;
    if (HG::Intersect(ref, &forest)) {
      // if (crf_uniform_empirical) {
      //   if (!SILENT) cerr << "  USING UNIFORM WEIGHTS\n";
      //   for (int i = 0; i < forest.edges_.size(); ++i)
      //     forest.edges_[i].edge_prob_=prob_t::One(); }
      if (remove_intersected_rule_annotations) {
        for (unsigned i = 0; i < forest.edges_.size(); ++i)
          if (forest.edges_[i].rule_ &&
              forest.edges_[i].rule_->parent_rule_)
            forest.edges_[i].rule_ = forest.edges_[i].rule_->parent_rule_;
      }
      forest.Reweight(*(context->last_weights_ptr));
      return Just(arg);
    } else {
      if (!SILENT) cerr << "  REFERENCE UNREACHABLE.\n";
      delete arg;
      return Nothing<Hypergraph *>();
    }
  }
 private:
  bool remove_intersected_rule_annotations;
};

ConstrainHgWithRef::ConstrainHgWithRef(const VarMap &conf, Context *context) : pimpl_(new ConstrainHgWithRefImpl(conf, context)) {}
ConstrainHgWithRef::~ConstrainHgWithRef() {}
ConstrainHgWithRef::otype ConstrainHgWithRef::Apply(const Input &input, Context *context, itype arg) const {
  return pimpl_->Apply(input, context, arg);
}

} // namespace pipeline
