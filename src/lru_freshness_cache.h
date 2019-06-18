#ifndef LETTERA_LRU_WITH_FRESHNESS_H
#define LETTERA_LRU_WITH_FRESHNESS_H

#include <cassert>
#include <list>
#include <unordered_map>

namespace lru_freshness_cache {
using std::list;
using std::unordered_map;

template <typename key_t, typename value_t>
class LRUFreshnessCache {
  typedef typename std::pair<key_t, value_t> key_value_pair_t;
  typedef typename std::list<key_value_pair_t>::iterator list_iterator_t;

 private:
  size_t max_size_;
  size_t fresh_count_ = 0;

  list<key_value_pair_t> queue_;
  unordered_map<key_t, list_iterator_t> queue_pos_;
  unordered_map<key_t, bool> fresh;

 public:
  LRUFreshnessCache(size_t max_size) : max_size_(max_size), fresh(max_size) {}

  bool put(const key_t& key, const value_t& value) {
    // If the the cache hasn't space left and it cannot replace any stale value
    if (full() && !contains_stale()) {
      return false;
    }

    auto it = queue_pos_.find(key);

    if (it != queue_pos_.end()) {
      queue_.erase(it->second);
      queue_pos_.erase(it);
    }

    queue_.push_front(key_value_pair_t(key, value));
    queue_pos_[key] = queue_.begin();

    if (!fresh[key]) {
      fresh[key] = true;
      fresh_count_++;
    }

    if (queue_pos_.size() > max_size_) {
      auto last = queue_.end();
      last--;
      queue_pos_.erase(last->first);
      queue_.pop_back();

      fresh.erase(last->first);
      fresh_count_--;
    }

    return true;
  }

  value_t* get(const key_t& key) {
    auto it = queue_pos_.find(key);

    if (it == queue_pos_.end()) {
      return nullptr;
    }

    queue_.splice(queue_.begin(), queue_, it->second);

    fresh[key] = true;
    fresh_count_++;

    return &(it->second->second);
  }

  bool exists(const key_t& key) const {
    return queue_pos_.find(key) != queue_pos_.end();
  }
  bool full() const { return queue_pos_.size() == max_size_; }
  bool contains_stale() const { return fresh_count_ < queue_pos_.size(); }
  size_t size() const { return queue_pos_.size(); }
  bool is_fresh(const key_t& key) const { return fresh.at(key); }
  void invalidate() {
    for (const auto& kv_pair : fresh) {
      fresh[kv_pair.first] = false;
    }
    fresh_count_ = 0;
  }
};

}  // namespace lru_freshness_cache

#endif /* LETTERA_LRU_WITH_FRESHNESS_H */
