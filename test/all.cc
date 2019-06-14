#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "../src/lru_freshness_cache.h"

TEST_CASE("LRU cache works") {
  lru_freshness_cache::LRUFreshnessCache<std::string, int> c(3);

  CHECK(c.size() == 0);
  CHECK(!c.exists("a"));
  CHECK(!c.contains_stale());
  CHECK(!c.full());

  CHECK(c.put("a", 17));
  CHECK(c.size() == 1);
  CHECK(c.exists("a"));
  CHECK(c.is_fresh("a"));
  CHECK(!c.contains_stale());
  CHECK(!c.full());

  CHECK(c.put("b", -12));
  CHECK(c.size() == 2);
  CHECK(c.exists("a"));
  CHECK(c.exists("b"));
  CHECK(c.is_fresh("b"));
  CHECK(c.is_fresh("b"));
  CHECK(!c.contains_stale());
  CHECK(!c.full());

  CHECK(c.put("c", 43));
  CHECK(c.size() == 3);
  CHECK(c.exists("a"));
  CHECK(c.exists("b"));
  CHECK(c.exists("c"));
  CHECK(c.is_fresh("b"));
  CHECK(c.is_fresh("b"));
  CHECK(c.is_fresh("c"));
  CHECK(!c.contains_stale());
  CHECK(c.full());

  CHECK(c.get("a") != nullptr);
  CHECK(c.get("a") != nullptr);
  CHECK(c.get("c") != nullptr);
  // So now the order is c a b

  // I cannot put d since the cache is filled
  CHECK(!c.put("d", 14));

  // Set all items to stale i.e. forall keys : fresh[key] = false
  c.invalidate();

  CHECK(c.put("d", 14));
  CHECK(c.size() == 3);
  CHECK(c.exists("d"));
  CHECK(c.exists("c"));
  CHECK(c.exists("a"));
  CHECK(c.contains_stale());  // c and a should be stale
  CHECK(c.is_fresh("d"));
  CHECK(!c.is_fresh("c"));
  CHECK(!c.is_fresh("a"));
  CHECK(c.full());

  CHECK(c.put("e", -1));
  CHECK(c.size() == 3);
  CHECK(c.exists("e"));
  CHECK(c.exists("d"));
  CHECK(c.exists("c"));
  CHECK(c.contains_stale());  // c should be stale
  CHECK(c.is_fresh("e"));
  CHECK(c.is_fresh("d"));
  CHECK(!c.is_fresh("c"));
  CHECK(c.full());

  CHECK(c.put("f", -1));
  CHECK(c.size() == 3);
  CHECK(c.exists("f"));
  CHECK(c.exists("e"));
  CHECK(c.exists("d"));
  CHECK(c.contains_stale());
  CHECK(c.is_fresh("f"));
  CHECK(c.is_fresh("e"));
  CHECK(c.is_fresh("d"));
  CHECK(c.full());
}
