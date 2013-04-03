#ifndef _NEW_DECODER_COMPONENTS_H_
#define _NEW_DECODER_COMPONENTS_H_

#include "constrain_hg.h"
#include "inspect_hg.h"
#include "generate_hg.h"
#include "rescore_hg.h"
#include "stages.h"
#include "write_hg.h"

#include "pipeline.h"
#include "input.h"
#include "context.h"

namespace pipeline {

typedef Lift<ShowExpectedLength::PIPE_ON(ShouldShowExpectedLength)> OptShowExpectedLength;
typedef Lift<ShowPartition::PIPE_ON(ShouldShowPartition)> OptShowPartition;

//
// Init. forest from translator
//

typedef
  InitForestStage::                     // Sets stage string
  PIPE_COMP(FirstPassTranslate)::       // Build init forest
  PIPE_BIND(Lift<ShowForestStats>)::    // Show forest if succeeds
  PIPE_BIND(OptShowExpectedLength)::    // Show E(L) when required via options
  PIPE_BIND(OptShowPartition)           // Show partition value when required
StdInitForest;

//
// Rescored forest
//

template <int PASS>
class StdRescoreDefs {
 public:
  typedef
    typename RescoreHg<PASS>::          // Actual rescoring
    template PIPE_BIND(Lift<ShowForestStats>):: // Show forest if suceeds
    template PIPE_BIND(OptShowPartition)        // Show partition value when required
  DoRescore;

  typedef
    typename RescoreStage<PASS>::       // Stage string
    template PIPE_BIND(
      typename DoRescore::
      template PIPE_WHEN(ShouldRescore<PASS>)) // Only rescore when
                                               // feature functions or different weight vector are given.
  StdRescore;

  typedef
    typename SummaryHg<PASS>::
    template PIPE_WHEN(ShouldSummarize<PASS>) // Only summarize when asked to and rescoring happened.
  StdSummary;

  typedef
    typename PruneHg<PASS>::
    template PIPE_BIND(Lift<ShowForestStats>)
  DoPrune;

  typedef
    typename RescorePruneStage<PASS>::
    template PIPE_BIND(
      typename DoPrune::
      template PIPE_WHEN(ShouldPrune<PASS>)) // Only prune when asked to.
  StdPrune;

  typedef
    typename StdRescore::
    template PIPE_BIND(StdSummary)::
    template PIPE_BIND(StdPrune)
  StdRescorePass;
};

typedef StdRescoreDefs<1>::StdRescorePass StdRescorePass1;
typedef StdRescoreDefs<2>::StdRescorePass StdRescorePass2;
typedef StdRescoreDefs<3>::StdRescorePass StdRescorePass3;
typedef StdRescorePass1::PIPE_BIND(StdRescorePass2)::PIPE_BIND(StdRescorePass3) Std3PassRescore;

//
// Write results
//

typedef
  ConstrainForestStage::                // Stage string
  PIPE_BIND(
    ConstrainHgWithRef::
    PIPE_BIND(Lift<ShowForestStats>)::
    PIPE_BIND(Lift<ShowViterbiFTree>)::
    PIPE_BIND(OptShowPartition)::
  PIPE_WHEN(InputHasRef))               // Only when input has reference
StdRefIntersect;

typedef Cond<ShouldWrite1Best, Write1Best, WriteKBest::PIPE_ON(ShouldWriteKBest)> OutputSent;

// Compose intead of bind, because we always want to output, even when there is nothing.
typedef
  OutputSentStage::
  PIPE_COMP(OutputSent)::                // OutputSent takes Maybe<Hypergraph *>
  PIPE_BIND(Lift<WriteForestToFile::PIPE_ON(ShouldWriteForest)>)::
  PIPE_BIND(Lift<OptPrintGraphviz>)::
  PIPE_BIND(Lift<OptJoshuaViz>)
StdOutputSent;

//
// Standard translation pipeline
//
typedef StdInitForest::PIPE_BIND(Std3PassRescore)::PIPE_BIND(StdRefIntersect)::PIPE_COMP(StdOutputSent) StdDecode;

} // namespace pipeline

#endif  // _NEW_DECODER_COMPONENTS_H_
