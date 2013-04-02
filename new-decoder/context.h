#ifndef _NEW_DECODER_CONTEXT_H_
#define _NEW_DECODER_CONTEXT_H_

#include <string>
#include <vector>

#include <boost/program_options.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include "sentence_metadata.h"
#include "weights.h"
#include "oracle_bleu.h"

namespace pipeline {
// Cache weights by path to avoid repeatedly loading large weights
// file.
class WeightsCacheImpl;
class WeightsCache {
 public:
  WeightsCache();
  ~WeightsCache();
  boost::shared_ptr<std::vector<weight_t> > Get(const std::string &);
 private:
  boost::scoped_ptr<WeightsCacheImpl> pimpl_;
};

// Context that stores shared information across Functions.
class Context {
 public:
  Context() : sent_id(-1) {}
  // Metadata of current sentence
  // TODO: default ctor, assignment and copy for SentenceMetadata so that we no longer need the ptr
  boost::scoped_ptr<SentenceMetadata> smeta_ptr;
  // `init_weights_ptr` is set at construction of `FirstPassTranslate`
  // Any pipe that uses a weight vector is responsible for updating `last_weights_ptr`
  boost::shared_ptr<std::vector<weight_t> > init_weights_ptr, last_weights_ptr;
  OracleBleu oracle;
  int sent_id;
  WeightsCache weights;
  std::string stage;
};
}      // namespace pipeline
#endif  // _NEW_DECODER_CONTEXT_H_
