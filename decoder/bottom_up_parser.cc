//TODO: when using many nonterminals, group passive edges for a span (treat all as a single X for the active items).

//TODO: figure out what cdyer was talking about when he said that having unary rules A->B and B->A, doesn't make cycles appear in result provided rules are sorted in some way (that they typically are)

#include "bottom_up_parser.h"

#include <iostream>
#include <map>

#include "hg.h"
#include "array2d.h"
#include "tdict.h"
#include "verbose.h"

using namespace std;

// split: receives a char delimiter; returns a vector of strings
// By default ignores repeated delimiters, unless argument rep == 1.
int split_string(const string& str, char delim, int rep, vector<string>* flds) {
  if (flds != NULL) flds->clear();
  string work = str;
  string buf = "";
  int i = 0;
  int count = 0;
  while (i < work.length()) {
    if (work[i] != delim)
      buf += work[i];
    else if (rep == 1) {
      if (flds != NULL) flds->push_back(buf);
      buf = "";
      count++;
    } else if (buf.length() > 0) {
      if (flds != NULL) flds->push_back(buf);
      buf = "";
      count++;
    }
    i++;
  }
  if (!buf.empty()) {
    if (flds != NULL) flds->push_back(buf);
    count++;
  }
  return count;
}

struct SyntacticConstraint {
  SyntacticConstraint(const string& constraints, int iLen):
      len_(iLen) {
    constraint_array_ = new char[ iLen * iLen ];
    memset(constraint_array_, 0, iLen * iLen);
    SetConstraints(constraints);
    //PrintConstraints( );
  }
  ~SyntacticConstraint() {
    delete [] constraint_array_;
  }

  char GetConstraint(int x, int y) {
    assert(x < len_);
    assert(y < len_);
    return constraint_array_[GetArrayIndex(x, y)];
  }

  int inline GetArrayIndex(int x, int y) {
    return x * len_ + y;
  }
  void SetConstraints(const string& constraints) {
    vector<string> vec;
    int count = split_string( constraints, ';', 1, &vec );
    assert( count == len_ );
    for (size_t i = 0; i < vec.size(); i++) {
      constraint_array_[GetArrayIndex(i, i)] = 1; //[i,i] is always okay
      vector<string> item;
      split_string( vec[i], ',', 0, &item );
      for (size_t j = 0; j < item.size(); j++) {
        size_t t = item[j].find('-', 0);
        if(t == string::npos) {
          constraint_array_[GetArrayIndex(i, atoi(item[j].c_str()))] = 1;
        }
        else {
          int start = atoi(item[j].substr(0, t).c_str());
          int end = atoi(item[j].substr(t+1).c_str()) + 1;
          for (int k = start; k < end; k++)
            constraint_array_[GetArrayIndex(i, k)] = 1;
        }
      }
    }
  }
 private:
  void PrintConstraints( ) {
    cerr << "\\\t";
    for (size_t i = 0; i < len_; i++) {
      cerr << i << "\t";
    }
    cerr << "\n";
    for (size_t i = 0; i < len_; i++) {
      cerr << i << "\t";
      for (size_t j = 0; j < len_; j++) {
        cerr << constraint_array_[GetArrayIndex(i, j)] << "\t";
      }
      cerr << "\n";
    }
  }
 private:
  char* constraint_array_;
  const int len_;
};

class ActiveChart;
class PassiveChart {
 public:
  PassiveChart(const string& goal,
               const vector<GrammarPtr>& grammars,
               const Lattice& input,
               SentenceMetadata* smeta,
               Hypergraph* forest);
  ~PassiveChart();

  inline const vector<int>& operator()(int i, int j) const { return chart_(i,j); }
  bool Parse();
  inline int size() const { return chart_.width(); }
  inline bool GoalFound() const { return goal_idx_ >= 0; }
  inline int GetGoalIndex() const { return goal_idx_; }

 private:
  void ApplyRules(const int i,
                  const int j,
                  const RuleBin* rules,
                  const Hypergraph::TailNodeVector& tail,
                  const float lattice_cost);

  void ApplyRule(const int i,
                 const int j,
                 const TRulePtr& r,
                 const Hypergraph::TailNodeVector& ant_nodes,
                 const float lattice_cost);

  void ApplyUnaryRules(const int i, const int j);

  const vector<GrammarPtr>& grammars_;
  const Lattice& input_;
  Hypergraph* forest_;
  const SentenceMetadata* smeta_;
  Array2D<vector<int> > chart_;   // chart_(i,j) is the list of nodes derived spanning i,j
  typedef map<int, int> Cat2NodeMap;
  Array2D<Cat2NodeMap> nodemap_;
  vector<ActiveChart*> act_chart_;
  const WordID goal_cat_;    // category that is being searched for at [0,n]
  TRulePtr goal_rule_;
  int goal_idx_;             // index of goal node, if found
  const int lc_fid_;

  static WordID kGOAL;       // [Goal]
};

WordID PassiveChart::kGOAL = 0;

class ActiveChart {
 public:
  ActiveChart(const Hypergraph* hg, const PassiveChart& psv_chart) :
      hg_(hg),
      act_chart_(psv_chart.size(), psv_chart.size()), psv_chart_(psv_chart) {}

  struct ActiveItem {
    ActiveItem(const GrammarIter* g, const Hypergraph::TailNodeVector& a, float lcost) :
        gptr_(g), ant_nodes_(a), lattice_cost(lcost) {}
    explicit ActiveItem(const GrammarIter* g) :
        gptr_(g), ant_nodes_(), lattice_cost(0.0) {}

    void ExtendTerminal(int symbol, float src_cost, vector<ActiveItem>* out_cell) const {
      const GrammarIter* ni = gptr_->Extend(symbol);
      if (ni) {
        out_cell->push_back(ActiveItem(ni, ant_nodes_, lattice_cost + src_cost));
      }
    }
    void ExtendNonTerminal(const Hypergraph* hg, int node_index, vector<ActiveItem>* out_cell) const {
      int symbol = hg->nodes_[node_index].cat_;
      const GrammarIter* ni = gptr_->Extend(symbol);
      if (!ni) return;
      Hypergraph::TailNodeVector na(ant_nodes_.size() + 1);
      for (unsigned i = 0; i < ant_nodes_.size(); ++i)
        na[i] = ant_nodes_[i];
      na[ant_nodes_.size()] = node_index;
      out_cell->push_back(ActiveItem(ni, na, lattice_cost));
    }

    const GrammarIter* gptr_;
    Hypergraph::TailNodeVector ant_nodes_;
    float lattice_cost;  // TODO? use SparseVector<double>
  };

  inline const vector<ActiveItem>& operator()(int i, int j) const { return act_chart_(i,j); }
  void SeedActiveChart(const Grammar& g) {
    int size = act_chart_.width();
    for (int i = 0; i < size; ++i)
      if (g.HasRuleForSpan(i,i,0))
        act_chart_(i,i).push_back(ActiveItem(g.GetRoot()));
  }

  void ExtendActiveItems(int i, int k, int j) {
    //cerr << "  LOOK(" << i << "," << k << ") for completed items in (" << k << "," << j << ")\n";
    vector<ActiveItem>& cell = act_chart_(i,j);
    const vector<ActiveItem>& icell = act_chart_(i,k);
    const vector<int>& idxs = psv_chart_(k, j);
    //if (!idxs.empty()) { cerr << "FOUND IN (" << k << "," << j << ")\n"; }
    for (vector<ActiveItem>::const_iterator di = icell.begin(); di != icell.end(); ++di) {
      for (vector<int>::const_iterator ni = idxs.begin(); ni != idxs.end(); ++ni) {
        di->ExtendNonTerminal(hg_, *ni, &cell);
      }
    }
  }

  void AdvanceDotsForAllItemsInCell(int i, int j, const vector<vector<LatticeArc> >& input) {
    //cerr << "ADVANCE(" << i << "," << j << ")\n";
    for (int k=i+1; k < j; ++k)
      ExtendActiveItems(i, k, j);

    const vector<LatticeArc>& out_arcs = input[j-1];
    for (vector<LatticeArc>::const_iterator ai = out_arcs.begin();
         ai != out_arcs.end(); ++ai) {
      const WordID& f = ai->label;
      const double& c = ai->cost;
      const int& len = ai->dist2next;
      //VLOG(1) << "F: " << TD::Convert(f) << endl;
      const vector<ActiveItem>& ec = act_chart_(i, j-1);
      for (vector<ActiveItem>::const_iterator di = ec.begin(); di != ec.end(); ++di)
        di->ExtendTerminal(f, c, &act_chart_(i, j + len - 1));
    }
  }

 private:
  const Hypergraph* hg_;
  Array2D<vector<ActiveItem> > act_chart_;
  const PassiveChart& psv_chart_;
};

PassiveChart::PassiveChart(const string& goal,
                           const vector<GrammarPtr>& grammars,
                           const Lattice& input,
                           SentenceMetadata* smeta,
                           Hypergraph* forest) :
    grammars_(grammars),
    input_(input),
    smeta_(smeta),
    forest_(forest),
    chart_(input.size()+1, input.size()+1),
    nodemap_(input.size()+1, input.size()+1),
    goal_cat_(TD::Convert(goal) * -1),
    goal_rule_(new TRule("[Goal] ||| [" + goal + ",1] ||| [" + goal + ",1]")),
    goal_idx_(-1),
  lc_fid_(FD::Convert("LatticeCost")) {
  act_chart_.resize(grammars_.size());
  for (unsigned i = 0; i < grammars_.size(); ++i)
    act_chart_[i] = new ActiveChart(forest, *this);
  if (!kGOAL) kGOAL = TD::Convert("Goal") * -1;
  if (!SILENT) cerr << "  Goal category: [" << goal << ']' << endl;
}

void PassiveChart::ApplyRule(const int i,
                             const int j,
                             const TRulePtr& r,
                             const Hypergraph::TailNodeVector& ant_nodes,
                             const float lattice_cost) {
  Hypergraph::Edge* new_edge = forest_->AddEdge(r, ant_nodes);
  new_edge->prev_i_ = r->prev_i;
  new_edge->prev_j_ = r->prev_j;
  new_edge->i_ = i;
  new_edge->j_ = j;
  new_edge->feature_values_ = r->GetFeatureValues();
  if (lattice_cost && lc_fid_)
    new_edge->feature_values_.set_value(lc_fid_, lattice_cost);
  Cat2NodeMap& c2n = nodemap_(i,j);
  const bool is_goal = (r->GetLHS() == kGOAL);
  const Cat2NodeMap::iterator ni = c2n.find(r->GetLHS());
  Hypergraph::Node* node = NULL;
  if (ni == c2n.end()) {
    node = forest_->AddNode(r->GetLHS());
    c2n[r->GetLHS()] = node->id_;
    if (is_goal) {
      assert(goal_idx_ == -1);
      goal_idx_ = node->id_;
    } else {
      chart_(i,j).push_back(node->id_);
    }
  } else {
    node = &forest_->nodes_[ni->second];
  }
  forest_->ConnectEdgeToHeadNode(new_edge, node);
}

void PassiveChart::ApplyRules(const int i,
                              const int j,
                              const RuleBin* rules,
                              const Hypergraph::TailNodeVector& tail,
                              const float lattice_cost) {
  const int n = rules->GetNumRules();
  for (int k = 0; k < n; ++k)
    ApplyRule(i, j, rules->GetIthRule(k), tail, lattice_cost);
}

void PassiveChart::ApplyUnaryRules(const int i, const int j) {
  const vector<int>& nodes = chart_(i,j);  // reference is important!
  for (unsigned gi = 0; gi < grammars_.size(); ++gi) {
    if (!grammars_[gi]->HasRuleForSpan(i,j,input_.Distance(i,j))) continue;
    for (unsigned di = 0; di < nodes.size(); ++di) {
      const WordID& cat = forest_->nodes_[nodes[di]].cat_;
      const vector<TRulePtr>& unaries = grammars_[gi]->GetUnaryRulesForRHS(cat);
      for (unsigned ri = 0; ri < unaries.size(); ++ri) {
        // cerr << "At (" << i << "," << j << "): applying " << unaries[ri]->AsString() << endl;
        const Hypergraph::TailNodeVector ant(1, nodes[di]);
        ApplyRule(i, j, unaries[ri], ant, 0);  // may update nodes
      }
    }
  }
}

bool PassiveChart::Parse() {
  size_t in_size_2 = input_.size() * input_.size();
  forest_->nodes_.reserve(in_size_2 * 2);
  size_t res = min(static_cast<size_t>(2000000), static_cast<size_t>(in_size_2 * 1000));
  forest_->edges_.reserve(res);
  goal_idx_ = -1;
  for (unsigned gi = 0; gi < grammars_.size(); ++gi)
    act_chart_[gi]->SeedActiveChart(*grammars_[gi]);

  //added by lijunhui, handling constraints
  string constraints = "";
  SyntacticConstraint *syn_constraint = NULL;
  if ( smeta_ != NULL ) {
    constraints = smeta_->GetSGMLValue(string("constraints"));
    if (constraints == string("")) {
      //no constraints
      syn_constraint = NULL;
    }
    else {
      syn_constraint = new SyntacticConstraint(constraints, smeta_->GetSourceLength());
    }
  }

  if (!SILENT) cerr << "    ";
  for (unsigned l=1; l<input_.size()+1; ++l) {
    if (!SILENT) cerr << '.';
    for (unsigned i=0; i<input_.size() + 1 - l; ++i) {
      unsigned j = i + l;
      for (unsigned gi = 0; gi < grammars_.size(); ++gi) {
        const Grammar& g = *grammars_[gi];
        if (g.HasRuleForSpan(i, j, input_.Distance(i, j))) {
          act_chart_[gi]->AdvanceDotsForAllItemsInCell(i, j, input_);

          const vector<ActiveChart::ActiveItem>& cell = (*act_chart_[gi])(i,j);
          for (vector<ActiveChart::ActiveItem>::const_iterator ai = cell.begin();
               ai != cell.end(); ++ai) {
            const RuleBin* rules = (ai->gptr_->GetRules());
            if (!rules) continue;

            //added by lijunhui, push the constraints
            if (syn_constraint != NULL && grammars_[gi]->GetGrammarName() == smeta_->GetSGMLValue(string("grammar"))) {
              if (syn_constraint->GetConstraint(i, j-1) == 0)
                continue;
            }
            if (syn_constraint != NULL && grammars_[gi]->GetGrammarName() == "ReorderableSyntacticConstrainedGrammar") {
              if (syn_constraint->GetConstraint(i, j-1) == 0)
                continue;
            }

            ApplyRules(i, j, rules, ai->ant_nodes_, ai->lattice_cost);
          }
        }
      }
      ApplyUnaryRules(i,j);

      for (unsigned gi = 0; gi < grammars_.size(); ++gi) {
        const Grammar& g = *grammars_[gi];
        // deal with non-terminals that were just proved
        if (g.HasRuleForSpan(i, j, input_.Distance(i,j)))
          act_chart_[gi]->ExtendActiveItems(i, i, j);
      }
    }
    const vector<int>& dh = chart_(0, input_.size());
    for (unsigned di = 0; di < dh.size(); ++di) {
      const Hypergraph::Node& node = forest_->nodes_[dh[di]];
      if (node.cat_ == goal_cat_) {
        Hypergraph::TailNodeVector ant(1, node.id_);
        ApplyRule(0, input_.size(), goal_rule_, ant, 0);
      }
    }
  }
  if (!SILENT) cerr << endl;
  if (syn_constraint != NULL) delete syn_constraint;

  if (GoalFound())
    forest_->PruneUnreachable(forest_->nodes_.size() - 1);
  return GoalFound();
}

PassiveChart::~PassiveChart() {
  for (unsigned i = 0; i < act_chart_.size(); ++i)
    delete act_chart_[i];
}

ExhaustiveBottomUpParser::ExhaustiveBottomUpParser(
    const string& goal_sym,
    const vector<GrammarPtr>& grammars) :
    goal_sym_(goal_sym),
    grammars_(grammars) {}

bool ExhaustiveBottomUpParser::Parse(const Lattice& input,
                                     SentenceMetadata* smeta,
                                     Hypergraph* forest) const {
  PassiveChart chart(goal_sym_, grammars_, input, smeta, forest);
  const bool result = chart.Parse();
  return result;
}
