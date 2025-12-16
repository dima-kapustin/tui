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
  auto total_ascent = SizeRequirements { };
  auto total_descent = SizeRequirements { };
  for (auto &&req : children) {
    int ascent = (req.alignment * req.minimum);
    int descent = req.minimum - ascent;
    total_ascent.minimum = std::max(ascent, total_ascent.minimum);
    total_descent.minimum = std::max(descent, total_descent.minimum);

    ascent = (req.alignment * req.preferred);
    descent = req.preferred - ascent;
    total_ascent.preferred = std::max(ascent, total_ascent.preferred);
    total_descent.preferred = std::max(descent, total_descent.preferred);

    ascent = (req.alignment * req.maximum);
    descent = req.maximum - ascent;
    total_ascent.maximum = std::max(ascent, total_ascent.maximum);
    total_descent.maximum = std::max(descent, total_descent.maximum);
  }

  auto min = std::min(total_ascent.minimum + total_descent.minimum, std::numeric_limits<int>::max());
  auto pref = std::min(total_ascent.preferred + total_descent.preferred, std::numeric_limits<int>::max());
  auto max = std::min(total_ascent.maximum + total_descent.maximum, std::numeric_limits<int>::max());
  auto alignment = 0.0f;

  if (min > 0) {
    alignment = (float) total_ascent.minimum / min;
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

  auto total_alignment = normal ? total.alignment : 1.0f - total.alignment;
  auto total_ascent = int(allocated * total_alignment);
  auto total_descent = allocated - total_ascent;
  for (auto i = 0U; i < children.size(); ++i) {
    auto &&req = children[i];
    auto alignment = normal ? req.alignment : 1.0f - req.alignment;
    auto max_ascent = int(req.maximum * alignment);
    auto max_descent = req.maximum - max_ascent;
    auto ascent = std::min(total_ascent, max_ascent);
    auto descent = std::min(total_descent, max_descent);

    offsets[i] = total_ascent - ascent;
    spans[i] = std::min(ascent + descent, std::numeric_limits<int>::max());
  }
}

void SizeRequirements::compressed_tile(int allocated, int min, int pref, int max, std::vector<SizeRequirements> const &request, std::vector<int> &offsets, std::vector<int> &spans, bool forward) {
  offsets.resize(request.size());
  spans.resize(request.size());

  // ---- determine what we have to work with ----
  auto total_play = std::min(pref - allocated, pref - min);
  auto factor = (pref - min == 0) ? 0.0f : total_play / (pref - min);

  // ---- make the adjustments ----
  auto total_offset = 0;
  if (forward) {
    // lay out with offsets increasing from 0
    total_offset = 0;
    for (auto i = 0U; i < request.size(); ++i) {
      offsets[i] = total_offset;
      auto &&req = request[i];
      auto play = factor * (req.preferred - req.minimum);
      spans[i] = (req.preferred - play);
      total_offset = std::min(total_offset + spans[i], std::numeric_limits<int>::max());
    }
  } else {
    // lay out with offsets decreasing from the end of the allocation
    total_offset = allocated;
    for (auto i = 0U; i < request.size(); ++i) {
      auto &&req = request[i];
      auto play = factor * (req.preferred - req.minimum);
      spans[i] = (req.preferred - play);
      offsets[i] = total_offset - spans[i];
      total_offset = std::max(total_offset - spans[i], 0);
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
  auto total_offset = 0;
  if (forward) {
    // lay out with offsets increasing from 0
    total_offset = 0;
    for (auto i = 0U; i < request.size(); ++i) {
      offsets[i] = total_offset;
      auto &&req = request[i];
      auto play = int(factor * (req.maximum - req.preferred));
      spans[i] = std::min(req.preferred + play, std::numeric_limits<int>::max());
      total_offset = std::min(total_offset + spans[i], std::numeric_limits<int>::max());
    }
  } else {
    // lay out with offsets decreasing from the end of the allocation
    total_offset = allocated;
    for (auto i = 0U; i < request.size(); ++i) {
      auto &&req = request[i];
      auto play = int(factor * (req.maximum - req.preferred));
      spans[i] = std::min(req.preferred + play, std::numeric_limits<int>::max());
      offsets[i] = total_offset - spans[i];
      total_offset = std::max(total_offset - spans[i], 0);
    }
  }
}
}
