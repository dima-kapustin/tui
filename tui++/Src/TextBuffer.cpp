#include <tui++/TextBuffer.h>

#include <bit>
#include <cstring>
#include <regex>
#include <stdexcept>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace tui {

struct TextBuffer::Mapping {
#ifdef _WIN32
  HANDLE file = INVALID_HANDLE_VALUE;
  HANDLE mapping = nullptr;
  char *base = nullptr;
  ~Mapping() {
    if (this->base) {
      ::UnmapViewOfFile(this->base);
    }
    if (this->mapping) {
      ::CloseHandle(this->mapping);
    }
    if (this->file != INVALID_HANDLE_VALUE) {
      ::CloseHandle(this->file);
    }
  }
#else
  int fd = -1;
  char *base = nullptr;
  ~Mapping() {
    if (this->base) {
      ::munmap(this->base, 0);
    }
    if (this->fd >= 0) {
      ::close(this->fd);
    }
  }
#endif
};

TextBuffer::~TextBuffer() = default;

std::shared_ptr<TextBuffer> TextBuffer::create_empty() {
  auto buffer = std::shared_ptr<TextBuffer>(new TextBuffer());
  buffer->file_length = 0;
  buffer->have_tail = true;
  buffer->starts = { 0 };
  return buffer;
}

void TextBuffer::map_file(std::string const &path) {
  this->mapping = std::make_unique<Mapping>();

#ifdef _WIN32
  this->mapping->file = ::CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (this->mapping->file == INVALID_HANDLE_VALUE) {
    throw std::runtime_error("cannot open " + path);
  }
  LARGE_INTEGER size { };
  ::GetFileSizeEx(this->mapping->file, &size);
  this->file_length = std::uint64_t(size.QuadPart);
  if (this->file_length > 0) {
    this->mapping->mapping = ::CreateFileMappingA(this->mapping->file, nullptr, PAGE_READONLY, size.HighPart, size.LowPart, nullptr);
    if (not this->mapping->mapping) {
      throw std::runtime_error("cannot map " + path);
    }
    this->mapping->base = static_cast<char*>(::MapViewOfFile(this->mapping->mapping, FILE_MAP_READ, 0, 0, 0));
    if (not this->mapping->base) {
      throw std::runtime_error("cannot map " + path);
    }
  }
#else
  this->mapping->fd = ::open(path.c_str(), O_RDONLY);
  if (this->mapping->fd < 0) {
    throw std::runtime_error("cannot open " + path);
  }
  struct stat st { };
  ::fstat(this->mapping->fd, &st);
  this->file_length = std::uint64_t(st.st_size);
  if (this->file_length > 0) {
    this->mapping->base = static_cast<char*>(::mmap(nullptr, this->file_length, PROT_READ, MAP_PRIVATE, this->mapping->fd, 0));
    if (this->mapping->base == MAP_FAILED) {
      throw std::runtime_error("cannot map " + path);
    }
  }
#endif
}

void TextBuffer::build_file_pages() {
  auto page_count = std::size_t((this->file_length + PAGE_SIZE - 1) / PAGE_SIZE);
  this->pages.reserve(page_count);
  for (std::size_t i = 0; i < page_count; ++i) {
    Page page;
    page.file_offset = int64_t(i) * int64_t(PAGE_SIZE);
    this->pages.emplace_back(page);
  }
  this->starts.assign(this->pages.size() + 1, 0);
  for (std::size_t i = 0; i < this->pages.size(); ++i) {
    this->starts[i + 1] = this->starts[i] + this->pages[i].length(this->file_length);
  }
}

std::shared_ptr<TextBuffer> TextBuffer::open_file(std::string const &path) {
  auto buffer = std::shared_ptr<TextBuffer>(new TextBuffer());
  buffer->map_file(path);
  buffer->build_file_pages();
  return buffer;
}

char const *TextBuffer::page_bytes(std::size_t page_index) const {
  auto const &page = this->pages[page_index];
  if (page.file_offset >= 0) {
    return this->mapping->base + page.file_offset;
  }
  return page.data->data();
}

int TextBuffer::page_index_of(std::uint64_t offset) const {
  auto it = std::upper_bound(this->starts.begin(), this->starts.end(), offset);
  auto i = it - this->starts.begin() - 1;
  return int(std::max<std::ptrdiff_t>(0, i));
}

std::string TextBuffer::read(std::uint64_t offset, std::uint64_t len) const {
  if (len == 0 or offset >= this->length()) {
    return {};
  }
  len = std::min(len, this->length() - offset);
  std::string out;
  out.reserve(std::size_t(len));
  auto pos = offset;
  auto end = offset + len;
  while (pos < end) {
    auto page = std::size_t(page_index_of(pos));
    auto in_page = pos - this->starts[page];
    auto page_len = std::uint64_t(this->pages[page].length(this->file_length));
    auto take = std::min(page_len - in_page, end - pos);
    out.append(page_bytes(page) + in_page, std::size_t(take));
    pos += take;
  }
  return out;
}

void TextBuffer::replace(std::uint64_t offset, std::uint64_t delete_len, std::string_view replacement) {
  auto total = this->length();
  offset = std::min(offset, total);
  delete_len = std::min(delete_len, total - offset);
  invalidate_samples_at(offset);

  if (this->pages.empty()) {
    // Pure insert into an empty buffer.
    std::string content(replacement);
    if (not content.empty()) {
      auto data = std::make_shared<std::string const>(std::move(content));
      for (std::size_t at = 0; at < data->size(); at += PAGE_SIZE) {
        auto take = std::min<std::size_t>(PAGE_SIZE, data->size() - at);
        Page page;
        page.data = at == 0 ? data : std::make_shared<std::string const>(data->substr(at, take));
        page.data_length = uint32_t(take);
        this->pages.emplace_back(std::move(page));
      }
    }
    this->starts.assign(this->pages.size() + 1, 0);
    for (std::size_t i = 0; i < this->pages.size(); ++i) {
      this->starts[i + 1] = this->starts[i] + this->pages[i].length(this->file_length);
    }
    return;
  }

  auto first = std::size_t(page_index_of(offset));
  auto last = std::size_t(page_index_of(offset + delete_len == total ? total - 1 : offset + delete_len));

  // The surviving prefix of the first affected page and the surviving suffix
  // of the last one (at most one page each, unless both are the same page).
  auto prefix = std::string { };
  auto offset_in_a = offset - this->starts[first];
  if (offset_in_a > 0) {
    auto const &page_a = this->pages[first];
    auto take = std::min<std::uint64_t>(offset_in_a, page_a.length(this->file_length));
    prefix.assign(page_bytes(first), std::size_t(take));
  }

  auto suffix = std::string { };
  auto delete_end = offset + delete_len;
  auto const &page_b = this->pages[last];
  auto offset_in_b = delete_end - this->starts[last];
  if (offset_in_b < page_b.length(this->file_length)) {
    auto take = page_b.length(this->file_length) - offset_in_b;
    suffix.assign(page_bytes(last) + offset_in_b, std::size_t(take));
  }

  auto combined = prefix + std::string(replacement) + suffix;
  auto data = std::make_shared<std::string const>(std::move(combined));

  auto removed = last - first + 1;
  this->pages.erase(this->pages.begin() + std::ptrdiff_t(first), this->pages.begin() + std::ptrdiff_t(first + removed));

  auto at = std::size_t(0);
  auto insert_before = this->pages.begin() + std::ptrdiff_t(first);
  while (at < data->size()) {
    auto take = std::min<std::size_t>(PAGE_SIZE, data->size() - at);
    Page page;
    page.data = at == 0 ? data : std::make_shared<std::string const>(data->substr(at, take));
    page.data_length = uint32_t(take);
    this->pages.insert(insert_before++, std::move(page));
    at += take;
  }

  this->starts.assign(this->pages.size() + 1, 0);
  for (std::size_t i = 0; i < this->pages.size(); ++i) {
    this->starts[i + 1] = this->starts[i] + this->pages[i].length(this->file_length);
  }
}

// ---------------------------------------------------------------------------
// lazy line index

void TextBuffer::invalidate_samples_at(std::uint64_t offset) {
  while (not this->samples.empty() and this->samples.back().offset >= offset) {
    this->samples.pop_back();
  }
  if (this->samples.empty()) {
    this->frontier_line = 0;
    this->frontier_offset = 0;
  } else {
    this->frontier_line = this->samples.back().line;
    this->frontier_offset = this->samples.back().offset;
  }
  this->have_tail = false;
}

// Scans the newlines in [frontier_offset, stop) and moves the frontier to the
// start of the last line fully seen; reaches EOF when stop == length().
void TextBuffer::scan(std::uint64_t stop) {
  stop = std::min(stop, this->length());
  if (this->have_tail or stop <= this->frontier_offset) {
    if (not this->have_tail and stop == this->frontier_offset and stop == this->length()) {
      // Empty content: there is exactly one (empty) line; the scan is done.
      this->have_tail = true;
    }
    return;
  }

  auto pos = this->frontier_offset;
  auto line = this->frontier_line;
  auto last_newline_after = this->frontier_offset;
  auto count = std::uint64_t(0);
  while (pos < stop) {
    auto page = std::size_t(page_index_of(pos));
    auto in_page = pos - this->starts[page];
    auto page_len = std::uint64_t(this->pages[page].length(this->file_length));
    auto take = std::min(page_len - in_page, stop - pos);
    auto data = page_bytes(page) + in_page;
    auto end = data + take;
    auto p = data;
    while (p < end) {
      auto nl = static_cast<char const*>(std::memchr(p, '\n', end - p));
      if (not nl) {
        break;
      }
      ++count;
      ++line;
      auto after = pos + (nl - data) + 1;
      if (line % SCAN_SAMPLE_LINES == 0) {
        this->samples.emplace_back(line, after);
      }
      last_newline_after = after;
      p = nl + 1;
    }
    pos += take;
  }

  this->frontier_line = line;
  if (count > 0) {
    this->frontier_offset = last_newline_after;
  }
  if (stop == this->length()) {
    // A file with N newlines has N + 1 lines; the last one is empty when the
    // content ends with a newline (the caret can sit on it).
    this->have_tail = true;
  }
}

std::uint64_t TextBuffer::ensure_line(std::uint64_t line) {
  auto chunk = std::uint64_t(1) << 22; // 4 MiB, doubled each step
  while (known_line_count() <= line and not this->have_tail) {
    auto stop = std::min(this->length(), this->frontier_offset + chunk);
    if (stop == this->frontier_offset) {
      break;
    }
    scan(stop);
    chunk = std::min(chunk * 2, std::uint64_t(1) << 30);
  }
  return known_line_count();
}

std::uint64_t TextBuffer::scan_to_end() {
  if (not this->have_tail) {
    scan(this->length());
  }
  return known_line_count();
}

void TextBuffer::ensure_scanned_to(std::uint64_t offset) {
  if (this->have_tail or offset <= this->frontier_offset) {
    return;
  }
  // Scan the whole gap in one pass so the anchors stored along the way make
  // later offset_to_line / line_start calls near `offset` cheap.
  scan(std::min(offset, this->length()));
}

std::vector<TextBuffer::LineRange> TextBuffer::read_line_ranges(std::uint64_t first_line, std::uint64_t count) const {
  std::vector<LineRange> out;
  if (count == 0) {
    return out;
  }
  auto known = known_line_count();
  if (first_line >= known) {
    return out;
  }
  out.reserve(std::size_t(std::min(count, known - first_line)));

  // One forward pass over the pages: the first line start comes from the
  // anchor index, every later line starts right after the previous one's
  // newline, found with a plain memchr sweep (no per-line anchor rescans).
  auto pos = line_start(first_line);
  auto line = first_line;
  auto total = this->length();
  while (out.size() < count and line < known) {
    auto start = pos;
    auto end = total;
    auto found = false;
    while (pos < total) {
      auto page = std::size_t(page_index_of(pos));
      auto in_page = pos - this->starts[page];
      auto page_len = std::uint64_t(this->pages[page].length(this->file_length));
      auto take = page_len - in_page;
      auto data = page_bytes(page) + in_page;
      auto hit = static_cast<char const*>(std::memchr(data, '\n', std::size_t(take)));
      if (not hit) {
        pos += take;
        continue;
      }
      end = pos + std::uint64_t(hit - data);
      found = true;
      break;
    }
    out.emplace_back(start, end, found);
    if (found) {
      // The content after the newline starts the next line...
      pos = end + 1;
      ++line;
    } else {
      break; // ... and without one the content (and the lines) end here.
    }
  }
  return out;
}

std::uint64_t TextBuffer::line_start(std::uint64_t line) const {
  if (line == 0) {
    return 0;
  }
  auto known = known_line_count();
  if (line > known) {
    return this->length();
  }
  auto sample_line = std::uint64_t(0);
  auto sample_offset = std::uint64_t(0);
  for (auto const &sample : this->samples) {
    if (sample.line >= line) {
      break;
    }
    sample_line = sample.line;
    sample_offset = sample.offset;
  }
  auto remaining = line - sample_line;
  if (remaining == 0) {
    return sample_offset;
  }
  // Skip `remaining` newlines starting at the sample offset.
  auto pos = sample_offset;
  while (remaining > 0 and pos < this->length()) {
    auto page = std::size_t(page_index_of(pos));
    auto in_page = pos - this->starts[page];
    auto page_len = std::uint64_t(this->pages[page].length(this->file_length));
    auto data = page_bytes(page) + in_page;
    auto take = page_len - in_page;
    auto nl = static_cast<char const*>(std::memchr(data, '\n', std::size_t(take)));
    if (not nl) {
      pos += take;
      continue;
    }
    --remaining;
    pos += std::uint64_t(nl - data) + 1;
  }
  return pos;
}

std::pair<std::uint64_t, std::uint64_t> TextBuffer::offset_to_line(std::uint64_t offset) const {
  offset = std::min(offset, this->length());
  auto sample_line = std::uint64_t(0);
  auto sample_offset = std::uint64_t(0);
  for (auto const &sample : this->samples) {
    if (sample.offset > offset) {
      break;
    }
    sample_line = sample.line;
    sample_offset = sample.offset;
  }
  auto line = sample_line;
  auto last_newline_after = sample_offset;
  auto pos = sample_offset;
  while (pos < offset) {
    auto page = std::size_t(page_index_of(pos));
    auto in_page = pos - this->starts[page];
    auto page_len = std::uint64_t(this->pages[page].length(this->file_length));
    auto data = page_bytes(page) + in_page;
    auto take = std::min(page_len - in_page, offset - pos);
    auto p = data;
    auto end = data + take;
    while (p < end) {
      auto nl = static_cast<char const*>(std::memchr(p, '\n', end - p));
      if (not nl) {
        break;
      }
      ++line;
      last_newline_after = pos + (nl - data) + 1;
      p = nl + 1;
    }
    pos += take;
  }
  return { line, offset - std::min(last_newline_after, offset) };
}

// ---------------------------------------------------------------------------
// search

namespace {

// Searches `hay` for `needle` at or after `from`, eight bytes at a time
// (SWAR, the same technique glyph_width in util/utf-8.h uses): the needle's
// first byte is located with the classic per-lane "haszero" test -- a lane
// holding that byte makes (chunk XOR repeat(first)) zero there, and the
// per-lane subtraction underflows only in or behind such lanes, so a nonzero
// mask means the byte occurs in the chunk. Every candidate lane is verified
// with memcmp (mask bits can also sit on benign false-positive lanes behind
// a real one), which keeps the scan exact for any bytes, UTF-8 included:
// searching never decodes, it only compares bytes. The classic worst case (a
// needle whose first byte saturates the haystack) degrades to a memcmp per
// byte; typical log needles scan eight bytes per test.
std::size_t swar_find(std::string_view hay, std::string_view needle, std::size_t from) {
  auto const n = needle.size();
  if (n == 0) {
    return std::min(from, hay.size());
  }
  if (from > hay.size() or n > hay.size() - from) {
    return std::string::npos;
  }

  constexpr auto ones = std::uint64_t { 0x0101'0101'0101'0101 };
  constexpr auto high = std::uint64_t { 0x8080'8080'8080'8080 };
  auto const repeated = ones * std::uint64_t(std::uint8_t(needle[0]));

  auto i = from;
  while (i + 8 <= hay.size()) {
    auto chunk = std::uint64_t { };
    std::memcpy(&chunk, hay.data() + i, sizeof(chunk));
    auto const xored = chunk ^ repeated;
    auto mask = (xored - ones) & ~xored & high;
    while (mask) {
      auto const at = i + std::size_t(std::countr_zero(mask));
      if (at + n <= hay.size() and std::memcmp(hay.data() + at, needle.data(), n) == 0) {
        return at;
      }
      mask &= mask - 1;
    }
    i += 8;
  }
  for (; i + n <= hay.size(); ++i) {
    if (std::uint8_t(hay[i]) == std::uint8_t(needle[0]) and std::memcmp(hay.data() + i, needle.data(), n) == 0) {
      return i;
    }
  }
  return std::string::npos;
}

}

std::optional<std::uint64_t> TextBuffer::find(std::string_view needle, std::uint64_t from) const {
  if (needle.empty()) {
    return std::min(from, this->length());
  }
  // Windowed scan: each window carries `needle.size() - 1` bytes of overlap
  // so a match straddling a window boundary is found by the next window.
  auto const window_size = std::uint64_t(1) << 20; // 1 MiB
  auto pos = std::min(from, this->length());
  while (pos < this->length()) {
    auto take = std::min<std::uint64_t>(window_size + needle.size() - 1, this->length() - pos);
    auto window = read(pos, take);
    auto hit = swar_find(window, needle, 0);
    if (hit != std::string::npos) {
      return pos + std::uint64_t(hit);
    }
    if (take <= window_size) {
      break;
    }
    pos += window_size;
  }
  return std::nullopt;
}

std::vector<std::uint64_t> TextBuffer::find_all(std::string_view needle, std::uint64_t from, std::size_t limit) const {
  if (limit == 0) {
    return {};
  }
  auto pos = std::min(from, this->length());
  if (needle.empty()) {
    return { pos };
  }

  // One forward pass over the same windows find() uses. A window owns the
  // matches that start inside its core (its first window_size bytes); the
  // overlap tail (needle.size() - 1 bytes) is re-read by the next window,
  // which starts exactly at the core's end, so a match that straddles a
  // boundary is reported exactly once, by the window whose core it starts in.
  auto const window_size = std::uint64_t(1) << 20; // 1 MiB
  auto results = std::vector<std::uint64_t> { };
  results.reserve(std::min<std::size_t>(limit, 64));
  while (pos < this->length() and results.size() < limit) {
    auto take = std::min<std::uint64_t>(window_size + needle.size() - 1, this->length() - pos);
    auto window = read(pos, take);
    auto const core = std::min<std::uint64_t>(window_size, take);
    auto cursor = std::size_t { 0 };
    while (results.size() < limit) {
      auto hit = swar_find(window, needle, cursor);
      if (hit == std::string::npos or hit >= core) {
        break; // only the overlap tail is left; the next window owns it
      }
      results.emplace_back(pos + std::uint64_t(hit));
      cursor = hit + needle.size();
    }
    if (take <= window_size) {
      break;
    }
    pos += window_size;
  }
  return results;
}

std::optional<std::pair<std::uint64_t, std::uint64_t>> TextBuffer::find_regex(std::string const &pattern, std::uint64_t from) const {
  std::regex re;
  try {
    re = std::regex(pattern, std::regex::ECMAScript);
  } catch (std::regex_error const &) {
    return std::nullopt;
  }

  // The regex runs over overlapping windows of REGEX_WINDOW bytes; matches
  // longer than the overlap may straddle a boundary and go unnoticed.
  auto const window_size = std::uint64_t(8) << 20; // 8 MiB
  auto const overlap = std::uint64_t(64) << 10;    // 64 KiB
  auto pos = std::min(from, this->length());
  while (pos < this->length()) {
    auto take = std::min<std::uint64_t>(window_size, this->length() - pos);
    auto window = read(pos, take);
    std::smatch match;
    if (std::regex_search(window, match, re)) {
      return std::pair { pos + std::uint64_t(match.position()), pos + std::uint64_t(match.position() + match.length()) };
    }
    if (take < window_size) {
      break;
    }
    pos += window_size - overlap;
  }
  return std::nullopt;
}

std::optional<std::uint64_t> TextBuffer::find_byte(char byte, std::uint64_t from, std::uint64_t limit) const {
  auto const window_size = std::uint64_t(1) << 20; // 1 MiB
  auto pos = std::min(from, this->length());
  limit = std::min(limit, this->length());
  while (pos < limit) {
    auto take = std::min<std::uint64_t>(window_size, limit - pos);
    auto window = read(pos, take);
    auto hit = window.find(byte);
    if (hit != std::string::npos) {
      return pos + std::uint64_t(hit);
    }
    pos += window_size;
  }
  return std::nullopt;
}

}
