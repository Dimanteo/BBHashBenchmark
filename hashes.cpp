#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <map>
#include <random>
#include <utility>
#include <vector>

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

  const std::vector<key_t> &keys() const;

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

} // namespace hash_bench

using namespace hash_bench;

int main(int argc, char **argv) {
  if (argc < 4) {
    std::cout << "Usage:" << argv[0]
              << " <num entries> <num tasks> <num runs>\n";
    return 1;
  }
  size_t num_entries = std::strtoull(argv[1], nullptr, 10);
  size_t num_iters = std::strtoull(argv[2], nullptr, 10);
  size_t num_runs = std::strtoull(argv[3], nullptr, 10);
  BenchSet bench_set(RND_SEED);
  bench_set.gen(num_entries);
  const key_value_t *data = bench_set.data();
  std::vector<hash_bench::key_t> queries = bench_set.make_task(num_iters);
  std::vector<double> results;
  for (size_t r = 0; r < num_runs; ++r) {
    auto start = std::chrono::high_resolution_clock::now();
    for (auto key : queries) {
      get_val_binsearch(key, data, num_entries);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto time =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    results.push_back(time.count());
  }
  double sum = std::accumulate(results.begin(), results.end(), 0);
  std::cout << "Binsearch " << sum / results.size() << " us\n";
  return 0;
}