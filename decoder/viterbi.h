#ifndef _VITERBI_H_
#define _VITERBI_H_

#include <vector>
#include "prob.h"
#include "hg.h"
#include "tdict.h"
#include "filelib.h"
#include <boost/make_shared.hpp>

std::string viterbi_stats(Hypergraph const& hg, std::string const& name="forest", bool estring=true, bool etree=false, bool derivation_tree=false, bool extract_rules=false, boost::shared_ptr<WriteFile> extract_file = boost::make_shared<WriteFile>());

/// computes for each hg node the best (according to WeightType/WeightFunction) derivation, and some homomorphism (bottom up expression tree applied through Traversal) of it. T is the "return type" of Traversal, which is called only once for the best edge for a node's result (i.e. result will start default constructed)
//TODO: make T a typename inside Traversal and WeightType a typename inside WeightFunction?
// Traversal must implement:
//  typedef T Result;
//  void operator()(HG::Edge const& e,const vector<const Result*>& ants, Result* result) const;
// WeightFunction must implement:
//  typedef prob_t Weight;
//  Weight operator()(HG::Edge const& e) const;
template<class Traversal,class WeightFunction>
typename WeightFunction::Weight Viterbi(const Hypergraph& hg,
                   typename Traversal::Result* result,
                   const Traversal& traverse,
                   const WeightFunction& weight) {
  typedef typename Traversal::Result T;
  typedef typename WeightFunction::Weight WeightType;
  const int num_nodes = hg.nodes_.size();
  std::vector<T> vit_result(num_nodes);
  std::vector<WeightType> vit_weight(num_nodes, WeightType());

  for (int i = 0; i < num_nodes; ++i) {
    const Hypergraph::Node& cur_node = hg.nodes_[i];
    WeightType* const cur_node_best_weight = &vit_weight[i];
    T*          const cur_node_best_result = &vit_result[i];

    const unsigned num_in_edges = cur_node.in_edges_.size();
    if (num_in_edges == 0) {
      *cur_node_best_weight = WeightType(1);
      continue;
    }
    HG::Edge const* edge_best=0;
    for (unsigned j = 0; j < num_in_edges; ++j) {
      const HG::Edge& edge = hg.edges_[cur_node.in_edges_[j]];
      WeightType score = weight(edge);
      for (unsigned k = 0; k < edge.tail_nodes_.size(); ++k)
        score *= vit_weight[edge.tail_nodes_[k]];
      if (!edge_best || *cur_node_best_weight < score) {
        *cur_node_best_weight = score;
        edge_best=&edge;
      }
    }
    assert(edge_best);
    HG::Edge const& edgeb=*edge_best;
    std::vector<const T*> antsb(edgeb.tail_nodes_.size());
    for (unsigned k = 0; k < edgeb.tail_nodes_.size(); ++k)
      antsb[k] = &vit_result[edgeb.tail_nodes_[k]];
    traverse(edgeb, antsb, cur_node_best_result);
  }
  if (vit_result.empty())
    return WeightType(0);
  std::swap(*result, vit_result.back());
  return vit_weight.back();
}


/*
template<typename Traversal,typename WeightFunction>
typename WeightFunction::Weight Viterbi(const Hypergraph& hg,
                   typename Traversal::Result* result)
{
  Traversal traverse;
  WeightFunction weight;
  return Viterbi(hg,result,traverse,weight);
}

template<class Traversal,class WeightFunction=EdgeProb>
typename WeightFunction::Weight Viterbi(const Hypergraph& hg,
                   typename Traversal::Result* result,
                   Traversal const& traverse=Traversal()
  )
{
  WeightFunction weight;
  return Viterbi(hg,result,traverse,weight);
}
*/

//spec for EdgeProb
template<class Traversal>
prob_t Viterbi(const Hypergraph& hg,
                   typename Traversal::Result* result,
                   Traversal const& traverse=Traversal()
  )
{
  EdgeProb weight;
  return Viterbi(hg,result,traverse,weight);
}


template<class Traversal>
void Viterbi(const Hypergraph& hg,
                   typename Traversal::Align* align,
                   Traversal const& traverse=Traversal()
  )
{
  EdgeProb weight;
  Viterbi(hg,align,traverse,weight);
}

inline bool AlignSortFunction(AlignmentPoint* i, AlignmentPoint* j) {
	if (i->t_ < j->t_)
		return true;
	else if (i->t_ > j->t_)
		return false;
	else {
		if (i->s_ > j->s_)
			return false;
		else
			return true;
	}
}

#include "trule.h"
typedef std::pair<short, short> AlignPair;
template<class Traversal,class WeightFunction>
void Viterbi(const Hypergraph& hg,
		     typename Traversal::Align* align,
             const Traversal& traverse,
             const WeightFunction& weight) {
	  typedef typename std::vector<AlignPair> A;
	  typedef typename WeightFunction::Weight WeightType;
	  const int num_nodes = hg.nodes_.size();
	  std::vector<A> vit_align(num_nodes);
	  std::vector<int> vit_f_word(num_nodes);
	  std::vector<int> vit_e_word(num_nodes);
	  std::vector<WeightType> vit_weight(num_nodes, WeightType());

	  for (int i = 0; i < num_nodes; ++i) {
	    const Hypergraph::Node& cur_node = hg.nodes_[i];
	    WeightType* const cur_node_best_weight = &vit_weight[i];
	    A*          const cur_node_best_align = &vit_align[i];

	    const unsigned num_in_edges = cur_node.in_edges_.size();
	    if (num_in_edges == 0) {
	      *cur_node_best_weight = WeightType(1);
	      continue;
	    }
	    HG::Edge const* edge_best=0;
	    for (unsigned j = 0; j < num_in_edges; ++j) {
	      const HG::Edge& edge = hg.edges_[cur_node.in_edges_[j]];
	      WeightType score = weight(edge);
	      for (unsigned k = 0; k < edge.tail_nodes_.size(); ++k)
	        score *= vit_weight[edge.tail_nodes_[k]];
	      if (!edge_best || *cur_node_best_weight < score) {
	        *cur_node_best_weight = score;
	        edge_best=&edge;
	      }
	    }
	    assert(edge_best);
	    HG::Edge const& edgeb=*edge_best;
	    int f_word = 0, e_word = 0;
	    for (unsigned k = 0; k < edgeb.tail_nodes_.size(); ++k) {
	    	f_word += vit_f_word[edgeb.tail_nodes_[k]];
	    	e_word += vit_e_word[edgeb.tail_nodes_[k]];
	    }
	    f_word += edgeb.rule_->f_.size() - edgeb.tail_nodes_.size();
	    e_word += edgeb.rule_->e_.size() - edgeb.tail_nodes_.size();
	    assert(f_word == edgeb.j_ - edgeb.i_);
	    vit_f_word[i] = f_word;
	    vit_e_word[i] = e_word;

	    std::vector<const A*> antsb(edgeb.tail_nodes_.size());
	    std::vector<int> antsb_f_word(edgeb.tail_nodes_.size());
	    std::vector<int> antsb_e_word(edgeb.tail_nodes_.size());
	    for (unsigned k = 0; k < edgeb.tail_nodes_.size(); ++k) {
	      antsb[k] = &vit_align[edgeb.tail_nodes_[k]];
	      antsb_f_word[k] = vit_f_word[edgeb.tail_nodes_[k]];
	      antsb_e_word[k] = vit_e_word[edgeb.tail_nodes_[k]];
	    }

	    traverse(edgeb, antsb, antsb_f_word, antsb_e_word, cur_node_best_align);
	  }
	  if (vit_align.empty())
		  return;
	  const A& alignb = vit_align.back();
	  for (unsigned i = 0; i < alignb.size(); i++)
		  align->push_back(std::make_pair(alignb[i].first, alignb[i].second));
}

struct AlignmentTraversal {
	typedef std::vector<AlignPair> Align;
	void operator() (const HG::Edge& edge,
			         const std::vector<const Align*>& ants,
			         const std::vector<int>& ants_f_words,
			         const std::vector<int>& ants_e_words,
			         Align* align) const {
		unsigned vc = 0;
		align->clear();
		const TRulePtr rule = edge.rule_;
		std::vector<int> f_index(rule->f_.size());
		int index = 0;
		for (unsigned i = 0; i < rule->f_.size(); i++) {
			f_index[i] = index;
			const WordID& c = rule->f_[i];
			if (c < 1)
				index += ants_f_words[vc++];
			else
				index++;
		}
		assert(vc == ants.size());

		std::vector<int> e_index(rule->e_.size());
		index = 0;
		vc = 0;
		for (unsigned i = 0; i < rule->e_.size(); i++) {
			e_index[i] = index;
			const WordID& c = rule->e_[i];
			if (c < 1) {
				index += ants_e_words[-c];
				vc++;
			}
			else
				index++;
		}
		assert(vc == ants.size());

		//then we need the alignment with regarding to target side item
		std::vector<AlignmentPoint*> cur_align; //besides the alignment of terminals, include the alignment of NTs
		cur_align.reserve(edge.rule_->a_.size() + ants.size());
		unsigned nt_pos = 0;
		for (unsigned i = 0; i < edge.rule_->f_.size(); i++) {
			if (edge.rule_->f_[i] < 0) {
				unsigned j;
				for (j = 0; j < edge.rule_->e_.size(); j++)
					if (edge.rule_->e_[j] * -1 == nt_pos) {
						cur_align.push_back(new AlignmentPoint(i, j));
						break;
					}
				assert(j != edge.rule_->e_.size());
				nt_pos++;
			}
		}
		for (unsigned i = 0; i < edge.rule_->a_.size(); i++) {
			cur_align.push_back(new AlignmentPoint(edge.rule_->a_[i].s_, edge.rule_->a_[i].t_));
		}
		std::sort(cur_align.begin(), cur_align.end(), AlignSortFunction);

		vc = 0;
		for (std::vector<AlignmentPoint*>::const_iterator i = cur_align.begin(); i != cur_align.end(); ++i) {
			const WordID& c = rule->e_[(*i)->t_];
			if (c < 1) {
				++vc;
		        const Align* var_value = ants[-c];
		        int findex = f_index[(*i)->s_];
		        int eindex = e_index[(*i)->t_];
		        for (unsigned j = 0; j < var_value->size(); j++) {
		        	align->push_back(std::make_pair(findex + (*var_value)[j].first, eindex + (*var_value)[j].second));
		        }
			} else {
		        int findex = f_index[(*i)->s_];
		        int eindex = e_index[(*i)->t_];
	        	align->push_back(std::make_pair(findex, eindex));
			}
		}
	    assert(vc == ants.size());

	    for (size_t i = 0; i < cur_align.size(); i++)
	    	delete cur_align[i];
	}
};

struct PathLengthTraversal {
  typedef int Result;
  void operator()(const HG::Edge& edge,
                  const std::vector<const int*>& ants,
                  int* result) const {
    (void) edge;
    *result = 1;
    for (unsigned i = 0; i < ants.size(); ++i) *result += *ants[i];
  }
};

struct ESentenceTraversal {
  typedef std::vector<WordID> Result;
  void operator()(const HG::Edge& edge,
                  const std::vector<const Result*>& ants,
                  Result* result) const {
    edge.rule_->ESubstitute(ants, result);
  }
};

struct ELengthTraversal {
  typedef int Result;
  void operator()(const HG::Edge& edge,
                  const std::vector<const int*>& ants,
                  int* result) const {
    *result = edge.rule_->ELength() - edge.rule_->Arity();
    for (unsigned i = 0; i < ants.size(); ++i) *result += *ants[i];
  }
};

struct FSentenceTraversal {
  typedef std::vector<WordID> Result;
  void operator()(const HG::Edge& edge,
                  const std::vector<const Result*>& ants,
                  Result* result) const {
    edge.rule_->FSubstitute(ants, result);
  }
};

// create a strings of the form (S (X the man) (X said (X he (X would (X go)))))
struct ETreeTraversal {
  ETreeTraversal() : left("("), space(" "), right(")") {}
  const std::string left;
  const std::string space;
  const std::string right;
  typedef std::vector<WordID> Result;
  void operator()(const HG::Edge& edge,
                  const std::vector<const Result*>& ants,
                  Result* result) const {
    Result tmp;
    edge.rule_->ESubstitute(ants, &tmp);
    const std::string cat = TD::Convert(edge.rule_->GetLHS() * -1);
    if (cat == "Goal")
      result->swap(tmp);
    else
      TD::ConvertSentence(left + cat + space + TD::GetString(tmp) + right,
                          result);
  }
};

struct FTreeTraversal {
  FTreeTraversal() : left("("), space(" "), right(")") {}
  const std::string left;
  const std::string space;
  const std::string right;
  typedef std::vector<WordID> Result;
  void operator()(const HG::Edge& edge,
                  const std::vector<const Result*>& ants,
                  Result* result) const {
    Result tmp;
    edge.rule_->FSubstitute(ants, &tmp);
    const std::string cat = TD::Convert(edge.rule_->GetLHS() * -1);
    if (cat == "Goal")
      result->swap(tmp);
    else
      TD::ConvertSentence(left + cat + space + TD::GetString(tmp) + right,
                          result);
  }
};

struct ViterbiPathTraversal {
  typedef std::vector<HG::Edge const*> Result;
  void operator()(const HG::Edge& edge,
                  std::vector<Result const*> const& ants,
                  Result* result) const {
    for (unsigned i = 0; i < ants.size(); ++i)
      for (unsigned j = 0; j < ants[i]->size(); ++j)
        result->push_back((*ants[i])[j]);
    result->push_back(&edge);
  }
};

struct FeatureVectorTraversal {
  typedef SparseVector<double> Result;
  void operator()(HG::Edge const& edge,
                  std::vector<Result const*> const& ants,
                  Result* result) const {
    for (unsigned i = 0; i < ants.size(); ++i)
      *result+=*ants[i];
    *result+=edge.feature_values_;
  }
};


std::string JoshuaVisualizationString(const Hypergraph& hg);
prob_t ViterbiESentence(const Hypergraph& hg, std::vector<WordID>* result);
std::string ViterbiETree(const Hypergraph& hg);
void ViterbiRules(const Hypergraph& hg, std::ostream* s);
prob_t ViterbiFSentence(const Hypergraph& hg, std::vector<WordID>* result);
std::string ViterbiFTree(const Hypergraph& hg);
int ViterbiELength(const Hypergraph& hg);
int ViterbiPathLength(const Hypergraph& hg);

std::string ViterbiAlignment(const Hypergraph& hg);

/// if weights supplied, assert viterbi prob = features.dot(*weights) (exception if fatal, cerr warn if not).  return features (sum over all edges in viterbi derivation)
SparseVector<double> ViterbiFeatures(Hypergraph const& hg,WeightVector const* weights=0,bool fatal_dotprod_disagreement=false);

#endif
