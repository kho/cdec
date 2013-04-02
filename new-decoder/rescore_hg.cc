#include "rescore_hg.h"

#include <ostream>
#include <string>
#include <vector>

#include <boost/lexical_cast.hpp>

#include "apply_models.h"
#include "csplit.h"
#include "ff.h"
#include "ff_factory.h"
#include "ffset.h"
#include "weights.h"

#include "stringlib.h"

using namespace std;
using boost::scoped_ptr;
using boost::shared_ptr;



static inline string PerPassOptionName(const string &opt, int pass) {
  if (pass == 1) return opt;
  else return opt + boost::lexical_cast<string>(pass);
}

template <class T>
static inline T FindExactOption(const string &opt, int pass, const pipeline::VarMap &conf) {
  return conf[PerPassOptionName(opt, pass)].as<T>();
}

template <class T>
static inline T FindLastAvailableOption(const string &opt, int pass, const pipeline::VarMap &conf) {
  while (pass > 1 && !conf.count(PerPassOptionName(opt, pass))) --pass;
  return conf[PerPassOptionName(opt, pass)].as<T>();
}

static inline shared_ptr<FeatureFunction> make_ff(string const& ffp, bool verbose_feature_functions, char const* pre = "") {
  std::string ff, param;
  SplitCommandAndParam(ffp, &ff, &param);
  if (verbose_feature_functions)
    std::cerr << pre << "feature: " << ff;

  if (param.size() > 0) std::cerr << " (with config parameters '" << param << "')\n";
  else std::cerr << " (no config parameters)\n";

  boost::shared_ptr<FeatureFunction> pf = ff_registry.Create(ff, param);
  if (!pf) exit(1);
  int nbyte=pf->StateSize();
  if (verbose_feature_functions)
    std::cerr<<"State is "<<nbyte<<" bytes for "<<pre<<"feature "<<ffp<<std::endl;
  return pf;
}


namespace pipeline {

bool ShouldRescorePred(const VarMap &conf, int pass) {
  string ws(PerPassOptionName("weights", pass)), ff(PerPassOptionName("feature_function", pass));
  bool has_ws = conf.count(ws), has_ff = conf.count(ff);
  return has_ff || (pass > 1 && has_ws);
}

bool ShouldSummarizePred(const VarMap &conf, int pass) {
  string sf(PerPassOptionName("summary_feature", pass));
  bool has_sf = conf.count(sf);
  return ShouldRescorePred(conf, pass) && has_sf;
}

bool ShouldPrunePred(const VarMap &conf, int pass) {
  bool has_bp = conf.count(PerPassOptionName("beam_prune", pass)),
      has_dp = conf.count(PerPassOptionName("density_prune", pass));
  return has_bp || has_dp;
}

class RescoreHgImpl::Rescorer {
 public:
  Rescorer(shared_ptr<vector<weight_t> > weights,
           const vector<string> &feature_functions,
           const string &intersection_strategy,
           int pop_limit) : weights_(weights), ffs_(), models_(), inter_conf_() {
    bool has_stateful = false;
    ffs_.reserve(feature_functions.size());
    for (vector<string>::const_iterator ffit = feature_functions.begin();
         ffit != feature_functions.end(); ++ffit) {
      shared_ptr<FeatureFunction> ff_ptr = make_ff(*ffit, true);
      has_stateful = has_stateful || ff_ptr->IsStateful();
      ffs_.push_back(ff_ptr);
    }
    vector<const FeatureFunction *> ff_ptrs;
    ff_ptrs.reserve(feature_functions.size());
    for (vector<shared_ptr<FeatureFunction> >::const_iterator ffsit = ffs_.begin();
         ffsit != ffs_.end(); ++ffsit)
      ff_ptrs.push_back(ffsit->get());
    models_.reset(new ModelSet(*weights_, ff_ptrs));

    int palg = has_stateful ? 1 : 0;
    if (intersection_strategy == "cube_pruning") {
      palg = has_stateful ? 1 : 0;
    } else if (intersection_strategy == "full") {
      palg = 1;
    } else if (intersection_strategy == "fast_cube_pruning") {
      palg = 2;
      cerr << "Using Fast Cube Pruning intersection (see Algorithm 2 described in: Gesmundo A., Henderson J,. Faster Cube Pruning, IWSLT 2010).\n";
    }
    else if (intersection_strategy == "fast_cube_pruning_2") {
      palg = 3;
      cerr << "Using Fast Cube Pruning 2 intersection (see Algorithm 3 described in: Gesmundo A., Henderson J,. Faster Cube Pruning, IWSLT 2010).\n";
    }
    inter_conf_.reset(new IntersectionConfiguration(palg, pop_limit));
  }

  Hypergraph *Apply(const Input &/*input*/, Context *context, Hypergraph *forest) const {
    forest->Reweight(*weights_);
    if (!models_->empty()) {
      Timer t("Forest rescoring:");
      models_->PrepareForInput(*context->smeta_ptr);
      Hypergraph rescored_forest;
      ApplyModelSet(*forest, *context->smeta_ptr, *models_, *inter_conf_, &rescored_forest);
      forest->swap(rescored_forest);
      forest->Reweight(*weights_);
    }
    context->last_weights_ptr = weights_;
    return forest;
  }

 private:
  shared_ptr<vector<weight_t> > weights_;
  vector<shared_ptr<FeatureFunction> > ffs_;
  scoped_ptr<ModelSet> models_;
  scoped_ptr<IntersectionConfiguration> inter_conf_;
};

RescoreHgImpl::RescoreHgImpl(const VarMap &conf, Context *context, int pass) : r_() {
  string ff(PerPassOptionName("feature_function", pass));
  bool has_ff = conf.count(ff);

  // For weights, intersection strategy, pop limit, reuse values
  // from previous pass if not given
  shared_ptr<vector<weight_t> > weights =
      context->weights.Get(FindLastAvailableOption<string>("weights", pass, conf));
  string intersection_strategy =
      LowercaseString(FindLastAvailableOption<string>("intersection_strategy", pass, conf));
  int pop_limit =
      FindLastAvailableOption<int>("cubepruning_pop_limit", pass, conf);
  // do not "reuse" previous feature functions...
  vector<string> feature_functions;
  if (has_ff)
    feature_functions = FindExactOption<vector<string> >("feature_functions", pass, conf);
  Rescorer *p = new Rescorer(weights, feature_functions, intersection_strategy, pop_limit);
  r_.reset(p);
}

RescoreHgImpl::~RescoreHgImpl() {}

Hypergraph *RescoreHgImpl::Apply(const Input &input, Context *context, Hypergraph *arg) const {
  return r_->Apply(input, context, arg);
}

class SummaryHgImpl::Summarizer {
 public:
  Summarizer(SummaryFeature type, int fid, shared_ptr<vector<weight_t> > weights) : type_(type), fid_(fid), weights_(weights) {}
  Hypergraph *Apply(const Input &/*input*/, Context */*context*/, Hypergraph *arg) const {
    Hypergraph &forest = *arg;
    if (type_ == kEDGE_PROB) {
      const prob_t z = forest.PushWeightsToGoal(1.0);
      if (!std::isfinite(log(z)) || std::isnan(log(z))) {
        cerr << " !!! Invalid partition detected, abandoning.\n";
      } else {
        for (int i = 0; i < forest.edges_.size(); ++i) {
          const double log_prob_transition = log(forest.edges_[i].edge_prob_); // locally normalized by the edge
          // head node by forest.PushWeightsToGoal
          if (!std::isfinite(log_prob_transition) || std::isnan(log_prob_transition)) {
            cerr << "Edge: i=" << i << " got bad inside prob: " << *forest.edges_[i].rule_ << endl;
            abort();
          }

          forest.edges_[i].feature_values_.set_value(fid_, log_prob_transition);
        }
        forest.Reweight(*weights_);  // reset weights
      }
    } else if (type_ == kNODE_RISK) {
      Hypergraph::EdgeProbs posts;
      const prob_t z = forest.ComputeEdgePosteriors(1.0, &posts);
      if (!std::isfinite(log(z)) || std::isnan(log(z))) {
        cerr << " !!! Invalid partition detected, abandoning.\n";
      } else {
        for (int i = 0; i < forest.nodes_.size(); ++i) {
          const Hypergraph::EdgesVector& in_edges = forest.nodes_[i].in_edges_;
          prob_t node_post = prob_t(0);
          for (int j = 0; j < in_edges.size(); ++j)
            node_post += (posts[in_edges[j]] / z);
          const double log_np = log(node_post);
          if (!std::isfinite(log_np) || std::isnan(log_np)) {
            cerr << "got bad posterior prob for node " << i << endl;
            abort();
          }
          for (int j = 0; j < in_edges.size(); ++j)
            forest.edges_[in_edges[j]].feature_values_.set_value(fid_, exp(log_np));
          //            Hypergraph::Edge& example_edge = forest.edges_[in_edges[0]];
          //            string n = "NONE";
          //            if (forest.nodes_[i].cat_) n = TD::Convert(-forest.nodes_[i].cat_);
          //            cerr << "[" << n << "," << example_edge.i_ << "," << example_edge.j_ << "] = " << exp(log_np) << endl;
        }
      }
    } else if (type_ == kEDGE_RISK) {
      Hypergraph::EdgeProbs posts;
      const prob_t z = forest.ComputeEdgePosteriors(1.0, &posts);
      if (!std::isfinite(log(z)) || std::isnan(log(z))) {
        cerr << " !!! Invalid partition detected, abandoning.\n";
      } else {
        assert(posts.size() == forest.edges_.size());
        for (int i = 0; i < posts.size(); ++i) {
          const double log_np = log(posts[i] / z);
          if (!std::isfinite(log_np) || std::isnan(log_np)) {
            cerr << "got bad posterior prob for node " << i << endl;
            abort();
          }
          forest.edges_[i].feature_values_.set_value(fid_, exp(log_np));
        }
      }
    } else {
      assert(!"shouldn't happen");
    }
    return arg;
  }
 private:
  SummaryFeature type_;
  int fid_;
  shared_ptr<vector<weight_t> > weights_;
};

SummaryHgImpl::SummaryHgImpl(const VarMap &conf, Context *context, int pass) : s_() {
  SummaryFeature summary_feature_type;
  if (conf["summary_feature_type"].as<string>() == "edge_risk")
    summary_feature_type = kEDGE_RISK;
  else if (conf["summary_feature_type"].as<string>() == "node_risk")
    summary_feature_type = kNODE_RISK;
  else if (conf["summary_feature_type"].as<string>() == "edge_prob")
    summary_feature_type = kEDGE_PROB;
  else {
    cerr << "Bad summary_feature_type: " << conf["summary_feature_type"].as<string>() << endl;
    abort();                            // TODO: move over to `Apply`, do not crash inside a constructor
  }

  int fid_summary = FD::Convert(FindExactOption<string>("summary_feature", pass, conf));

  shared_ptr<vector<weight_t> > weights =
      context->weights.Get(FindLastAvailableOption<string>("weights", pass, conf));
  s_.reset(new Summarizer(summary_feature_type, fid_summary, weights));
}

SummaryHgImpl::~SummaryHgImpl() {}

Hypergraph * SummaryHgImpl::Apply(const Input &input, Context *context, Hypergraph *arg) const {
  return s_->Apply(input, context, arg);
}

class PruneHgImpl::Pruner {
 public:
  Pruner(double beam_prune, double density_prune, bool scale) : scale_(scale),
      bp_(beam_prune), dp_(density_prune) {}

  Hypergraph *Apply(const Input &input, Context */*context*/, Hypergraph *arg) const {
    Hypergraph &forest = *arg;

    double bp = bp_, dp = dp_;
    // TODO: use smeta or something
    // int srclen = context->smeta_ptr->GetSourceLength();
    int srclen = NTokens(input.to_translate,' ');
    if (scale_) {
      bp *= srclen;
      dp *= srclen;
    }

    vector<bool> preserve_mask, *pm = NULL;

    // // FIXME: move out
    // if (context->conf.count("csplit_preserve_full_word")) {
    //   preserve_mask.resize(forest.edges_.size());
    //   preserve_mask[CompoundSplit::GetFullWordEdgeIndex(forest)] = true;
    //   pm=&preserve_mask;
    // }

    forest.PruneInsideOutside(bp, dp, pm, false, 1);

    return arg;
  }

 private:
  bool scale_;
  double bp_, dp_;
};

PruneHgImpl::PruneHgImpl(const VarMap &conf, Context */*context*/, int pass) : p_() {
  bool has_bp = conf.count(PerPassOptionName("beam_prune", pass)),
      has_dp = conf.count(PerPassOptionName("density_prune", pass));
  double beam_prune = 0, density_prune = 0;
  if (has_bp)
    beam_prune = FindExactOption<double>("beam_prune", pass, conf);
  if (has_dp)
    density_prune = FindExactOption<double>("density_prune", pass, conf);

  p_.reset(new Pruner(beam_prune, density_prune, conf.count("scale_prune_srclen")));
}

PruneHgImpl::~PruneHgImpl() {}

Hypergraph * PruneHgImpl::Apply(const Input &input, Context *context, Hypergraph *arg) const {
  return p_->Apply(input, context, arg);
}

} // namespace pipeline
