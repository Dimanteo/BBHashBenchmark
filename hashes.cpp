#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <map>
#include <random>
#include <utility>
#include <vector>

#include "BBHash/BooPHF.h"

#define RND_SEED 42

namespace hash_bench {

using key_t = uint64_t;
using value_t = uint64_t;
using key_value_t = std::pair<key_t, value_t>;

class BenchSet final {
public:
  BenchSet(int seed) : m_rand_gen(seed) {}

  auto data() const { return m_data.data(); }

  void gen(size_t num_entries) {
    m_keys.clear();
    m_data.clear();
    std::map<uint64_t, uint64_t> map;
    std::uniform_int_distribution<uint64_t> distrib(0x10000000, 0x80000000);
    for (size_t i = 1; i <= num_entries; ++i) {
      uint64_t key = distrib(m_rand_gen);
      while (map.count(key)) {
        key = distrib(m_rand_gen);
      }
      map[key] = i;
    }
    for (const key_value_t &key_val : map) {
      m_keys.emplace_back(key_val.first);
      m_data.emplace_back(key_val);
    }
    assert(size() == num_entries);
  }

  size_t size() const { return m_data.size(); }

  const std::vector<key_t> &keys() const { return m_keys; }
  const std::vector<key_value_t> &key_vals() const { return m_data; }

  std::vector<key_t> make_task(size_t n_iters) {
    std::vector<key_t> task;
    std::vector<key_t> shuffle_buf;
    std::copy(m_keys.begin(), m_keys.end(), std::back_inserter(shuffle_buf));
    for (size_t i = 0; i < n_iters; ++i) {
      std::shuffle(shuffle_buf.begin(), shuffle_buf.end(), m_rand_gen);
      std::copy(shuffle_buf.begin(), shuffle_buf.end(),
                std::back_inserter(task));
    }
    return task;
  }

private:
  std::vector<key_t> m_keys;
  std::vector<key_value_t> m_data;
  std::mt19937 m_rand_gen;
};

value_t get_val_binsearch(key_t key, const key_value_t *data,
                          size_t num_entries) {
  size_t left = 0, right = num_entries - 1;
  while (left + 1 != right) {
    size_t mid = (left + right) / 2;
    auto flatten_data = reinterpret_cast<const uint64_t *>(data + mid);
    key_t mid_key = *flatten_data;
    if (key == mid_key) {
      return *(flatten_data + 1);
    }
    if (key < mid_key) {
      right = mid;
    } else {
      left = mid;
    }
  }
  return data[right].first == key ? data[right].second : data[left].second;
}

typedef boomphf::SingleHashFunctor<uint64_t> hasher_t;
typedef boomphf::mphf<uint64_t, hasher_t> boophf_t;

class BBHashMap final {
public:
  BBHashMap(size_t N, const BenchSet &bench, double gamma)
      : m_bphf(N, boomphf::range(bench.keys().begin(), bench.keys().end()), 1,
               gamma) {
    m_vals.resize(N);
    for (key_value_t key_val : bench.key_vals()) {
      size_t idx = m_bphf.lookup(key_val.first);
      assert(idx < N);
      m_vals[idx] = std::move(key_val);
    }
  }

  value_t get_val(key_t key) {
    auto &&key_val = m_vals[m_bphf.lookup(key)];
    if (key == key_val.first) {
      return key_val.second;
    }
    return 0;
  }

private:
  boophf_t m_bphf;
  // We need to store key to check for invalid key
  std::vector<key_value_t> m_vals;
};

} // namespace hash_bench

using namespace hash_bench;

int main(int argc, char **argv) {
  if (argc < 5) {
    std::cout << "Usage:" << argv[0]
              << " <num entries> <num tasks> <num runs> <0 - bin search, 1 - "
                 "bbhash>\n";
    return 1;
  }
  size_t num_entries = std::strtoull(argv[1], nullptr, 10);
  size_t num_iters = std::strtoull(argv[2], nullptr, 10);
  size_t num_runs = std::strtoull(argv[3], nullptr, 10);
  unsigned bench_type = std::strtoull(argv[4], nullptr, 10);
  BenchSet bench_set(RND_SEED);
  bench_set.gen(num_entries);
  std::vector<hash_bench::key_t> queries = bench_set.make_task(num_iters);
  std::vector<double> results;

  // Bench bin search
  if (bench_type == 0) {
    const key_value_t *data = bench_set.data();
    value_t blob = 0;
    for (size_t r = 0; r < num_runs; ++r) {
      auto start = std::chrono::high_resolution_clock::now();
      for (auto key : queries) {
        blob = blob ^ get_val_binsearch(key, data, num_entries);
      }
      auto end = std::chrono::high_resolution_clock::now();
      auto time =
          std::chrono::duration_cast<std::chrono::microseconds>(end - start);
      results.push_back(time.count());
    }
    std::cout << "Values blob " << blob << "\n";
    std::cout << "BinSearch ";
  } else if (bench_type == 1) {
    // Bench the mphf
    // lowest bit/elem is achieved with gamma=1, higher values lead to larger
    // mphf but faster construction/query
    BBHashMap bbhash_map(num_entries, bench_set, 1.0);
    value_t blob = 0;
    for (size_t r = 0; r < num_runs; ++r) {
      auto start = std::chrono::high_resolution_clock::now();
      for (auto key : queries) {
        blob ^= bbhash_map.get_val(key);
      }
      auto end = std::chrono::high_resolution_clock::now();
      auto time =
          std::chrono::duration_cast<std::chrono::microseconds>(end - start);
      results.push_back(time.count());
    }
    std::cout << "Values blob " << blob << "\n";
    std::cout << "BBhash ";
  }
  double sum = std::accumulate(results.begin(), results.end(), 0);
  std::cout << sum / results.size() << " us\n";
  return 0;
}