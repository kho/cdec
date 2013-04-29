#include "ff_hash.h"

#include <string>
#include "hg.h"

using namespace std;

const char kHEXMAP[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

RuleHashFeatures::RuleHashFeatures(const std::string &) {}

void RuleHashFeatures::TraversalFeaturesImpl(const SentenceMetadata& smeta,
                                             const HG::Edge& edge,
                                             const std::vector<const void*>& ant_contexts,
                                             SparseVector<double>* features,
                                             SparseVector<double>* estimated_features,
                                             void* context) const {
  string feat_name=("H:");
  feat_name.reserve(18);
  StringHash<2>(edge.rule_->AsString(false), &feat_name);
  features->add_value(FD::Convert(feat_name), 1);
}
