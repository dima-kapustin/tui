#include <tui++/SizeRequirements.h>

#include <limits>

namespace tui {

SizeRequirements SizeRequirements::get_tiled_size_requirements(std::vector<SizeRequirements> const &children) {
  auto total = SizeRequirements { };
  for (auto &&req : children) {
    total.minimum = std::min(total.minimum + req.minimum, std::numeric_limits<int>::max());
    total.preferred = std::min(total.preferred + req.preferred, std::numeric_limits<int>::max());
    total.maximum = std::min(total.maximum + req.maximum, std::numeric_limits<int>::max());
  }
  return total;
}

SizeRequirements SizeRequirements::get_aligned_size_requirements(std::vector<SizeRequirements> const &children) {
  auto totalAscent = SizeRequirements { };
  auto totalDescent = SizeRequirements { };
  for (auto &&req : children) {
    int ascent = (req.alignment * req.minimum);
    int descent = req.minimum - ascent;
    totalAscent.minimum = std::max(ascent, totalAscent.minimum);
    totalDescent.minimum = std::max(descent, totalDescent.minimum);

    ascent = (req.alignment * req.preferred);
    descent = req.preferred - ascent;
    totalAscent.preferred = std::max(ascent, totalAscent.preferred);
    totalDescent.preferred = std::max(descent, totalDescent.preferred);

    ascent = (req.alignment * req.maximum);
    descent = req.maximum - ascent;
    totalAscent.maximum = std::max(ascent, totalAscent.maximum);
    totalDescent.maximum = std::max(descent, totalDescent.maximum);
  }

  int min = std::min(totalAscent.minimum + totalDescent.minimum, std::numeric_limits<int>::max());
  int pref = std::min(totalAscent.preferred + totalDescent.preferred, std::numeric_limits<int>::max());
  int max = std::min(totalAscent.maximum + totalDescent.maximum, std::numeric_limits<int>::max());
  float alignment = 0.0f;

  if (min > 0) {
    alignment = (float) totalAscent.minimum / min;
    alignment = alignment > 1.0f ? 1.0f : alignment < 0.0f ? 0.0f : alignment;
  }
  return {min, pref, max, alignment};
}

void SizeRequirements::calculate_tiled_positions(int allocated, std::vector<SizeRequirements> const &children, std::vector<int> &offsets, std::vector<int> &spans, bool forward) {
  auto min = 0;
  auto pref = 0;
  auto max = 0;
  for (auto &&req : children) {
    min += req.minimum;
    pref += req.preferred;
    max += req.maximum;
  }
  if (allocated >= pref) {
    expanded_tile(allocated, min, pref, max, children, offsets, spans, forward);
  } else {
    compressed_tile(allocated, min, pref, max, children, offsets, spans, forward);
  }
}

void SizeRequirements::calculate_aligned_positions(int allocated, const SizeRequirements &total, std::vector<SizeRequirements> const &children, std::vector<int> &offsets, std::vector<int> &spans, bool normal) {
  offsets.resize(children.size());
  spans.resize(children.size());

  auto totalAlignment = normal ? total.alignment : 1.0f - total.alignment;
  auto totalAscent = int(allocated * totalAlignment);
  auto totalDescent = allocated - totalAscent;
  for (auto i = 0U; i < children.size(); ++i) {
    auto &&req = children[i];
    auto alignment = normal ? req.alignment : 1.0f - req.alignment;
    auto maxAscent = int(req.maximum * alignment);
    auto maxDescent = req.maximum - maxAscent;
    auto ascent = std::min(totalAscent, maxAscent);
    auto descent = std::min(totalDescent, maxDescent);

    offsets[i] = totalAscent - ascent;
    spans[i] = std::min(ascent + descent, std::numeric_limits<int>::max());
  }
}

void SizeRequirements::compressed_tile(int allocated, int min, int pref, int max, std::vector<SizeRequirements> const &request, std::vector<int> &offsets, std::vector<int> &spans, bool forward) {
  offsets.resize(request.size());
  spans.resize(request.size());

  // ---- determine what we have to work with ----
  auto totalPlay = std::min(pref - allocated, pref - min);
  auto factor = (pref - min == 0) ? 0.0f : totalPlay / (pref - min);

  // ---- make the adjustments ----
  auto totalOffset = 0;
  if (forward) {
    // lay out with offsets increasing from 0
    totalOffset = 0;
    for (auto i = 0U; i < request.size(); ++i) {
      offsets[i] = totalOffset;
      auto &&req = request[i];
      auto play = factor * (req.preferred - req.minimum);
      spans[i] = (req.preferred - play);
      totalOffset = std::min(totalOffset + spans[i], std::numeric_limits<int>::max());
    }
  } else {
    // lay out with offsets decreasing from the end of the allocation
    totalOffset = allocated;
    for (auto i = 0U; i < request.size(); ++i) {
      auto &&req = request[i];
      auto play = factor * (req.preferred - req.minimum);
      spans[i] = (req.preferred - play);
      offsets[i] = totalOffset - spans[i];
      totalOffset = std::max(totalOffset - spans[i], 0);
    }
  }
}

void SizeRequirements::expanded_tile(int allocated, int min, int pref, int max, std::vector<SizeRequirements> const &request, std::vector<int> &offsets, std::vector<int> &spans, bool forward) {
  offsets.resize(request.size());
  spans.resize(request.size());

  // ---- determine what we have to work with ----
  auto totalPlay = std::min(allocated - pref, max - pref);
  auto factor = (max - pref == 0) ? 0.0f : totalPlay / (max - pref);

  // ---- make the adjustments ----
  auto totalOffset = 0;
  if (forward) {
    // lay out with offsets increasing from 0
    totalOffset = 0;
    for (auto i = 0U; i < request.size(); ++i) {
      offsets[i] = totalOffset;
      auto &&req = request[i];
      auto play = int(factor * (req.maximum - req.preferred));
      spans[i] = std::min(req.preferred + play, std::numeric_limits<int>::max());
      totalOffset = std::min(totalOffset + spans[i], std::numeric_limits<int>::max());
    }
  } else {
    // lay out with offsets decreasing from the end of the allocation
    totalOffset = allocated;
    for (auto i = 0U; i < request.size(); ++i) {
      auto &&req = request[i];
      auto play = int(factor * (req.maximum - req.preferred));
      spans[i] = std::min(req.preferred + play, std::numeric_limits<int>::max());
      offsets[i] = totalOffset - spans[i];
      totalOffset = std::max(totalOffset - spans[i], 0);
    }
  }
}
}
