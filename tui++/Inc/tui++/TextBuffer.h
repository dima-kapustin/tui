#pragma once

// TextBuffer - a memory-efficient, editable text model for very large files.
//
// The file is never loaded into memory: it is memory-mapped (paged in by the
// OS on demand) and addressed through a table of fixed-size *pages*. Editing
// rewrites only the pages an edit touches (copy-on-write memory pages spliced
// into the page table); every other page still points at the file mapping, so
// memory stays proportional to the edited bytes, not the file size.
//
// Line structure is indexed lazily: while scanning forward, an anchor is
// stored every SCAN_SAMPLE_LINES lines. Finding a line start / the line of an
// offset then scans at most one sample interval. Edits invalidate the anchors
// after the edited offset.
//
// Line model: content is split by '\n'. A file with N newline bytes has N+1
// lines; an empty file has one (empty) line, and a trailing newline opens a
// final empty line (so a text editor can park its caret below the last text
// row). All offsets are byte offsets into the current (edited) content.

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace tui {

class TextBuffer {
public:
  // A page of content: either a window over the memory-mapped file, or a
  // copy-on-write memory page produced by an edit.
  struct Page {
    // File offset of this page in the mapped file, or -1 for a memory page.
    int64_t file_offset = -1;
    std::shared_ptr<std::string const> data;

    // A memory page's logical length (its chunk of the edited content). A
    // memory page that shares a larger string (the first page of a re-write
    // keeps the whole combined buffer alive so the copies behind it stay
    // cheap) must not claim the shared string's full size.
    uint32_t data_length = 0;

    uint32_t length(std::uint64_t file_length) const {
      if (this->file_offset >= 0) {
        auto remaining = file_length - std::uint64_t(this->file_offset);
        return uint32_t(std::min<std::uint64_t>(remaining, PAGE_SIZE));
      }
      return this->data_length;
    }
  };

  // One line anchor recorded while scanning: (line, start offset of the line),
  // stored every SCAN_SAMPLE_LINES lines.
  struct Sample {
    std::uint64_t line;
    std::uint64_t offset;
  };

  // A resolved line: [start, end) is the line's bytes, `end` points at the
  // terminating '\n' (or at the content end when the line is unterminated).
  struct LineRange {
    std::uint64_t start;
    std::uint64_t end;
    bool has_newline;
  };

  static constexpr std::uint64_t PAGE_SIZE = 1 << 18; // 256 KiB
  static constexpr std::uint64_t SCAN_SAMPLE_LINES = 256;

public:
  // Opens `path` for editing. Throws std::runtime_error when the file cannot
  // be opened or mapped.
  static std::shared_ptr<TextBuffer> open_file(std::string const &path);

  // An empty in-memory buffer (for a new/scratch text).
  static std::shared_ptr<TextBuffer> create_empty();

  ~TextBuffer();

  // Total length of the current content, in bytes.
  std::uint64_t length() const {
    return this->starts.back();
  }

  // Materializes up to `len` bytes at `offset` (clamped to the content end).
  std::string read(std::uint64_t offset, std::uint64_t len) const;

  // Replaces [offset, offset + delete_len) with `replacement`. Only the pages
  // overlapping the edit are copied; the rest of the file is untouched.
  void replace(std::uint64_t offset, std::uint64_t delete_len, std::string_view replacement);

  // --- lazy line index ------------------------------------------------------

  // The number of lines known so far (grows as the buffer is scanned).
  std::uint64_t known_line_count() const {
    return this->frontier_line + 1;
  }

  // True when the scan frontier reached the end of the content (the total
  // line count is then exact).
  bool is_fully_scanned() const {
    return this->have_tail;
  }

  // Scans forward until line `line` is known (or the content ends). Returns
  // the number of lines known afterwards.
  std::uint64_t ensure_line(std::uint64_t line);

  // Scans the whole content (for Ctrl+End or an exact total); returns the
  // total number of lines.
  std::uint64_t scan_to_end();

  // Scans the content up to `offset` so that anchors cover it (one pass over
  // any unscanned gap; a no-op when already scanned that far). Used after a
  // long jump (search hit, Ctrl+End) so caret operations near the jump do not
  // rescan the gap on every key.
  void ensure_scanned_to(std::uint64_t offset);

  // Start offset of `line`. `line` must be < known_line_count() (call
  // ensure_line first); line == known_line_count() returns the content end.
  std::uint64_t line_start(std::uint64_t line) const;

  // (line, byte column) of `offset`; requires the line to be known.
  std::pair<std::uint64_t, std::uint64_t> offset_to_line(std::uint64_t offset) const;

  // Resolves the byte ranges of up to `count` consecutive lines starting at
  // `first_line` in one forward pass (no per-line anchor rescans), so a text
  // viewport can fill itself cheaply. Fewer ranges are returned when the
  // content runs out; `first_line` must already be known.
  std::vector<LineRange> read_line_ranges(std::uint64_t first_line, std::uint64_t count) const;

  // --- search ---------------------------------------------------------------

  // Finds the first occurrence of `needle` at or after `from`. The scan is
  // windowed (memory stays bounded); matches may span page boundaries.
  std::optional<std::uint64_t> find(std::string_view needle, std::uint64_t from) const;

  // Every non-overlapping occurrence of `needle` at or after `from`, in
  // order, capped at `limit` results -- one single forward pass with the same
  // window semantics and SWAR scanner as find(). An empty needle yields the
  // single offset `from` (like find()).
  std::vector<std::uint64_t> find_all(std::string_view needle, std::uint64_t from, std::size_t limit) const;

  // Finds the first ECMAScript-regexp match at or after `from`, running the
  // regex over overlapping windows of REGEX_WINDOW bytes. Returns (start,
  // end) of the match; matches longer than the window overlap may straddle a
  // boundary and go unnoticed, and pathological patterns are bounded by the
  // window size. Invalid patterns yield nullopt.
  std::optional<std::pair<std::uint64_t, std::uint64_t>> find_regex(std::string const &pattern, std::uint64_t from) const;

  // The byte offset of the first `byte` at or after `from`, scanning in
  // bounded windows; nullopt when the content ends without one. `limit`
  // bounds the scan (default: the content end).
  std::optional<std::uint64_t> find_byte(char byte, std::uint64_t from, std::uint64_t limit = UINT64_MAX) const;

private:
  TextBuffer() = default;

  void map_file(std::string const &path);
  void build_file_pages();

  char const *page_bytes(std::size_t page_index) const;
  int page_index_of(std::uint64_t offset) const;

  // Drops line anchors at/after `offset` and rewinds the scan frontier to the
  // last surviving anchor (an edit before them invalidates every line count).
  void invalidate_samples_at(std::uint64_t offset);

  // Scans newlines in [frontier_offset, stop); moves the frontier to the start
  // of the last line fully seen and stores anchors every SCAN_SAMPLE_LINES
  // lines. When stop == length() the scan is complete and the total line
  // count becomes exact.
  void scan(std::uint64_t stop);

  // The logical content is the concatenation of the pages; `starts[i]` is the
  // start offset of page i, `starts.back()` the total length.
  std::vector<Page> pages;
  std::vector<std::uint64_t> starts;

  std::vector<Sample> samples;

  // The scan frontier: `frontier_line` newlines have been counted and the
  // start of that line is `frontier_offset`.
  std::uint64_t frontier_line = 0;
  std::uint64_t frontier_offset = 0;

  std::uint64_t file_length = 0; // total bytes of the mapped file
  bool have_tail = false;        // the content end was scanned

  struct Mapping;
  std::unique_ptr<Mapping> mapping;

  static constexpr std::uint64_t REGEX_WINDOW = 8ull << 20;    // 8 MiB
  static constexpr std::uint64_t REGEX_OVERLAP = 64ull << 10;  // 64 KiB
};

}
