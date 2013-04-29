#ifndef _FF_HASH_H_
#define _FF_HASH_H_

// A collection of hash features
#include <string>
#include "ff.h"
#include "MurmurHash3.h"

class RuleHashFeatures : public FeatureFunction {
 public:
  RuleHashFeatures(const std::string& param);
 protected:
  virtual void TraversalFeaturesImpl(const SentenceMetadata& smeta,
                                     const HG::Edge& edge,
                                     const std::vector<const void*>& ant_contexts,
                                     SparseVector<double>* features,
                                     SparseVector<double>* estimated_features,
                                     void* context) const;
  virtual void PrepareForInput(const SentenceMetadata& smeta) {}
};


extern const char kHEXMAP[];
// Hashes a string into a WIDTH * 32-bit uint and appends the
// little-endian hexadecimal representation to the output.
template <int WIDTH>
void StringHash(const std::string &in, std::string *out) {
  uint32_t h;
  char buf[8];
  for (size_t i = 0; i < WIDTH; ++i) {
    MurmurHash3_x86_32(static_cast<const void *>(in.c_str()),
                       in.size(), i, static_cast<void *>(&h));
    buf[0] = kHEXMAP[h & 0xf]; h = h >> 4;
    buf[1] = kHEXMAP[h & 0xf]; h = h >> 4;
    buf[2] = kHEXMAP[h & 0xf]; h = h >> 4;
    buf[3] = kHEXMAP[h & 0xf]; h = h >> 4;
    buf[4] = kHEXMAP[h & 0xf]; h = h >> 4;
    buf[5] = kHEXMAP[h & 0xf]; h = h >> 4;
    buf[6] = kHEXMAP[h & 0xf]; h = h >> 4;
    buf[7] = kHEXMAP[h & 0xf];
    out->append(buf, 8);
  }
}


#endif  // _FF_HASH_H_
