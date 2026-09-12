// Unit tests for TextBuffer: the memory-mapped, lazily indexed text model.

#include <tui++/TextBuffer.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

using namespace tui;

namespace {

void expect_length(TextBuffer const &buffer, std::uint64_t length) {
  assert(buffer.length() == length);
}

// Builds content of `lines` lines: "line 0\nline 1\n..." with an optional
// trailing newline (the last text row is otherwise unterminated).
std::string make_text(std::uint64_t lines, bool trailing_newline) {
  std::string text;
  for (std::uint64_t i = 0; i < lines; ++i) {
    text += "line ";
    text += std::to_string(i);
    text += '\n';
  }
  if (not trailing_newline and lines > 0) {
    text.pop_back();
  }
  return text;
}

void test_empty_buffer() {
  auto buffer = TextBuffer::create_empty();
  assert(buffer->length() == 0);
  assert(buffer->known_line_count() == 1);
  assert(buffer->is_fully_scanned());
  auto [line, column] = buffer->offset_to_line(0);
  assert(line == 0 and column == 0);
  assert(buffer->line_start(0) == 0);
}

// The line model: a file with N newlines has N + 1 lines; a trailing newline
// opens a final empty line; an unterminated file counts its last row.
void test_line_model() {
  {
    auto buffer = TextBuffer::create_empty();
    buffer->replace(0, 0, "a\n");
    buffer->scan_to_end();
    assert(buffer->known_line_count() == 2);
    assert(buffer->line_start(0) == 0);
    assert(buffer->line_start(1) == 2);
  }
  {
    auto buffer = TextBuffer::create_empty();
    buffer->replace(0, 0, "a\nb\n");
    buffer->scan_to_end();
    assert(buffer->known_line_count() == 3); // "a", "b", final empty line
    assert(buffer->line_start(2) == 4);
  }
  {
    auto buffer = TextBuffer::create_empty();
    buffer->replace(0, 0, "a\nb");
    buffer->scan_to_end();
    assert(buffer->known_line_count() == 2);
    assert(buffer->line_start(1) == 2);
  }
  {
    auto buffer = TextBuffer::create_empty();
    buffer->replace(0, 0, "a");
    buffer->scan_to_end();
    assert(buffer->known_line_count() == 1);
    assert(buffer->line_start(0) == 0);
    assert(buffer->line_start(1) == 1); // clamped to the content end
  }
}

void test_offsets_and_lines() {
  // "abc\ndef\n\nxyz" -> lines: "abc", "def", "", "xyz"
  auto buffer = TextBuffer::create_empty();
  buffer->replace(0, 0, "abc\ndef\n\nxyz");
  buffer->scan_to_end();
  assert(buffer->known_line_count() == 4);

  auto [line0, col0] = buffer->offset_to_line(0);
  assert(line0 == 0 and col0 == 0);
  auto [line_abc_end, col_abc_end] = buffer->offset_to_line(3); // on the newline
  assert(line_abc_end == 0 and col_abc_end == 3);
  auto [line4, col4] = buffer->offset_to_line(4);
  assert(line4 == 1 and col4 == 0);
  auto [line_xyz_end, col_xyz_end] = buffer->offset_to_line(11);
  assert(line_xyz_end == 3 and col_xyz_end == 2); // the 'z' of "xyz"
  auto [line_eof, col_eof] = buffer->offset_to_line(12); // == length
  assert(line_eof == 3 and col_eof == 3); // end of the unterminated last line

  assert(buffer->line_start(1) == 4);
  assert(buffer->line_start(2) == 8);
  assert(buffer->line_start(3) == 9);
  assert(buffer->line_start(4) == 12); // clamped to the content end
}

// Lazy indexing: only the lines actually scanned are known up front; the
// index grows on demand and stays exact afterwards.
void test_lazy_index() {
  auto buffer = TextBuffer::create_empty();
  auto text = make_text(1000, true); // 1000 newlines -> 1001 lines
  buffer->replace(0, 0, text);

  assert(not buffer->is_fully_scanned());
  assert(buffer->known_line_count() == 1); // nothing scanned yet

  auto line = buffer->ensure_line(42);
  assert(line >= 43);
  auto [found_line, column] = buffer->offset_to_line(std::uint64_t(0));
  (void)found_line;
  (void)column;

  // The first line start must be right even when only a little was scanned.
  assert(buffer->line_start(3) == std::string("line 0\nline 1\nline 2\n").size());

  auto total = buffer->scan_to_end();
  assert(buffer->is_fully_scanned());
  assert(total == 1001);
  assert(buffer->line_start(1000) == text.size()); // final empty line at EOF
  assert(buffer->line_start(999) == text.size() - std::string("line 999\n").size());
}

// Insert/delete keep offsets consistent; edits in the middle rewrite only the
// touched region (checked through the observable content).
void test_edits() {
  auto buffer = TextBuffer::create_empty();
  buffer->replace(0, 0, "hello\nworld\n");
  buffer->replace(6, 5, "there"); // "hello\nthere\n"
  assert(buffer->read(0, 100) == "hello\nthere\n");
  assert(buffer->length() == 12);

  // Delete spanning two lines.
  buffer->replace(0, 6, ""); // "there\n"
  assert(buffer->read(0, 100) == "there\n");
  assert(buffer->length() == 6);

  // Insert in the middle.
  buffer->replace(3, 0, "-inserted-");
  assert(buffer->read(0, 100) == "the-inserted-re\n");
  buffer->scan_to_end();
  assert(buffer->known_line_count() == 2);

  // Offsets keep pointing at the edited content.
  auto [line, column] = buffer->offset_to_line(6);
  assert(line == 0 and column == 6);
}

// An edit crossing page boundaries (pages are 256 KiB) must not corrupt the
// surrounding content.
void test_edit_across_pages() {
  auto buffer = TextBuffer::create_empty();
  auto page = std::uint64_t(1) << 18;
  std::string big;
  big.reserve(std::size_t(page + 1024));
  for (std::uint64_t i = 0; i < page + 1024; ++i) {
    big += char('a' + (i % 26));
  }
  buffer->replace(0, 0, big);

  // Replace a window straddling the first page boundary.
  auto at = page - 32;
  buffer->replace(at, 64, "<<<");
  auto head = buffer->read(0, page - 32 + 64 + 8);
  auto expected = big.substr(0, std::size_t(page - 32)) + "<<<" + big.substr(std::size_t(page - 32 + 64), 8);
  assert(head.substr(0, expected.size()) == expected);

  // The tail beyond the edit is untouched.
  assert(buffer->length() == big.size() - 64 + 3);
  assert(buffer->read(buffer->length() - 16, 16) == big.substr(big.size() - 16));
}

// read_line_ranges resolves consecutive lines in one sweep.
void test_read_line_ranges() {
  auto buffer = TextBuffer::create_empty();
  buffer->replace(0, 0, "aaa\nbb\ncccc\n\nddddd");
  buffer->scan_to_end();
  auto ranges = buffer->read_line_ranges(0, 10);
  assert(ranges.size() == 5);
  assert(ranges[0].start == 0 and ranges[0].end == 3 and ranges[0].has_newline);
  assert(ranges[1].start == 4 and ranges[1].end == 6 and ranges[1].has_newline);
  assert(ranges[2].start == 7 and ranges[2].end == 11 and ranges[2].has_newline);
  assert(ranges[3].start == 12 and ranges[3].end == 12 and ranges[3].has_newline); // empty line
  assert(ranges[4].start == 13 and ranges[4].end == 18 and not ranges[4].has_newline); // unterminated tail
}

void test_search() {
  auto buffer = TextBuffer::create_empty();
  std::string text;
  for (int i = 0; i < 500; ++i) {
    text += "filler line with a needle at ";
    text += std::to_string(i);
    text += " here\n";
  }
  buffer->replace(0, 0, text);

  auto first = buffer->find("needle", 0);
  assert(first and *first == text.find("needle"));
  assert(buffer->find("needle", *first + 1) == buffer->find("needle", *first + 1));
  assert(buffer->find("absent-zzz", 0) == std::nullopt);

  // Byte search (the row separator of a viewport scan).
  auto newline = buffer->find_byte('\n', 0);
  assert(newline and *newline == text.find('\n'));

  // Regexp search returns the full match range.
  auto match = buffer->find_regex("needle at ([0-9]+) here", 0);
  assert(match);
  auto window = buffer->read(match->first, match->second - match->first);
  assert(window.rfind("needle at ", 0) == 0);
  assert(window.find(" here") == window.size() - 5);
  assert(buffer->find_regex("nope (", 0) == std::nullopt); // invalid pattern

  // A regexp spanning the window boundary is still found (overlapping
  // windows); the demo document here is small, so just verify a tail hit.
  auto tail = buffer->find_regex("needle at 499 here", text.size() - 2000);
  assert(tail);
}

// The search options: case-insensitive and whole-word matching, for the plain
// and the regexp searches, and their find-all counterparts.
void test_search_options() {
  // The document is laid out so the three rules differ on every line: a
  // capitalized whole word, a whole word plus the head of "needles", a
  // longer upper-case word, a word glued to a letter and to a digit, and a
  // whole word at the very end of the content.
  auto text = std::string {
      "Needle in a haystack\n"
      "a needle, and needles\n"
      "NEEDLES everywhere\n"
      "xneedle and needle2\n"
      "tail needle" };
  auto buffer = TextBuffer::create_empty();
  buffer->replace(0, 0, text);

  auto ci = SearchOptions { .case_insensitive = true };
  auto ww = SearchOptions { .whole_word = true };
  auto ci_ww = SearchOptions { .case_insensitive = true, .whole_word = true };

  auto line0 = std::uint64_t(text.find("Needle"));
  auto word1 = std::uint64_t(text.find("needle"));
  auto head = std::uint64_t(text.find("needles"));
  auto upper = std::uint64_t(text.find("NEEDLES"));
  auto glued_left = std::uint64_t(text.find("needle", head + 7));
  auto glued_right = std::uint64_t(text.find("needle", glued_left + 6));
  auto tail = std::uint64_t(text.rfind("needle"));

  // Case-insensitive find reaches the capitalized occurrence the plain one
  // skips (ASCII folding, the only one a byte-wise scan can do).
  assert(buffer->find("needle", 0) == word1);
  assert(buffer->find("Needle", 0) == line0);
  assert(buffer->find("needle", 0, ci) == line0);
  assert(buffer->find("needles", 0, ci) == head && "the lower-case plural");
  assert(buffer->find("NEEDLES", 0, ci) == head && "the folded scan reaches the plural before the upper-case line");
  assert(buffer->find("NEEDLES EVERYWHERE", 0, ci) == upper);
  assert(buffer->find("NEEDLES", 0) == upper && "the plain scan still finds the upper-case word");

  // find_all honors the options: the plain pass finds every lower-case
  // occurrence, the whole-word one drops the glued "xneedle", the "needle2"
  // and the head of "needles", and the case-insensitive whole-word pass adds
  // the capitalized line-0 one.
  auto plain = buffer->find_all("needle", 0, 32);
  assert(plain.size() == 5);
  assert(plain == std::vector<std::uint64_t>({ word1, head, glued_left, glued_right, tail }));

  auto all_ci = buffer->find_all("needle", 0, 32, ci);
  assert(all_ci.size() == 7 and std::is_sorted(all_ci.begin(), all_ci.end()));
  assert(all_ci.front() == line0 and all_ci.back() == tail);

  auto all_ww = buffer->find_all("needle", 0, 32, ww);
  assert(all_ww.size() == 2 && "the glued ones and the head of \"needles\" are skipped");
  assert(all_ww[0] == word1 and all_ww[1] == tail);

  auto all_ci_ww = buffer->find_all("needle", 0, 32, ci_ww);
  assert(all_ci_ww.size() == 3);
  assert(all_ci_ww[0] == line0 and all_ci_ww[1] == word1 and all_ci_ww[2] == tail);

  // A whole word with no byte on one side (the content's start or end) is
  // still a word.
  auto edges = TextBuffer::create_empty();
  edges->replace(0, 0, "needle");
  assert(edges->find("needle", 0, ww) == 0);
  assert(edges->find("needle", 0, ci_ww) == 0);

  // The regexp searches take the same options: icase reaches the capitalized
  // occurrences, the whole-word rule still skips the glued ones.
  auto re_ci = buffer->find_regex("n[ae]edle", 0, ci);
  assert(re_ci and re_ci->first == line0);
  auto re_ww = buffer->find_regex("needle[0-9]?", 0, ww);
  assert(re_ww and re_ww->first == word1 && "not the glued xneedle");
  assert(buffer->read(re_ww->first, re_ww->second - re_ww->first) == "needle");

  auto re_all = buffer->find_all_regex("n[ae]edle[s]?", 0, 32, ci);
  assert(re_all.size() == 7);
  assert(std::is_sorted(re_all.begin(), re_all.end(), [](auto const &a, auto const &b) {
    return a.first < b.first;
  }));
  assert(re_all.front().first == line0 and re_all.back().first == tail);
  assert(re_all[2].first == head and re_all[2].second - re_all[2].first == 7 && "the greedy [s]? takes the plural");

  // A pattern matching the empty string terminates (the scan advances a byte)
  // and yields the limited number of zero-length matches; an invalid pattern
  // yields none.
  auto empty = buffer->find_all_regex("x*", 0, 5);
  assert(empty.size() == 5);
  for (auto const &[start, end] : empty) {
    assert(start == end);
  }
  assert(buffer->find_all_regex("nope (", 0, 8).empty() && "an invalid pattern yields no matches");

  // A match that starts exactly on the 1 MiB window core boundary is judged
  // with the byte before it, which lives in the previous window.
  auto boundary = TextBuffer::create_empty();
  auto filler = std::string(std::size_t(1) << 20, 'a');
  boundary->replace(0, 0, filler + "needle needle\n");
  auto after_core = boundary->find_all("needle", 0, 8, ww);
  assert(after_core.size() == 1 && "the first is glued to the filler, the second is a word");
  assert(after_core[0] == (std::size_t(1) << 20) + 7);
  assert(boundary->find("needle", 0, ci_ww) == after_core[0]);
  assert(boundary->find("needle", 0, ww) == after_core[0]);
}

// Opening a real file maps it (read-only) and still allows in-memory edits.
// The SWAR substring scanner must agree with a brute-force reading on
// windows that straddle the 1 MiB scan boundaries, from arbitrary start
// offsets, and the non-overlapping find_all pass must equal repeated
// find() calls. Searching is byte-level (which is exactly right for UTF-8:
// a match found in bytes is a match in the text, whatever the encoding).
void test_find_all() {
  auto const needle = std::string_view { "needle" };
  auto text = std::string { };
  // ~2.6 MiB with an occurrence every few dozen bytes: matches land on and
  // across every 1 MiB window boundary.
  for (std::uint64_t i = 0; text.size() < (2u << 20) + 2000; ++i) {
    text += "line ";
    text += std::to_string(i);
    text += " filler needle marker\n";
  }
  auto buffer = TextBuffer::create_empty();
  buffer->replace(0, 0, text);

  auto expected_from = [&](std::size_t from) {
    auto out = std::vector<std::uint64_t> { };
    auto p = text.find(needle, from);
    while (p != std::string::npos) {
      out.push_back(std::uint64_t(p));
      p = text.find(needle, p + needle.size());
    }
    return out;
  };

  for (auto from : { std::size_t { 0 }, std::size_t { 12345 }, text.size() / 2, text.size() - 10, text.size() }) {
    auto expected = expected_from(from);
    auto got = buffer->find_all(needle, from, 1000000);
    assert(got.size() == expected.size());
    assert(std::equal(got.begin(), got.end(), expected.begin()));

    auto first = buffer->find(needle, from);
    assert(bool(first) == not expected.empty());
    if (not expected.empty()) {
      assert(*first == expected.front());
    }
  }

  // A match straddling a 1 MiB window boundary is found and listed once.
  {
    auto straddle = std::string((1u << 20) - 3, 'a');
    straddle += needle;
    straddle += "tail";
    auto b2 = TextBuffer::create_empty();
    b2->replace(0, 0, straddle);
    auto hits = b2->find_all(needle, 0, 10);
    assert(hits.size() == 1);
    assert(hits[0] == (1u << 20) - 3);
    assert(*b2->find(needle, (1u << 20) - 5) == (1u << 20) - 3);
  }

  // Limit caps, the empty needle and an out-of-range start.
  auto few = buffer->find_all(needle, 0, 5);
  assert(few.size() == 5);
  assert(buffer->find_all(needle, 0, 0).empty());
  auto empty_hits = buffer->find_all("", 7, 10);
  assert(empty_hits.size() == 1 and empty_hits[0] == 7);
  assert(buffer->find_all(needle, text.size() + 5, 10).empty());
}

void test_open_file() {
  auto path = std::string { "test_textbuffer_edit_me.tmp" };
  {
    std::ofstream out(path, std::ios::binary);
    out << "first line\nsecond line\nthird line\n";
  }
  auto buffer = TextBuffer::open_file(path);
  assert(buffer->length() == 34); // 11 + 12 + 11
  assert(buffer->read(0, 11) == "first line\n");
  buffer->scan_to_end();
  assert(buffer->known_line_count() == 4); // 3 newlines -> 4 lines (final empty)

  // Editing a mapped file rewrites only the edited pages in memory.
  buffer->replace(0, 5, "FIRST");
  assert(buffer->read(0, 11) == "FIRST line\n");
  assert(buffer->length() == 34);

  std::remove(path.c_str());
}

// A huge synthetic buffer (a few MiB) exercises the page table and anchors
// without loading anything into one contiguous string.
void test_large_document() {
  auto buffer = TextBuffer::create_empty();
  std::uint64_t const lines = 200000;
  std::string line = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!@#\n";
  auto const bytes_per_line = std::uint64_t(line.size());
  // ~13 MiB: several pages, every 256th line anchored after a full scan.
  std::string content;
  content.reserve(std::size_t(bytes_per_line * lines));
  for (std::uint64_t i = 0; i < lines; ++i) {
    content += line;
  }
  buffer->replace(0, 0, content);

  // Deep random access must stay correct.
  auto total = buffer->scan_to_end();
  assert(total == lines + 1);
  auto at = std::uint64_t(123456) * bytes_per_line;
  auto [line_of, column] = buffer->offset_to_line(at);
  assert(line_of == 123456 and column == 0);

  auto ranges = buffer->read_line_ranges(123450, 5);
  assert(ranges.size() == 5);
  assert(ranges[2].start == std::uint64_t(123452) * bytes_per_line);

  auto hit = buffer->find("ABCDEFG", std::uint64_t(5000) * bytes_per_line);
  assert(hit and *hit >= std::uint64_t(5000) * bytes_per_line);
}

} // namespace

void test_TextBuffer() {
  test_empty_buffer();
  test_line_model();
  test_offsets_and_lines();
  test_lazy_index();
  test_edits();
  test_edit_across_pages();
  test_read_line_ranges();
  test_search();
  test_search_options();
  test_find_all();
  test_open_file();
  test_large_document();
  std::fprintf(stderr, "test_TextBuffer: ok\n");
}
