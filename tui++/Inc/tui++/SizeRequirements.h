#pragma once

#include <vector>

namespace tui {

struct SizeRequirements {
  int minimum;
  int preferred;
  int maximum;
  float alignment;

  static SizeRequirements get_tiled_size_requirements(std::vector<SizeRequirements> const &children);
  static SizeRequirements get_aligned_size_requirements(std::vector<SizeRequirements> const &children);

  static void calculate_tiled_positions(int allocated, std::vector<SizeRequirements> const &children, std::vector<int> &offsets, std::vector<int> &spans) {
    calculate_tiled_positions(allocated, children, offsets, spans, true);
  }
  static void calculate_tiled_positions(int allocated, std::vector<SizeRequirements> const &children, std::vector<int> &offsets, std::vector<int> &spans, bool forward);

  static void calculate_aligned_positions(int allocated, const SizeRequirements &total, std::vector<SizeRequirements> const &children, std::vector<int> &offsets, std::vector<int> &spans) {
    calculate_aligned_positions(allocated, total, children, offsets, spans, true);
  }
  static void calculate_aligned_positions(int allocated, const SizeRequirements &total, std::vector<SizeRequirements> const &children, std::vector<int> &offsets, std::vector<int> &spans, bool normal);

private:
  static void compressed_tile(int allocated, int min, int pref, int max, std::vector<SizeRequirements> const &request, std::vector<int> &offsets, std::vector<int> &spans, bool forward);
  static void expanded_tile(int allocated, int min, int pref, int max, std::vector<SizeRequirements> const &request, std::vector<int> &offsets, std::vector<int> &spans, bool forward);
};

}
