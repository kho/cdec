#include "context.h"

#include <tr1/unordered_map>
using namespace std;

using boost::make_shared;
using boost::shared_ptr;
using tr1::unordered_map;

namespace pipeline {
class WeightsCacheImpl {
 public:
  WeightsCacheImpl() : cache_() {}
  shared_ptr<vector<weight_t> > Get(const string &path) {
    Map::iterator it = cache_.find(path);
    if (it != cache_.end()) return it->second;
    shared_ptr<vector<weight_t> > ptr = make_shared<vector<weight_t> >();
    Weights::InitFromFile(path, ptr.get());
    cache_[path] = ptr;
    return ptr;
  }
 private:
  typedef unordered_map<string, shared_ptr<vector<weight_t> > > Map;
  Map cache_;
};

WeightsCache::WeightsCache() : pimpl_(new WeightsCacheImpl) {}
WeightsCache::~WeightsCache() {}
shared_ptr<vector<weight_t> > WeightsCache::Get(const string &path) {
  return pimpl_->Get(path);
}
} // namespace pipeline
