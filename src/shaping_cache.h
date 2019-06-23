#ifndef SRC_SHAPING_CACHE_H_
#define SRC_SHAPING_CACHE_H_

#include <harfbuzz/hb.h>

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace shaping_cache {
using std::pair;
using std::string;
using std::unordered_map;
using std::vector;

typedef pair<vector<size_t>, vector<hb_codepoint_t>> CodePointsFacePair;
typedef unordered_map<string, CodePointsFacePair> ShapingCache;

}  // namespace shaping_cache

#endif  // SRC_SHAPING_CACHE_H_
