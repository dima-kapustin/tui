#pragma once

#include <bitset>
#include <cstdint>

namespace tui::util::unicode {

constexpr char32_t MAX_UNICODE_CHAR = 0x1F'FFFF;
constexpr size_t UNICODE_CHAR_COUNT = MAX_UNICODE_CHAR + 1;

struct CodePointInterval {
  char32_t from, to; // inclusive
};

enum class WordBreak : uint8_t {
  None,
  ALetter,
  CR,
  Double_Quote,
  Extend,
  ExtendNumLet,
  Format,
  Hebrew_Letter,
  Katakana,
  LF,
  MidLetter,
  MidNum,
  MidNumLet,
  Newline,
  Numeric,
  Regional_Indicator,
  Single_Quote,
  WSegSpace,
  ZWJ,
};

constexpr class WordBreakMap {
  WordBreak map[UNICODE_CHAR_COUNT] = { };
public:
  constexpr WordBreakMap(std::initializer_list<std::pair<CodePointInterval, WordBreak>> list) {
    for (auto&& [interval, word_break] : list) {
      for (auto ch = interval.from; ch <= interval.to; ++ch) {
        this->map[ch] = word_break;
      }
    }
  }

  constexpr WordBreak operator[](char32_t cp) const {
    if (cp < std::size(this->map)) {
      return this->map[cp];
    }
    return WordBreak::None;
  }
} word_break_map = //
    // https://www.unicode.org/Public/UCD/latest/ucd/auxiliary/WordBreakProperty.txt
    { { { 0x0000A, 0x0000A }, WordBreak::LF }, //
      { { 0x0000B, 0x0000C }, WordBreak::Newline }, //
      { { 0x0000D, 0x0000D }, WordBreak::CR }, //
      { { 0x00020, 0x00020 }, WordBreak::WSegSpace }, //
      { { 0x00022, 0x00022 }, WordBreak::Double_Quote }, //
      { { 0x00027, 0x00027 }, WordBreak::Single_Quote }, //
      { { 0x0002C, 0x0002C }, WordBreak::MidNum }, //
      { { 0x0002E, 0x0002E }, WordBreak::MidNumLet }, //
      { { 0x00030, 0x00039 }, WordBreak::Numeric }, //
      { { 0x0003A, 0x0003A }, WordBreak::MidLetter }, //
      { { 0x0003B, 0x0003B }, WordBreak::MidNum }, //
      { { 0x00041, 0x0005A }, WordBreak::ALetter }, //
      { { 0x0005F, 0x0005F }, WordBreak::ExtendNumLet }, //
      { { 0x00061, 0x0007A }, WordBreak::ALetter }, //
      { { 0x00085, 0x00085 }, WordBreak::Newline }, //
      { { 0x000AA, 0x000AA }, WordBreak::ALetter }, //
      { { 0x000AD, 0x000AD }, WordBreak::Format }, //
      { { 0x000B5, 0x000B5 }, WordBreak::ALetter }, //
      { { 0x000B7, 0x000B7 }, WordBreak::MidLetter }, //
      { { 0x000BA, 0x000BA }, WordBreak::ALetter }, //
      { { 0x000C0, 0x000D6 }, WordBreak::ALetter }, //
      { { 0x000D8, 0x000F6 }, WordBreak::ALetter }, //
      { { 0x000F8, 0x001BA }, WordBreak::ALetter }, //
      { { 0x001BB, 0x001BB }, WordBreak::ALetter }, //
      { { 0x001BC, 0x001BF }, WordBreak::ALetter }, //
      { { 0x001C0, 0x001C3 }, WordBreak::ALetter }, //
      { { 0x001C4, 0x00293 }, WordBreak::ALetter }, //
      { { 0x00294, 0x00294 }, WordBreak::ALetter }, //
      { { 0x00295, 0x002AF }, WordBreak::ALetter }, //
      { { 0x002B0, 0x002C1 }, WordBreak::ALetter }, //
      { { 0x002C2, 0x002C5 }, WordBreak::ALetter }, //
      { { 0x002C6, 0x002D1 }, WordBreak::ALetter }, //
      { { 0x002D2, 0x002D7 }, WordBreak::ALetter }, //
      { { 0x002DE, 0x002DF }, WordBreak::ALetter }, //
      { { 0x002E0, 0x002E4 }, WordBreak::ALetter }, //
      { { 0x002E5, 0x002EB }, WordBreak::ALetter }, //
      { { 0x002EC, 0x002EC }, WordBreak::ALetter }, //
      { { 0x002ED, 0x002ED }, WordBreak::ALetter }, //
      { { 0x002EE, 0x002EE }, WordBreak::ALetter }, //
      { { 0x002EF, 0x002FF }, WordBreak::ALetter }, //
      { { 0x00300, 0x0036F }, WordBreak::Extend }, //
      { { 0x00370, 0x00373 }, WordBreak::ALetter }, //
      { { 0x00374, 0x00374 }, WordBreak::ALetter }, //
      { { 0x00376, 0x00377 }, WordBreak::ALetter }, //
      { { 0x0037A, 0x0037A }, WordBreak::ALetter }, //
      { { 0x0037B, 0x0037D }, WordBreak::ALetter }, //
      { { 0x0037E, 0x0037E }, WordBreak::MidNum }, //
      { { 0x0037F, 0x0037F }, WordBreak::ALetter }, //
      { { 0x00386, 0x00386 }, WordBreak::ALetter }, //
      { { 0x00387, 0x00387 }, WordBreak::MidLetter }, //
      { { 0x00388, 0x0038A }, WordBreak::ALetter }, //
      { { 0x0038C, 0x0038C }, WordBreak::ALetter }, //
      { { 0x0038E, 0x003A1 }, WordBreak::ALetter }, //
      { { 0x003A3, 0x003F5 }, WordBreak::ALetter }, //
      { { 0x003F7, 0x00481 }, WordBreak::ALetter }, //
      { { 0x00483, 0x00487 }, WordBreak::Extend }, //
      { { 0x00488, 0x00489 }, WordBreak::Extend }, //
      { { 0x0048A, 0x0052F }, WordBreak::ALetter }, //
      { { 0x00531, 0x00556 }, WordBreak::ALetter }, //
      { { 0x00559, 0x00559 }, WordBreak::ALetter }, //
      { { 0x0055A, 0x0055C }, WordBreak::ALetter }, //
      { { 0x0055E, 0x0055E }, WordBreak::ALetter }, //
      { { 0x0055F, 0x0055F }, WordBreak::MidLetter }, //
      { { 0x00560, 0x00588 }, WordBreak::ALetter }, //
      { { 0x00589, 0x00589 }, WordBreak::MidNum }, //
      { { 0x0058A, 0x0058A }, WordBreak::ALetter }, //
      { { 0x00591, 0x005BD }, WordBreak::Extend }, //
      { { 0x005BF, 0x005BF }, WordBreak::Extend }, //
      { { 0x005C1, 0x005C2 }, WordBreak::Extend }, //
      { { 0x005C4, 0x005C5 }, WordBreak::Extend }, //
      { { 0x005C7, 0x005C7 }, WordBreak::Extend }, //
      { { 0x005D0, 0x005EA }, WordBreak::Hebrew_Letter }, //
      { { 0x005EF, 0x005F2 }, WordBreak::Hebrew_Letter }, //
      { { 0x005F3, 0x005F3 }, WordBreak::ALetter }, //
      { { 0x005F4, 0x005F4 }, WordBreak::MidLetter }, //
      { { 0x00600, 0x00605 }, WordBreak::Format }, //
      { { 0x0060C, 0x0060D }, WordBreak::MidNum }, //
      { { 0x00610, 0x0061A }, WordBreak::Extend }, //
      { { 0x0061C, 0x0061C }, WordBreak::Format }, //
      { { 0x00620, 0x0063F }, WordBreak::ALetter }, //
      { { 0x00640, 0x00640 }, WordBreak::ALetter }, //
      { { 0x00641, 0x0064A }, WordBreak::ALetter }, //
      { { 0x0064B, 0x0065F }, WordBreak::Extend }, //
      { { 0x00660, 0x00669 }, WordBreak::Numeric }, //
      { { 0x0066B, 0x0066B }, WordBreak::Numeric }, //
      { { 0x0066C, 0x0066C }, WordBreak::MidNum }, //
      { { 0x0066E, 0x0066F }, WordBreak::ALetter }, //
      { { 0x00670, 0x00670 }, WordBreak::Extend }, //
      { { 0x00671, 0x006D3 }, WordBreak::ALetter }, //
      { { 0x006D5, 0x006D5 }, WordBreak::ALetter }, //
      { { 0x006D6, 0x006DC }, WordBreak::Extend }, //
      { { 0x006DD, 0x006DD }, WordBreak::Format }, //
      { { 0x006DF, 0x006E4 }, WordBreak::Extend }, //
      { { 0x006E5, 0x006E6 }, WordBreak::ALetter }, //
      { { 0x006E7, 0x006E8 }, WordBreak::Extend }, //
      { { 0x006EA, 0x006ED }, WordBreak::Extend }, //
      { { 0x006EE, 0x006EF }, WordBreak::ALetter }, //
      { { 0x006F0, 0x006F9 }, WordBreak::Numeric }, //
      { { 0x006FA, 0x006FC }, WordBreak::ALetter }, //
      { { 0x006FF, 0x006FF }, WordBreak::ALetter }, //
      { { 0x0070F, 0x0070F }, WordBreak::Format }, //
      { { 0x00710, 0x00710 }, WordBreak::ALetter }, //
      { { 0x00711, 0x00711 }, WordBreak::Extend }, //
      { { 0x00712, 0x0072F }, WordBreak::ALetter }, //
      { { 0x00730, 0x0074A }, WordBreak::Extend }, //
      { { 0x0074D, 0x007A5 }, WordBreak::ALetter }, //
      { { 0x007A6, 0x007B0 }, WordBreak::Extend }, //
      { { 0x007B1, 0x007B1 }, WordBreak::ALetter }, //
      { { 0x007C0, 0x007C9 }, WordBreak::Numeric }, //
      { { 0x007CA, 0x007EA }, WordBreak::ALetter }, //
      { { 0x007EB, 0x007F3 }, WordBreak::Extend }, //
      { { 0x007F4, 0x007F5 }, WordBreak::ALetter }, //
      { { 0x007F8, 0x007F8 }, WordBreak::MidNum }, //
      { { 0x007FA, 0x007FA }, WordBreak::ALetter }, //
      { { 0x007FD, 0x007FD }, WordBreak::Extend }, //
      { { 0x00800, 0x00815 }, WordBreak::ALetter }, //
      { { 0x00816, 0x00819 }, WordBreak::Extend }, //
      { { 0x0081A, 0x0081A }, WordBreak::ALetter }, //
      { { 0x0081B, 0x00823 }, WordBreak::Extend }, //
      { { 0x00824, 0x00824 }, WordBreak::ALetter }, //
      { { 0x00825, 0x00827 }, WordBreak::Extend }, //
      { { 0x00828, 0x00828 }, WordBreak::ALetter }, //
      { { 0x00829, 0x0082D }, WordBreak::Extend }, //
      { { 0x00840, 0x00858 }, WordBreak::ALetter }, //
      { { 0x00859, 0x0085B }, WordBreak::Extend }, //
      { { 0x00860, 0x0086A }, WordBreak::ALetter }, //
      { { 0x008A0, 0x008B4 }, WordBreak::ALetter }, //
      { { 0x008B6, 0x008C7 }, WordBreak::ALetter }, //
      { { 0x008D3, 0x008E1 }, WordBreak::Extend }, //
      { { 0x008E2, 0x008E2 }, WordBreak::Format }, //
      { { 0x008E3, 0x00902 }, WordBreak::Extend }, //
      { { 0x00903, 0x00903 }, WordBreak::Extend }, //
      { { 0x00904, 0x00939 }, WordBreak::ALetter }, //
      { { 0x0093A, 0x0093A }, WordBreak::Extend }, //
      { { 0x0093B, 0x0093B }, WordBreak::Extend }, //
      { { 0x0093C, 0x0093C }, WordBreak::Extend }, //
      { { 0x0093D, 0x0093D }, WordBreak::ALetter }, //
      { { 0x0093E, 0x00940 }, WordBreak::Extend }, //
      { { 0x00941, 0x00948 }, WordBreak::Extend }, //
      { { 0x00949, 0x0094C }, WordBreak::Extend }, //
      { { 0x0094D, 0x0094D }, WordBreak::Extend }, //
      { { 0x0094E, 0x0094F }, WordBreak::Extend }, //
      { { 0x00950, 0x00950 }, WordBreak::ALetter }, //
      { { 0x00951, 0x00957 }, WordBreak::Extend }, //
      { { 0x00958, 0x00961 }, WordBreak::ALetter }, //
      { { 0x00962, 0x00963 }, WordBreak::Extend }, //
      { { 0x00966, 0x0096F }, WordBreak::Numeric }, //
      { { 0x00971, 0x00971 }, WordBreak::ALetter }, //
      { { 0x00972, 0x00980 }, WordBreak::ALetter }, //
      { { 0x00981, 0x00981 }, WordBreak::Extend }, //
      { { 0x00982, 0x00983 }, WordBreak::Extend }, //
      { { 0x00985, 0x0098C }, WordBreak::ALetter }, //
      { { 0x0098F, 0x00990 }, WordBreak::ALetter }, //
      { { 0x00993, 0x009A8 }, WordBreak::ALetter }, //
      { { 0x009AA, 0x009B0 }, WordBreak::ALetter }, //
      { { 0x009B2, 0x009B2 }, WordBreak::ALetter }, //
      { { 0x009B6, 0x009B9 }, WordBreak::ALetter }, //
      { { 0x009BC, 0x009BC }, WordBreak::Extend }, //
      { { 0x009BD, 0x009BD }, WordBreak::ALetter }, //
      { { 0x009BE, 0x009C0 }, WordBreak::Extend }, //
      { { 0x009C1, 0x009C4 }, WordBreak::Extend }, //
      { { 0x009C7, 0x009C8 }, WordBreak::Extend }, //
      { { 0x009CB, 0x009CC }, WordBreak::Extend }, //
      { { 0x009CD, 0x009CD }, WordBreak::Extend }, //
      { { 0x009CE, 0x009CE }, WordBreak::ALetter }, //
      { { 0x009D7, 0x009D7 }, WordBreak::Extend }, //
      { { 0x009DC, 0x009DD }, WordBreak::ALetter }, //
      { { 0x009DF, 0x009E1 }, WordBreak::ALetter }, //
      { { 0x009E2, 0x009E3 }, WordBreak::Extend }, //
      { { 0x009E6, 0x009EF }, WordBreak::Numeric }, //
      { { 0x009F0, 0x009F1 }, WordBreak::ALetter }, //
      { { 0x009FC, 0x009FC }, WordBreak::ALetter }, //
      { { 0x009FE, 0x009FE }, WordBreak::Extend }, //
      { { 0x00A01, 0x00A02 }, WordBreak::Extend }, //
      { { 0x00A03, 0x00A03 }, WordBreak::Extend }, //
      { { 0x00A05, 0x00A0A }, WordBreak::ALetter }, //
      { { 0x00A0F, 0x00A10 }, WordBreak::ALetter }, //
      { { 0x00A13, 0x00A28 }, WordBreak::ALetter }, //
      { { 0x00A2A, 0x00A30 }, WordBreak::ALetter }, //
      { { 0x00A32, 0x00A33 }, WordBreak::ALetter }, //
      { { 0x00A35, 0x00A36 }, WordBreak::ALetter }, //
      { { 0x00A38, 0x00A39 }, WordBreak::ALetter }, //
      { { 0x00A3C, 0x00A3C }, WordBreak::Extend }, //
      { { 0x00A3E, 0x00A40 }, WordBreak::Extend }, //
      { { 0x00A41, 0x00A42 }, WordBreak::Extend }, //
      { { 0x00A47, 0x00A48 }, WordBreak::Extend }, //
      { { 0x00A4B, 0x00A4D }, WordBreak::Extend }, //
      { { 0x00A51, 0x00A51 }, WordBreak::Extend }, //
      { { 0x00A59, 0x00A5C }, WordBreak::ALetter }, //
      { { 0x00A5E, 0x00A5E }, WordBreak::ALetter }, //
      { { 0x00A66, 0x00A6F }, WordBreak::Numeric }, //
      { { 0x00A70, 0x00A71 }, WordBreak::Extend }, //
      { { 0x00A72, 0x00A74 }, WordBreak::ALetter }, //
      { { 0x00A75, 0x00A75 }, WordBreak::Extend }, //
      { { 0x00A81, 0x00A82 }, WordBreak::Extend }, //
      { { 0x00A83, 0x00A83 }, WordBreak::Extend }, //
      { { 0x00A85, 0x00A8D }, WordBreak::ALetter }, //
      { { 0x00A8F, 0x00A91 }, WordBreak::ALetter }, //
      { { 0x00A93, 0x00AA8 }, WordBreak::ALetter }, //
      { { 0x00AAA, 0x00AB0 }, WordBreak::ALetter }, //
      { { 0x00AB2, 0x00AB3 }, WordBreak::ALetter }, //
      { { 0x00AB5, 0x00AB9 }, WordBreak::ALetter }, //
      { { 0x00ABC, 0x00ABC }, WordBreak::Extend }, //
      { { 0x00ABD, 0x00ABD }, WordBreak::ALetter }, //
      { { 0x00ABE, 0x00AC0 }, WordBreak::Extend }, //
      { { 0x00AC1, 0x00AC5 }, WordBreak::Extend }, //
      { { 0x00AC7, 0x00AC8 }, WordBreak::Extend }, //
      { { 0x00AC9, 0x00AC9 }, WordBreak::Extend }, //
      { { 0x00ACB, 0x00ACC }, WordBreak::Extend }, //
      { { 0x00ACD, 0x00ACD }, WordBreak::Extend }, //
      { { 0x00AD0, 0x00AD0 }, WordBreak::ALetter }, //
      { { 0x00AE0, 0x00AE1 }, WordBreak::ALetter }, //
      { { 0x00AE2, 0x00AE3 }, WordBreak::Extend }, //
      { { 0x00AE6, 0x00AEF }, WordBreak::Numeric }, //
      { { 0x00AF9, 0x00AF9 }, WordBreak::ALetter }, //
      { { 0x00AFA, 0x00AFF }, WordBreak::Extend }, //
      { { 0x00B01, 0x00B01 }, WordBreak::Extend }, //
      { { 0x00B02, 0x00B03 }, WordBreak::Extend }, //
      { { 0x00B05, 0x00B0C }, WordBreak::ALetter }, //
      { { 0x00B0F, 0x00B10 }, WordBreak::ALetter }, //
      { { 0x00B13, 0x00B28 }, WordBreak::ALetter }, //
      { { 0x00B2A, 0x00B30 }, WordBreak::ALetter }, //
      { { 0x00B32, 0x00B33 }, WordBreak::ALetter }, //
      { { 0x00B35, 0x00B39 }, WordBreak::ALetter }, //
      { { 0x00B3C, 0x00B3C }, WordBreak::Extend }, //
      { { 0x00B3D, 0x00B3D }, WordBreak::ALetter }, //
      { { 0x00B3E, 0x00B3E }, WordBreak::Extend }, //
      { { 0x00B3F, 0x00B3F }, WordBreak::Extend }, //
      { { 0x00B40, 0x00B40 }, WordBreak::Extend }, //
      { { 0x00B41, 0x00B44 }, WordBreak::Extend }, //
      { { 0x00B47, 0x00B48 }, WordBreak::Extend }, //
      { { 0x00B4B, 0x00B4C }, WordBreak::Extend }, //
      { { 0x00B4D, 0x00B4D }, WordBreak::Extend }, //
      { { 0x00B55, 0x00B56 }, WordBreak::Extend }, //
      { { 0x00B57, 0x00B57 }, WordBreak::Extend }, //
      { { 0x00B5C, 0x00B5D }, WordBreak::ALetter }, //
      { { 0x00B5F, 0x00B61 }, WordBreak::ALetter }, //
      { { 0x00B62, 0x00B63 }, WordBreak::Extend }, //
      { { 0x00B66, 0x00B6F }, WordBreak::Numeric }, //
      { { 0x00B71, 0x00B71 }, WordBreak::ALetter }, //
      { { 0x00B82, 0x00B82 }, WordBreak::Extend }, //
      { { 0x00B83, 0x00B83 }, WordBreak::ALetter }, //
      { { 0x00B85, 0x00B8A }, WordBreak::ALetter }, //
      { { 0x00B8E, 0x00B90 }, WordBreak::ALetter }, //
      { { 0x00B92, 0x00B95 }, WordBreak::ALetter }, //
      { { 0x00B99, 0x00B9A }, WordBreak::ALetter }, //
      { { 0x00B9C, 0x00B9C }, WordBreak::ALetter }, //
      { { 0x00B9E, 0x00B9F }, WordBreak::ALetter }, //
      { { 0x00BA3, 0x00BA4 }, WordBreak::ALetter }, //
      { { 0x00BA8, 0x00BAA }, WordBreak::ALetter }, //
      { { 0x00BAE, 0x00BB9 }, WordBreak::ALetter }, //
      { { 0x00BBE, 0x00BBF }, WordBreak::Extend }, //
      { { 0x00BC0, 0x00BC0 }, WordBreak::Extend }, //
      { { 0x00BC1, 0x00BC2 }, WordBreak::Extend }, //
      { { 0x00BC6, 0x00BC8 }, WordBreak::Extend }, //
      { { 0x00BCA, 0x00BCC }, WordBreak::Extend }, //
      { { 0x00BCD, 0x00BCD }, WordBreak::Extend }, //
      { { 0x00BD0, 0x00BD0 }, WordBreak::ALetter }, //
      { { 0x00BD7, 0x00BD7 }, WordBreak::Extend }, //
      { { 0x00BE6, 0x00BEF }, WordBreak::Numeric }, //
      { { 0x00C00, 0x00C00 }, WordBreak::Extend }, //
      { { 0x00C01, 0x00C03 }, WordBreak::Extend }, //
      { { 0x00C04, 0x00C04 }, WordBreak::Extend }, //
      { { 0x00C05, 0x00C0C }, WordBreak::ALetter }, //
      { { 0x00C0E, 0x00C10 }, WordBreak::ALetter }, //
      { { 0x00C12, 0x00C28 }, WordBreak::ALetter }, //
      { { 0x00C2A, 0x00C39 }, WordBreak::ALetter }, //
      { { 0x00C3D, 0x00C3D }, WordBreak::ALetter }, //
      { { 0x00C3E, 0x00C40 }, WordBreak::Extend }, //
      { { 0x00C41, 0x00C44 }, WordBreak::Extend }, //
      { { 0x00C46, 0x00C48 }, WordBreak::Extend }, //
      { { 0x00C4A, 0x00C4D }, WordBreak::Extend }, //
      { { 0x00C55, 0x00C56 }, WordBreak::Extend }, //
      { { 0x00C58, 0x00C5A }, WordBreak::ALetter }, //
      { { 0x00C60, 0x00C61 }, WordBreak::ALetter }, //
      { { 0x00C62, 0x00C63 }, WordBreak::Extend }, //
      { { 0x00C66, 0x00C6F }, WordBreak::Numeric }, //
      { { 0x00C80, 0x00C80 }, WordBreak::ALetter }, //
      { { 0x00C81, 0x00C81 }, WordBreak::Extend }, //
      { { 0x00C82, 0x00C83 }, WordBreak::Extend }, //
      { { 0x00C85, 0x00C8C }, WordBreak::ALetter }, //
      { { 0x00C8E, 0x00C90 }, WordBreak::ALetter }, //
      { { 0x00C92, 0x00CA8 }, WordBreak::ALetter }, //
      { { 0x00CAA, 0x00CB3 }, WordBreak::ALetter }, //
      { { 0x00CB5, 0x00CB9 }, WordBreak::ALetter }, //
      { { 0x00CBC, 0x00CBC }, WordBreak::Extend }, //
      { { 0x00CBD, 0x00CBD }, WordBreak::ALetter }, //
      { { 0x00CBE, 0x00CBE }, WordBreak::Extend }, //
      { { 0x00CBF, 0x00CBF }, WordBreak::Extend }, //
      { { 0x00CC0, 0x00CC4 }, WordBreak::Extend }, //
      { { 0x00CC6, 0x00CC6 }, WordBreak::Extend }, //
      { { 0x00CC7, 0x00CC8 }, WordBreak::Extend }, //
      { { 0x00CCA, 0x00CCB }, WordBreak::Extend }, //
      { { 0x00CCC, 0x00CCD }, WordBreak::Extend }, //
      { { 0x00CD5, 0x00CD6 }, WordBreak::Extend }, //
      { { 0x00CDE, 0x00CDE }, WordBreak::ALetter }, //
      { { 0x00CE0, 0x00CE1 }, WordBreak::ALetter }, //
      { { 0x00CE2, 0x00CE3 }, WordBreak::Extend }, //
      { { 0x00CE6, 0x00CEF }, WordBreak::Numeric }, //
      { { 0x00CF1, 0x00CF2 }, WordBreak::ALetter }, //
      { { 0x00D00, 0x00D01 }, WordBreak::Extend }, //
      { { 0x00D02, 0x00D03 }, WordBreak::Extend }, //
      { { 0x00D04, 0x00D0C }, WordBreak::ALetter }, //
      { { 0x00D0E, 0x00D10 }, WordBreak::ALetter }, //
      { { 0x00D12, 0x00D3A }, WordBreak::ALetter }, //
      { { 0x00D3B, 0x00D3C }, WordBreak::Extend }, //
      { { 0x00D3D, 0x00D3D }, WordBreak::ALetter }, //
      { { 0x00D3E, 0x00D40 }, WordBreak::Extend }, //
      { { 0x00D41, 0x00D44 }, WordBreak::Extend }, //
      { { 0x00D46, 0x00D48 }, WordBreak::Extend }, //
      { { 0x00D4A, 0x00D4C }, WordBreak::Extend }, //
      { { 0x00D4D, 0x00D4D }, WordBreak::Extend }, //
      { { 0x00D4E, 0x00D4E }, WordBreak::ALetter }, //
      { { 0x00D54, 0x00D56 }, WordBreak::ALetter }, //
      { { 0x00D57, 0x00D57 }, WordBreak::Extend }, //
      { { 0x00D5F, 0x00D61 }, WordBreak::ALetter }, //
      { { 0x00D62, 0x00D63 }, WordBreak::Extend }, //
      { { 0x00D66, 0x00D6F }, WordBreak::Numeric }, //
      { { 0x00D7A, 0x00D7F }, WordBreak::ALetter }, //
      { { 0x00D81, 0x00D81 }, WordBreak::Extend }, //
      { { 0x00D82, 0x00D83 }, WordBreak::Extend }, //
      { { 0x00D85, 0x00D96 }, WordBreak::ALetter }, //
      { { 0x00D9A, 0x00DB1 }, WordBreak::ALetter }, //
      { { 0x00DB3, 0x00DBB }, WordBreak::ALetter }, //
      { { 0x00DBD, 0x00DBD }, WordBreak::ALetter }, //
      { { 0x00DC0, 0x00DC6 }, WordBreak::ALetter }, //
      { { 0x00DCA, 0x00DCA }, WordBreak::Extend }, //
      { { 0x00DCF, 0x00DD1 }, WordBreak::Extend }, //
      { { 0x00DD2, 0x00DD4 }, WordBreak::Extend }, //
      { { 0x00DD6, 0x00DD6 }, WordBreak::Extend }, //
      { { 0x00DD8, 0x00DDF }, WordBreak::Extend }, //
      { { 0x00DE6, 0x00DEF }, WordBreak::Numeric }, //
      { { 0x00DF2, 0x00DF3 }, WordBreak::Extend }, //
      { { 0x00E31, 0x00E31 }, WordBreak::Extend }, //
      { { 0x00E34, 0x00E3A }, WordBreak::Extend }, //
      { { 0x00E47, 0x00E4E }, WordBreak::Extend }, //
      { { 0x00E50, 0x00E59 }, WordBreak::Numeric }, //
      { { 0x00EB1, 0x00EB1 }, WordBreak::Extend }, //
      { { 0x00EB4, 0x00EBC }, WordBreak::Extend }, //
      { { 0x00EC8, 0x00ECD }, WordBreak::Extend }, //
      { { 0x00ED0, 0x00ED9 }, WordBreak::Numeric }, //
      { { 0x00F00, 0x00F00 }, WordBreak::ALetter }, //
      { { 0x00F18, 0x00F19 }, WordBreak::Extend }, //
      { { 0x00F20, 0x00F29 }, WordBreak::Numeric }, //
      { { 0x00F35, 0x00F35 }, WordBreak::Extend }, //
      { { 0x00F37, 0x00F37 }, WordBreak::Extend }, //
      { { 0x00F39, 0x00F39 }, WordBreak::Extend }, //
      { { 0x00F3E, 0x00F3F }, WordBreak::Extend }, //
      { { 0x00F40, 0x00F47 }, WordBreak::ALetter }, //
      { { 0x00F49, 0x00F6C }, WordBreak::ALetter }, //
      { { 0x00F71, 0x00F7E }, WordBreak::Extend }, //
      { { 0x00F7F, 0x00F7F }, WordBreak::Extend }, //
      { { 0x00F80, 0x00F84 }, WordBreak::Extend }, //
      { { 0x00F86, 0x00F87 }, WordBreak::Extend }, //
      { { 0x00F88, 0x00F8C }, WordBreak::ALetter }, //
      { { 0x00F8D, 0x00F97 }, WordBreak::Extend }, //
      { { 0x00F99, 0x00FBC }, WordBreak::Extend }, //
      { { 0x00FC6, 0x00FC6 }, WordBreak::Extend }, //
      { { 0x0102B, 0x0102C }, WordBreak::Extend }, //
      { { 0x0102D, 0x01030 }, WordBreak::Extend }, //
      { { 0x01031, 0x01031 }, WordBreak::Extend }, //
      { { 0x01032, 0x01037 }, WordBreak::Extend }, //
      { { 0x01038, 0x01038 }, WordBreak::Extend }, //
      { { 0x01039, 0x0103A }, WordBreak::Extend }, //
      { { 0x0103B, 0x0103C }, WordBreak::Extend }, //
      { { 0x0103D, 0x0103E }, WordBreak::Extend }, //
      { { 0x01040, 0x01049 }, WordBreak::Numeric }, //
      { { 0x01056, 0x01057 }, WordBreak::Extend }, //
      { { 0x01058, 0x01059 }, WordBreak::Extend }, //
      { { 0x0105E, 0x01060 }, WordBreak::Extend }, //
      { { 0x01062, 0x01064 }, WordBreak::Extend }, //
      { { 0x01067, 0x0106D }, WordBreak::Extend }, //
      { { 0x01071, 0x01074 }, WordBreak::Extend }, //
      { { 0x01082, 0x01082 }, WordBreak::Extend }, //
      { { 0x01083, 0x01084 }, WordBreak::Extend }, //
      { { 0x01085, 0x01086 }, WordBreak::Extend }, //
      { { 0x01087, 0x0108C }, WordBreak::Extend }, //
      { { 0x0108D, 0x0108D }, WordBreak::Extend }, //
      { { 0x0108F, 0x0108F }, WordBreak::Extend }, //
      { { 0x01090, 0x01099 }, WordBreak::Numeric }, //
      { { 0x0109A, 0x0109C }, WordBreak::Extend }, //
      { { 0x0109D, 0x0109D }, WordBreak::Extend }, //
      { { 0x010A0, 0x010C5 }, WordBreak::ALetter }, //
      { { 0x010C7, 0x010C7 }, WordBreak::ALetter }, //
      { { 0x010CD, 0x010CD }, WordBreak::ALetter }, //
      { { 0x010D0, 0x010FA }, WordBreak::ALetter }, //
      { { 0x010FC, 0x010FC }, WordBreak::ALetter }, //
      { { 0x010FD, 0x010FF }, WordBreak::ALetter }, //
      { { 0x01100, 0x01248 }, WordBreak::ALetter }, //
      { { 0x0124A, 0x0124D }, WordBreak::ALetter }, //
      { { 0x01250, 0x01256 }, WordBreak::ALetter }, //
      { { 0x01258, 0x01258 }, WordBreak::ALetter }, //
      { { 0x0125A, 0x0125D }, WordBreak::ALetter }, //
      { { 0x01260, 0x01288 }, WordBreak::ALetter }, //
      { { 0x0128A, 0x0128D }, WordBreak::ALetter }, //
      { { 0x01290, 0x012B0 }, WordBreak::ALetter }, //
      { { 0x012B2, 0x012B5 }, WordBreak::ALetter }, //
      { { 0x012B8, 0x012BE }, WordBreak::ALetter }, //
      { { 0x012C0, 0x012C0 }, WordBreak::ALetter }, //
      { { 0x012C2, 0x012C5 }, WordBreak::ALetter }, //
      { { 0x012C8, 0x012D6 }, WordBreak::ALetter }, //
      { { 0x012D8, 0x01310 }, WordBreak::ALetter }, //
      { { 0x01312, 0x01315 }, WordBreak::ALetter }, //
      { { 0x01318, 0x0135A }, WordBreak::ALetter }, //
      { { 0x0135D, 0x0135F }, WordBreak::Extend }, //
      { { 0x01380, 0x0138F }, WordBreak::ALetter }, //
      { { 0x013A0, 0x013F5 }, WordBreak::ALetter }, //
      { { 0x013F8, 0x013FD }, WordBreak::ALetter }, //
      { { 0x01401, 0x0166C }, WordBreak::ALetter }, //
      { { 0x0166F, 0x0167F }, WordBreak::ALetter }, //
      { { 0x01680, 0x01680 }, WordBreak::WSegSpace }, //
      { { 0x01681, 0x0169A }, WordBreak::ALetter }, //
      { { 0x016A0, 0x016EA }, WordBreak::ALetter }, //
      { { 0x016EE, 0x016F0 }, WordBreak::ALetter }, //
      { { 0x016F1, 0x016F8 }, WordBreak::ALetter }, //
      { { 0x01700, 0x0170C }, WordBreak::ALetter }, //
      { { 0x0170E, 0x01711 }, WordBreak::ALetter }, //
      { { 0x01712, 0x01714 }, WordBreak::Extend }, //
      { { 0x01720, 0x01731 }, WordBreak::ALetter }, //
      { { 0x01732, 0x01734 }, WordBreak::Extend }, //
      { { 0x01740, 0x01751 }, WordBreak::ALetter }, //
      { { 0x01752, 0x01753 }, WordBreak::Extend }, //
      { { 0x01760, 0x0176C }, WordBreak::ALetter }, //
      { { 0x0176E, 0x01770 }, WordBreak::ALetter }, //
      { { 0x01772, 0x01773 }, WordBreak::Extend }, //
      { { 0x017B4, 0x017B5 }, WordBreak::Extend }, //
      { { 0x017B6, 0x017B6 }, WordBreak::Extend }, //
      { { 0x017B7, 0x017BD }, WordBreak::Extend }, //
      { { 0x017BE, 0x017C5 }, WordBreak::Extend }, //
      { { 0x017C6, 0x017C6 }, WordBreak::Extend }, //
      { { 0x017C7, 0x017C8 }, WordBreak::Extend }, //
      { { 0x017C9, 0x017D3 }, WordBreak::Extend }, //
      { { 0x017DD, 0x017DD }, WordBreak::Extend }, //
      { { 0x017E0, 0x017E9 }, WordBreak::Numeric }, //
      { { 0x0180B, 0x0180D }, WordBreak::Extend }, //
      { { 0x0180E, 0x0180E }, WordBreak::Format }, //
      { { 0x01810, 0x01819 }, WordBreak::Numeric }, //
      { { 0x01820, 0x01842 }, WordBreak::ALetter }, //
      { { 0x01843, 0x01843 }, WordBreak::ALetter }, //
      { { 0x01844, 0x01878 }, WordBreak::ALetter }, //
      { { 0x01880, 0x01884 }, WordBreak::ALetter }, //
      { { 0x01885, 0x01886 }, WordBreak::Extend }, //
      { { 0x01887, 0x018A8 }, WordBreak::ALetter }, //
      { { 0x018A9, 0x018A9 }, WordBreak::Extend }, //
      { { 0x018AA, 0x018AA }, WordBreak::ALetter }, //
      { { 0x018B0, 0x018F5 }, WordBreak::ALetter }, //
      { { 0x01900, 0x0191E }, WordBreak::ALetter }, //
      { { 0x01920, 0x01922 }, WordBreak::Extend }, //
      { { 0x01923, 0x01926 }, WordBreak::Extend }, //
      { { 0x01927, 0x01928 }, WordBreak::Extend }, //
      { { 0x01929, 0x0192B }, WordBreak::Extend }, //
      { { 0x01930, 0x01931 }, WordBreak::Extend }, //
      { { 0x01932, 0x01932 }, WordBreak::Extend }, //
      { { 0x01933, 0x01938 }, WordBreak::Extend }, //
      { { 0x01939, 0x0193B }, WordBreak::Extend }, //
      { { 0x01946, 0x0194F }, WordBreak::Numeric }, //
      { { 0x019D0, 0x019D9 }, WordBreak::Numeric }, //
      { { 0x01A00, 0x01A16 }, WordBreak::ALetter }, //
      { { 0x01A17, 0x01A18 }, WordBreak::Extend }, //
      { { 0x01A19, 0x01A1A }, WordBreak::Extend }, //
      { { 0x01A1B, 0x01A1B }, WordBreak::Extend }, //
      { { 0x01A55, 0x01A55 }, WordBreak::Extend }, //
      { { 0x01A56, 0x01A56 }, WordBreak::Extend }, //
      { { 0x01A57, 0x01A57 }, WordBreak::Extend }, //
      { { 0x01A58, 0x01A5E }, WordBreak::Extend }, //
      { { 0x01A60, 0x01A60 }, WordBreak::Extend }, //
      { { 0x01A61, 0x01A61 }, WordBreak::Extend }, //
      { { 0x01A62, 0x01A62 }, WordBreak::Extend }, //
      { { 0x01A63, 0x01A64 }, WordBreak::Extend }, //
      { { 0x01A65, 0x01A6C }, WordBreak::Extend }, //
      { { 0x01A6D, 0x01A72 }, WordBreak::Extend }, //
      { { 0x01A73, 0x01A7C }, WordBreak::Extend }, //
      { { 0x01A7F, 0x01A7F }, WordBreak::Extend }, //
      { { 0x01A80, 0x01A89 }, WordBreak::Numeric }, //
      { { 0x01A90, 0x01A99 }, WordBreak::Numeric }, //
      { { 0x01AB0, 0x01ABD }, WordBreak::Extend }, //
      { { 0x01ABE, 0x01ABE }, WordBreak::Extend }, //
      { { 0x01ABF, 0x01AC0 }, WordBreak::Extend }, //
      { { 0x01B00, 0x01B03 }, WordBreak::Extend }, //
      { { 0x01B04, 0x01B04 }, WordBreak::Extend }, //
      { { 0x01B05, 0x01B33 }, WordBreak::ALetter }, //
      { { 0x01B34, 0x01B34 }, WordBreak::Extend }, //
      { { 0x01B35, 0x01B35 }, WordBreak::Extend }, //
      { { 0x01B36, 0x01B3A }, WordBreak::Extend }, //
      { { 0x01B3B, 0x01B3B }, WordBreak::Extend }, //
      { { 0x01B3C, 0x01B3C }, WordBreak::Extend }, //
      { { 0x01B3D, 0x01B41 }, WordBreak::Extend }, //
      { { 0x01B42, 0x01B42 }, WordBreak::Extend }, //
      { { 0x01B43, 0x01B44 }, WordBreak::Extend }, //
      { { 0x01B45, 0x01B4B }, WordBreak::ALetter }, //
      { { 0x01B50, 0x01B59 }, WordBreak::Numeric }, //
      { { 0x01B6B, 0x01B73 }, WordBreak::Extend }, //
      { { 0x01B80, 0x01B81 }, WordBreak::Extend }, //
      { { 0x01B82, 0x01B82 }, WordBreak::Extend }, //
      { { 0x01B83, 0x01BA0 }, WordBreak::ALetter }, //
      { { 0x01BA1, 0x01BA1 }, WordBreak::Extend }, //
      { { 0x01BA2, 0x01BA5 }, WordBreak::Extend }, //
      { { 0x01BA6, 0x01BA7 }, WordBreak::Extend }, //
      { { 0x01BA8, 0x01BA9 }, WordBreak::Extend }, //
      { { 0x01BAA, 0x01BAA }, WordBreak::Extend }, //
      { { 0x01BAB, 0x01BAD }, WordBreak::Extend }, //
      { { 0x01BAE, 0x01BAF }, WordBreak::ALetter }, //
      { { 0x01BB0, 0x01BB9 }, WordBreak::Numeric }, //
      { { 0x01BBA, 0x01BE5 }, WordBreak::ALetter }, //
      { { 0x01BE6, 0x01BE6 }, WordBreak::Extend }, //
      { { 0x01BE7, 0x01BE7 }, WordBreak::Extend }, //
      { { 0x01BE8, 0x01BE9 }, WordBreak::Extend }, //
      { { 0x01BEA, 0x01BEC }, WordBreak::Extend }, //
      { { 0x01BED, 0x01BED }, WordBreak::Extend }, //
      { { 0x01BEE, 0x01BEE }, WordBreak::Extend }, //
      { { 0x01BEF, 0x01BF1 }, WordBreak::Extend }, //
      { { 0x01BF2, 0x01BF3 }, WordBreak::Extend }, //
      { { 0x01C00, 0x01C23 }, WordBreak::ALetter }, //
      { { 0x01C24, 0x01C2B }, WordBreak::Extend }, //
      { { 0x01C2C, 0x01C33 }, WordBreak::Extend }, //
      { { 0x01C34, 0x01C35 }, WordBreak::Extend }, //
      { { 0x01C36, 0x01C37 }, WordBreak::Extend }, //
      { { 0x01C40, 0x01C49 }, WordBreak::Numeric }, //
      { { 0x01C4D, 0x01C4F }, WordBreak::ALetter }, //
      { { 0x01C50, 0x01C59 }, WordBreak::Numeric }, //
      { { 0x01C5A, 0x01C77 }, WordBreak::ALetter }, //
      { { 0x01C78, 0x01C7D }, WordBreak::ALetter }, //
      { { 0x01C80, 0x01C88 }, WordBreak::ALetter }, //
      { { 0x01C90, 0x01CBA }, WordBreak::ALetter }, //
      { { 0x01CBD, 0x01CBF }, WordBreak::ALetter }, //
      { { 0x01CD0, 0x01CD2 }, WordBreak::Extend }, //
      { { 0x01CD4, 0x01CE0 }, WordBreak::Extend }, //
      { { 0x01CE1, 0x01CE1 }, WordBreak::Extend }, //
      { { 0x01CE2, 0x01CE8 }, WordBreak::Extend }, //
      { { 0x01CE9, 0x01CEC }, WordBreak::ALetter }, //
      { { 0x01CED, 0x01CED }, WordBreak::Extend }, //
      { { 0x01CEE, 0x01CF3 }, WordBreak::ALetter }, //
      { { 0x01CF4, 0x01CF4 }, WordBreak::Extend }, //
      { { 0x01CF5, 0x01CF6 }, WordBreak::ALetter }, //
      { { 0x01CF7, 0x01CF7 }, WordBreak::Extend }, //
      { { 0x01CF8, 0x01CF9 }, WordBreak::Extend }, //
      { { 0x01CFA, 0x01CFA }, WordBreak::ALetter }, //
      { { 0x01D00, 0x01D2B }, WordBreak::ALetter }, //
      { { 0x01D2C, 0x01D6A }, WordBreak::ALetter }, //
      { { 0x01D6B, 0x01D77 }, WordBreak::ALetter }, //
      { { 0x01D78, 0x01D78 }, WordBreak::ALetter }, //
      { { 0x01D79, 0x01D9A }, WordBreak::ALetter }, //
      { { 0x01D9B, 0x01DBF }, WordBreak::ALetter }, //
      { { 0x01DC0, 0x01DF9 }, WordBreak::Extend }, //
      { { 0x01DFB, 0x01DFF }, WordBreak::Extend }, //
      { { 0x01E00, 0x01F15 }, WordBreak::ALetter }, //
      { { 0x01F18, 0x01F1D }, WordBreak::ALetter }, //
      { { 0x01F20, 0x01F45 }, WordBreak::ALetter }, //
      { { 0x01F48, 0x01F4D }, WordBreak::ALetter }, //
      { { 0x01F50, 0x01F57 }, WordBreak::ALetter }, //
      { { 0x01F59, 0x01F59 }, WordBreak::ALetter }, //
      { { 0x01F5B, 0x01F5B }, WordBreak::ALetter }, //
      { { 0x01F5D, 0x01F5D }, WordBreak::ALetter }, //
      { { 0x01F5F, 0x01F7D }, WordBreak::ALetter }, //
      { { 0x01F80, 0x01FB4 }, WordBreak::ALetter }, //
      { { 0x01FB6, 0x01FBC }, WordBreak::ALetter }, //
      { { 0x01FBE, 0x01FBE }, WordBreak::ALetter }, //
      { { 0x01FC2, 0x01FC4 }, WordBreak::ALetter }, //
      { { 0x01FC6, 0x01FCC }, WordBreak::ALetter }, //
      { { 0x01FD0, 0x01FD3 }, WordBreak::ALetter }, //
      { { 0x01FD6, 0x01FDB }, WordBreak::ALetter }, //
      { { 0x01FE0, 0x01FEC }, WordBreak::ALetter }, //
      { { 0x01FF2, 0x01FF4 }, WordBreak::ALetter }, //
      { { 0x01FF6, 0x01FFC }, WordBreak::ALetter }, //
      { { 0x02000, 0x02006 }, WordBreak::WSegSpace }, //
      { { 0x02008, 0x0200A }, WordBreak::WSegSpace }, //
      { { 0x0200C, 0x0200C }, WordBreak::Extend }, //
      { { 0x0200D, 0x0200D }, WordBreak::ZWJ }, //
      { { 0x0200E, 0x0200F }, WordBreak::Format }, //
      { { 0x02018, 0x02018 }, WordBreak::MidNumLet }, //
      { { 0x02019, 0x02019 }, WordBreak::MidNumLet }, //
      { { 0x02024, 0x02024 }, WordBreak::MidNumLet }, //
      { { 0x02027, 0x02027 }, WordBreak::MidLetter }, //
      { { 0x02028, 0x02028 }, WordBreak::Newline }, //
      { { 0x02029, 0x02029 }, WordBreak::Newline }, //
      { { 0x0202A, 0x0202E }, WordBreak::Format }, //
      { { 0x0202F, 0x0202F }, WordBreak::ExtendNumLet }, //
      { { 0x0203F, 0x02040 }, WordBreak::ExtendNumLet }, //
      { { 0x02044, 0x02044 }, WordBreak::MidNum }, //
      { { 0x02054, 0x02054 }, WordBreak::ExtendNumLet }, //
      { { 0x0205F, 0x0205F }, WordBreak::WSegSpace }, //
      { { 0x02060, 0x02064 }, WordBreak::Format }, //
      { { 0x02066, 0x0206F }, WordBreak::Format }, //
      { { 0x02071, 0x02071 }, WordBreak::ALetter }, //
      { { 0x0207F, 0x0207F }, WordBreak::ALetter }, //
      { { 0x02090, 0x0209C }, WordBreak::ALetter }, //
      { { 0x020D0, 0x020DC }, WordBreak::Extend }, //
      { { 0x020DD, 0x020E0 }, WordBreak::Extend }, //
      { { 0x020E1, 0x020E1 }, WordBreak::Extend }, //
      { { 0x020E2, 0x020E4 }, WordBreak::Extend }, //
      { { 0x020E5, 0x020F0 }, WordBreak::Extend }, //
      { { 0x02102, 0x02102 }, WordBreak::ALetter }, //
      { { 0x02107, 0x02107 }, WordBreak::ALetter }, //
      { { 0x0210A, 0x02113 }, WordBreak::ALetter }, //
      { { 0x02115, 0x02115 }, WordBreak::ALetter }, //
      { { 0x02119, 0x0211D }, WordBreak::ALetter }, //
      { { 0x02124, 0x02124 }, WordBreak::ALetter }, //
      { { 0x02126, 0x02126 }, WordBreak::ALetter }, //
      { { 0x02128, 0x02128 }, WordBreak::ALetter }, //
      { { 0x0212A, 0x0212D }, WordBreak::ALetter }, //
      { { 0x0212F, 0x02134 }, WordBreak::ALetter }, //
      { { 0x02135, 0x02138 }, WordBreak::ALetter }, //
      { { 0x02139, 0x02139 }, WordBreak::ALetter }, //
      { { 0x0213C, 0x0213F }, WordBreak::ALetter }, //
      { { 0x02145, 0x02149 }, WordBreak::ALetter }, //
      { { 0x0214E, 0x0214E }, WordBreak::ALetter }, //
      { { 0x02160, 0x02182 }, WordBreak::ALetter }, //
      { { 0x02183, 0x02184 }, WordBreak::ALetter }, //
      { { 0x02185, 0x02188 }, WordBreak::ALetter }, //
      { { 0x024B6, 0x024E9 }, WordBreak::ALetter }, //
      { { 0x02C00, 0x02C2E }, WordBreak::ALetter }, //
      { { 0x02C30, 0x02C5E }, WordBreak::ALetter }, //
      { { 0x02C60, 0x02C7B }, WordBreak::ALetter }, //
      { { 0x02C7C, 0x02C7D }, WordBreak::ALetter }, //
      { { 0x02C7E, 0x02CE4 }, WordBreak::ALetter }, //
      { { 0x02CEB, 0x02CEE }, WordBreak::ALetter }, //
      { { 0x02CEF, 0x02CF1 }, WordBreak::Extend }, //
      { { 0x02CF2, 0x02CF3 }, WordBreak::ALetter }, //
      { { 0x02D00, 0x02D25 }, WordBreak::ALetter }, //
      { { 0x02D27, 0x02D27 }, WordBreak::ALetter }, //
      { { 0x02D2D, 0x02D2D }, WordBreak::ALetter }, //
      { { 0x02D30, 0x02D67 }, WordBreak::ALetter }, //
      { { 0x02D6F, 0x02D6F }, WordBreak::ALetter }, //
      { { 0x02D7F, 0x02D7F }, WordBreak::Extend }, //
      { { 0x02D80, 0x02D96 }, WordBreak::ALetter }, //
      { { 0x02DA0, 0x02DA6 }, WordBreak::ALetter }, //
      { { 0x02DA8, 0x02DAE }, WordBreak::ALetter }, //
      { { 0x02DB0, 0x02DB6 }, WordBreak::ALetter }, //
      { { 0x02DB8, 0x02DBE }, WordBreak::ALetter }, //
      { { 0x02DC0, 0x02DC6 }, WordBreak::ALetter }, //
      { { 0x02DC8, 0x02DCE }, WordBreak::ALetter }, //
      { { 0x02DD0, 0x02DD6 }, WordBreak::ALetter }, //
      { { 0x02DD8, 0x02DDE }, WordBreak::ALetter }, //
      { { 0x02DE0, 0x02DFF }, WordBreak::Extend }, //
      { { 0x02E2F, 0x02E2F }, WordBreak::ALetter }, //
      { { 0x03000, 0x03000 }, WordBreak::WSegSpace }, //
      { { 0x03005, 0x03005 }, WordBreak::ALetter }, //
      { { 0x0302A, 0x0302D }, WordBreak::Extend }, //
      { { 0x0302E, 0x0302F }, WordBreak::Extend }, //
      { { 0x03031, 0x03035 }, WordBreak::Katakana }, //
      { { 0x0303B, 0x0303B }, WordBreak::ALetter }, //
      { { 0x0303C, 0x0303C }, WordBreak::ALetter }, //
      { { 0x03099, 0x0309A }, WordBreak::Extend }, //
      { { 0x0309B, 0x0309C }, WordBreak::Katakana }, //
      { { 0x030A0, 0x030A0 }, WordBreak::Katakana }, //
      { { 0x030A1, 0x030FA }, WordBreak::Katakana }, //
      { { 0x030FC, 0x030FE }, WordBreak::Katakana }, //
      { { 0x030FF, 0x030FF }, WordBreak::Katakana }, //
      { { 0x03105, 0x0312F }, WordBreak::ALetter }, //
      { { 0x03131, 0x0318E }, WordBreak::ALetter }, //
      { { 0x031A0, 0x031BF }, WordBreak::ALetter }, //
      { { 0x031F0, 0x031FF }, WordBreak::Katakana }, //
      { { 0x032D0, 0x032FE }, WordBreak::Katakana }, //
      { { 0x03300, 0x03357 }, WordBreak::Katakana }, //
      { { 0x0A000, 0x0A014 }, WordBreak::ALetter }, //
      { { 0x0A015, 0x0A015 }, WordBreak::ALetter }, //
      { { 0x0A016, 0x0A48C }, WordBreak::ALetter }, //
      { { 0x0A4D0, 0x0A4F7 }, WordBreak::ALetter }, //
      { { 0x0A4F8, 0x0A4FD }, WordBreak::ALetter }, //
      { { 0x0A500, 0x0A60B }, WordBreak::ALetter }, //
      { { 0x0A60C, 0x0A60C }, WordBreak::ALetter }, //
      { { 0x0A610, 0x0A61F }, WordBreak::ALetter }, //
      { { 0x0A620, 0x0A629 }, WordBreak::Numeric }, //
      { { 0x0A62A, 0x0A62B }, WordBreak::ALetter }, //
      { { 0x0A640, 0x0A66D }, WordBreak::ALetter }, //
      { { 0x0A66E, 0x0A66E }, WordBreak::ALetter }, //
      { { 0x0A66F, 0x0A66F }, WordBreak::Extend }, //
      { { 0x0A670, 0x0A672 }, WordBreak::Extend }, //
      { { 0x0A674, 0x0A67D }, WordBreak::Extend }, //
      { { 0x0A67F, 0x0A67F }, WordBreak::ALetter }, //
      { { 0x0A680, 0x0A69B }, WordBreak::ALetter }, //
      { { 0x0A69C, 0x0A69D }, WordBreak::ALetter }, //
      { { 0x0A69E, 0x0A69F }, WordBreak::Extend }, //
      { { 0x0A6A0, 0x0A6E5 }, WordBreak::ALetter }, //
      { { 0x0A6E6, 0x0A6EF }, WordBreak::ALetter }, //
      { { 0x0A6F0, 0x0A6F1 }, WordBreak::Extend }, //
      { { 0x0A708, 0x0A716 }, WordBreak::ALetter }, //
      { { 0x0A717, 0x0A71F }, WordBreak::ALetter }, //
      { { 0x0A720, 0x0A721 }, WordBreak::ALetter }, //
      { { 0x0A722, 0x0A76F }, WordBreak::ALetter }, //
      { { 0x0A770, 0x0A770 }, WordBreak::ALetter }, //
      { { 0x0A771, 0x0A787 }, WordBreak::ALetter }, //
      { { 0x0A788, 0x0A788 }, WordBreak::ALetter }, //
      { { 0x0A789, 0x0A78A }, WordBreak::ALetter }, //
      { { 0x0A78B, 0x0A78E }, WordBreak::ALetter }, //
      { { 0x0A78F, 0x0A78F }, WordBreak::ALetter }, //
      { { 0x0A790, 0x0A7BF }, WordBreak::ALetter }, //
      { { 0x0A7C2, 0x0A7CA }, WordBreak::ALetter }, //
      { { 0x0A7F5, 0x0A7F6 }, WordBreak::ALetter }, //
      { { 0x0A7F7, 0x0A7F7 }, WordBreak::ALetter }, //
      { { 0x0A7F8, 0x0A7F9 }, WordBreak::ALetter }, //
      { { 0x0A7FA, 0x0A7FA }, WordBreak::ALetter }, //
      { { 0x0A7FB, 0x0A801 }, WordBreak::ALetter }, //
      { { 0x0A802, 0x0A802 }, WordBreak::Extend }, //
      { { 0x0A803, 0x0A805 }, WordBreak::ALetter }, //
      { { 0x0A806, 0x0A806 }, WordBreak::Extend }, //
      { { 0x0A807, 0x0A80A }, WordBreak::ALetter }, //
      { { 0x0A80B, 0x0A80B }, WordBreak::Extend }, //
      { { 0x0A80C, 0x0A822 }, WordBreak::ALetter }, //
      { { 0x0A823, 0x0A824 }, WordBreak::Extend }, //
      { { 0x0A825, 0x0A826 }, WordBreak::Extend }, //
      { { 0x0A827, 0x0A827 }, WordBreak::Extend }, //
      { { 0x0A82C, 0x0A82C }, WordBreak::Extend }, //
      { { 0x0A840, 0x0A873 }, WordBreak::ALetter }, //
      { { 0x0A880, 0x0A881 }, WordBreak::Extend }, //
      { { 0x0A882, 0x0A8B3 }, WordBreak::ALetter }, //
      { { 0x0A8B4, 0x0A8C3 }, WordBreak::Extend }, //
      { { 0x0A8C4, 0x0A8C5 }, WordBreak::Extend }, //
      { { 0x0A8D0, 0x0A8D9 }, WordBreak::Numeric }, //
      { { 0x0A8E0, 0x0A8F1 }, WordBreak::Extend }, //
      { { 0x0A8F2, 0x0A8F7 }, WordBreak::ALetter }, //
      { { 0x0A8FB, 0x0A8FB }, WordBreak::ALetter }, //
      { { 0x0A8FD, 0x0A8FE }, WordBreak::ALetter }, //
      { { 0x0A8FF, 0x0A8FF }, WordBreak::Extend }, //
      { { 0x0A900, 0x0A909 }, WordBreak::Numeric }, //
      { { 0x0A90A, 0x0A925 }, WordBreak::ALetter }, //
      { { 0x0A926, 0x0A92D }, WordBreak::Extend }, //
      { { 0x0A930, 0x0A946 }, WordBreak::ALetter }, //
      { { 0x0A947, 0x0A951 }, WordBreak::Extend }, //
      { { 0x0A952, 0x0A953 }, WordBreak::Extend }, //
      { { 0x0A960, 0x0A97C }, WordBreak::ALetter }, //
      { { 0x0A980, 0x0A982 }, WordBreak::Extend }, //
      { { 0x0A983, 0x0A983 }, WordBreak::Extend }, //
      { { 0x0A984, 0x0A9B2 }, WordBreak::ALetter }, //
      { { 0x0A9B3, 0x0A9B3 }, WordBreak::Extend }, //
      { { 0x0A9B4, 0x0A9B5 }, WordBreak::Extend }, //
      { { 0x0A9B6, 0x0A9B9 }, WordBreak::Extend }, //
      { { 0x0A9BA, 0x0A9BB }, WordBreak::Extend }, //
      { { 0x0A9BC, 0x0A9BD }, WordBreak::Extend }, //
      { { 0x0A9BE, 0x0A9C0 }, WordBreak::Extend }, //
      { { 0x0A9CF, 0x0A9CF }, WordBreak::ALetter }, //
      { { 0x0A9D0, 0x0A9D9 }, WordBreak::Numeric }, //
      { { 0x0A9E5, 0x0A9E5 }, WordBreak::Extend }, //
      { { 0x0A9F0, 0x0A9F9 }, WordBreak::Numeric }, //
      { { 0x0AA00, 0x0AA28 }, WordBreak::ALetter }, //
      { { 0x0AA29, 0x0AA2E }, WordBreak::Extend }, //
      { { 0x0AA2F, 0x0AA30 }, WordBreak::Extend }, //
      { { 0x0AA31, 0x0AA32 }, WordBreak::Extend }, //
      { { 0x0AA33, 0x0AA34 }, WordBreak::Extend }, //
      { { 0x0AA35, 0x0AA36 }, WordBreak::Extend }, //
      { { 0x0AA40, 0x0AA42 }, WordBreak::ALetter }, //
      { { 0x0AA43, 0x0AA43 }, WordBreak::Extend }, //
      { { 0x0AA44, 0x0AA4B }, WordBreak::ALetter }, //
      { { 0x0AA4C, 0x0AA4C }, WordBreak::Extend }, //
      { { 0x0AA4D, 0x0AA4D }, WordBreak::Extend }, //
      { { 0x0AA50, 0x0AA59 }, WordBreak::Numeric }, //
      { { 0x0AA7B, 0x0AA7B }, WordBreak::Extend }, //
      { { 0x0AA7C, 0x0AA7C }, WordBreak::Extend }, //
      { { 0x0AA7D, 0x0AA7D }, WordBreak::Extend }, //
      { { 0x0AAB0, 0x0AAB0 }, WordBreak::Extend }, //
      { { 0x0AAB2, 0x0AAB4 }, WordBreak::Extend }, //
      { { 0x0AAB7, 0x0AAB8 }, WordBreak::Extend }, //
      { { 0x0AABE, 0x0AABF }, WordBreak::Extend }, //
      { { 0x0AAC1, 0x0AAC1 }, WordBreak::Extend }, //
      { { 0x0AAE0, 0x0AAEA }, WordBreak::ALetter }, //
      { { 0x0AAEB, 0x0AAEB }, WordBreak::Extend }, //
      { { 0x0AAEC, 0x0AAED }, WordBreak::Extend }, //
      { { 0x0AAEE, 0x0AAEF }, WordBreak::Extend }, //
      { { 0x0AAF2, 0x0AAF2 }, WordBreak::ALetter }, //
      { { 0x0AAF3, 0x0AAF4 }, WordBreak::ALetter }, //
      { { 0x0AAF5, 0x0AAF5 }, WordBreak::Extend }, //
      { { 0x0AAF6, 0x0AAF6 }, WordBreak::Extend }, //
      { { 0x0AB01, 0x0AB06 }, WordBreak::ALetter }, //
      { { 0x0AB09, 0x0AB0E }, WordBreak::ALetter }, //
      { { 0x0AB11, 0x0AB16 }, WordBreak::ALetter }, //
      { { 0x0AB20, 0x0AB26 }, WordBreak::ALetter }, //
      { { 0x0AB28, 0x0AB2E }, WordBreak::ALetter }, //
      { { 0x0AB30, 0x0AB5A }, WordBreak::ALetter }, //
      { { 0x0AB5B, 0x0AB5B }, WordBreak::ALetter }, //
      { { 0x0AB5C, 0x0AB5F }, WordBreak::ALetter }, //
      { { 0x0AB60, 0x0AB68 }, WordBreak::ALetter }, //
      { { 0x0AB69, 0x0AB69 }, WordBreak::ALetter }, //
      { { 0x0AB70, 0x0ABBF }, WordBreak::ALetter }, //
      { { 0x0ABC0, 0x0ABE2 }, WordBreak::ALetter }, //
      { { 0x0ABE3, 0x0ABE4 }, WordBreak::Extend }, //
      { { 0x0ABE5, 0x0ABE5 }, WordBreak::Extend }, //
      { { 0x0ABE6, 0x0ABE7 }, WordBreak::Extend }, //
      { { 0x0ABE8, 0x0ABE8 }, WordBreak::Extend }, //
      { { 0x0ABE9, 0x0ABEA }, WordBreak::Extend }, //
      { { 0x0ABEC, 0x0ABEC }, WordBreak::Extend }, //
      { { 0x0ABED, 0x0ABED }, WordBreak::Extend }, //
      { { 0x0ABF0, 0x0ABF9 }, WordBreak::Numeric }, //
      { { 0x0AC00, 0x0D7A3 }, WordBreak::ALetter }, //
      { { 0x0D7B0, 0x0D7C6 }, WordBreak::ALetter }, //
      { { 0x0D7CB, 0x0D7FB }, WordBreak::ALetter }, //
      { { 0x0FB00, 0x0FB06 }, WordBreak::ALetter }, //
      { { 0x0FB13, 0x0FB17 }, WordBreak::ALetter }, //
      { { 0x0FB1D, 0x0FB1D }, WordBreak::Hebrew_Letter }, //
      { { 0x0FB1E, 0x0FB1E }, WordBreak::Extend }, //
      { { 0x0FB1F, 0x0FB28 }, WordBreak::Hebrew_Letter }, //
      { { 0x0FB2A, 0x0FB36 }, WordBreak::Hebrew_Letter }, //
      { { 0x0FB38, 0x0FB3C }, WordBreak::Hebrew_Letter }, //
      { { 0x0FB3E, 0x0FB3E }, WordBreak::Hebrew_Letter }, //
      { { 0x0FB40, 0x0FB41 }, WordBreak::Hebrew_Letter }, //
      { { 0x0FB43, 0x0FB44 }, WordBreak::Hebrew_Letter }, //
      { { 0x0FB46, 0x0FB4F }, WordBreak::Hebrew_Letter }, //
      { { 0x0FB50, 0x0FBB1 }, WordBreak::ALetter }, //
      { { 0x0FBD3, 0x0FD3D }, WordBreak::ALetter }, //
      { { 0x0FD50, 0x0FD8F }, WordBreak::ALetter }, //
      { { 0x0FD92, 0x0FDC7 }, WordBreak::ALetter }, //
      { { 0x0FDF0, 0x0FDFB }, WordBreak::ALetter }, //
      { { 0x0FE00, 0x0FE0F }, WordBreak::Extend }, //
      { { 0x0FE10, 0x0FE10 }, WordBreak::MidNum }, //
      { { 0x0FE13, 0x0FE13 }, WordBreak::MidLetter }, //
      { { 0x0FE14, 0x0FE14 }, WordBreak::MidNum }, //
      { { 0x0FE20, 0x0FE2F }, WordBreak::Extend }, //
      { { 0x0FE33, 0x0FE34 }, WordBreak::ExtendNumLet }, //
      { { 0x0FE4D, 0x0FE4F }, WordBreak::ExtendNumLet }, //
      { { 0x0FE50, 0x0FE50 }, WordBreak::MidNum }, //
      { { 0x0FE52, 0x0FE52 }, WordBreak::MidNumLet }, //
      { { 0x0FE54, 0x0FE54 }, WordBreak::MidNum }, //
      { { 0x0FE55, 0x0FE55 }, WordBreak::MidLetter }, //
      { { 0x0FE70, 0x0FE74 }, WordBreak::ALetter }, //
      { { 0x0FE76, 0x0FEFC }, WordBreak::ALetter }, //
      { { 0x0FEFF, 0x0FEFF }, WordBreak::Format }, //
      { { 0x0FF07, 0x0FF07 }, WordBreak::MidNumLet }, //
      { { 0x0FF0C, 0x0FF0C }, WordBreak::MidNum }, //
      { { 0x0FF0E, 0x0FF0E }, WordBreak::MidNumLet }, //
      { { 0x0FF10, 0x0FF19 }, WordBreak::Numeric }, //
      { { 0x0FF1A, 0x0FF1A }, WordBreak::MidLetter }, //
      { { 0x0FF1B, 0x0FF1B }, WordBreak::MidNum }, //
      { { 0x0FF21, 0x0FF3A }, WordBreak::ALetter }, //
      { { 0x0FF3F, 0x0FF3F }, WordBreak::ExtendNumLet }, //
      { { 0x0FF41, 0x0FF5A }, WordBreak::ALetter }, //
      { { 0x0FF66, 0x0FF6F }, WordBreak::Katakana }, //
      { { 0x0FF70, 0x0FF70 }, WordBreak::Katakana }, //
      { { 0x0FF71, 0x0FF9D }, WordBreak::Katakana }, //
      { { 0x0FF9E, 0x0FF9F }, WordBreak::Extend }, //
      { { 0x0FFA0, 0x0FFBE }, WordBreak::ALetter }, //
      { { 0x0FFC2, 0x0FFC7 }, WordBreak::ALetter }, //
      { { 0x0FFCA, 0x0FFCF }, WordBreak::ALetter }, //
      { { 0x0FFD2, 0x0FFD7 }, WordBreak::ALetter }, //
      { { 0x0FFDA, 0x0FFDC }, WordBreak::ALetter }, //
      { { 0x0FFF9, 0x0FFFB }, WordBreak::Format }, //
      { { 0x10000, 0x1000B }, WordBreak::ALetter }, //
      { { 0x1000D, 0x10026 }, WordBreak::ALetter }, //
      { { 0x10028, 0x1003A }, WordBreak::ALetter }, //
      { { 0x1003C, 0x1003D }, WordBreak::ALetter }, //
      { { 0x1003F, 0x1004D }, WordBreak::ALetter }, //
      { { 0x10050, 0x1005D }, WordBreak::ALetter }, //
      { { 0x10080, 0x100FA }, WordBreak::ALetter }, //
      { { 0x10140, 0x10174 }, WordBreak::ALetter }, //
      { { 0x101FD, 0x101FD }, WordBreak::Extend }, //
      { { 0x10280, 0x1029C }, WordBreak::ALetter }, //
      { { 0x102A0, 0x102D0 }, WordBreak::ALetter }, //
      { { 0x102E0, 0x102E0 }, WordBreak::Extend }, //
      { { 0x10300, 0x1031F }, WordBreak::ALetter }, //
      { { 0x1032D, 0x10340 }, WordBreak::ALetter }, //
      { { 0x10341, 0x10341 }, WordBreak::ALetter }, //
      { { 0x10342, 0x10349 }, WordBreak::ALetter }, //
      { { 0x1034A, 0x1034A }, WordBreak::ALetter }, //
      { { 0x10350, 0x10375 }, WordBreak::ALetter }, //
      { { 0x10376, 0x1037A }, WordBreak::Extend }, //
      { { 0x10380, 0x1039D }, WordBreak::ALetter }, //
      { { 0x103A0, 0x103C3 }, WordBreak::ALetter }, //
      { { 0x103C8, 0x103CF }, WordBreak::ALetter }, //
      { { 0x103D1, 0x103D5 }, WordBreak::ALetter }, //
      { { 0x10400, 0x1044F }, WordBreak::ALetter }, //
      { { 0x10450, 0x1049D }, WordBreak::ALetter }, //
      { { 0x104A0, 0x104A9 }, WordBreak::Numeric }, //
      { { 0x104B0, 0x104D3 }, WordBreak::ALetter }, //
      { { 0x104D8, 0x104FB }, WordBreak::ALetter }, //
      { { 0x10500, 0x10527 }, WordBreak::ALetter }, //
      { { 0x10530, 0x10563 }, WordBreak::ALetter }, //
      { { 0x10600, 0x10736 }, WordBreak::ALetter }, //
      { { 0x10740, 0x10755 }, WordBreak::ALetter }, //
      { { 0x10760, 0x10767 }, WordBreak::ALetter }, //
      { { 0x10800, 0x10805 }, WordBreak::ALetter }, //
      { { 0x10808, 0x10808 }, WordBreak::ALetter }, //
      { { 0x1080A, 0x10835 }, WordBreak::ALetter }, //
      { { 0x10837, 0x10838 }, WordBreak::ALetter }, //
      { { 0x1083C, 0x1083C }, WordBreak::ALetter }, //
      { { 0x1083F, 0x10855 }, WordBreak::ALetter }, //
      { { 0x10860, 0x10876 }, WordBreak::ALetter }, //
      { { 0x10880, 0x1089E }, WordBreak::ALetter }, //
      { { 0x108E0, 0x108F2 }, WordBreak::ALetter }, //
      { { 0x108F4, 0x108F5 }, WordBreak::ALetter }, //
      { { 0x10900, 0x10915 }, WordBreak::ALetter }, //
      { { 0x10920, 0x10939 }, WordBreak::ALetter }, //
      { { 0x10980, 0x109B7 }, WordBreak::ALetter }, //
      { { 0x109BE, 0x109BF }, WordBreak::ALetter }, //
      { { 0x10A00, 0x10A00 }, WordBreak::ALetter }, //
      { { 0x10A01, 0x10A03 }, WordBreak::Extend }, //
      { { 0x10A05, 0x10A06 }, WordBreak::Extend }, //
      { { 0x10A0C, 0x10A0F }, WordBreak::Extend }, //
      { { 0x10A10, 0x10A13 }, WordBreak::ALetter }, //
      { { 0x10A15, 0x10A17 }, WordBreak::ALetter }, //
      { { 0x10A19, 0x10A35 }, WordBreak::ALetter }, //
      { { 0x10A38, 0x10A3A }, WordBreak::Extend }, //
      { { 0x10A3F, 0x10A3F }, WordBreak::Extend }, //
      { { 0x10A60, 0x10A7C }, WordBreak::ALetter }, //
      { { 0x10A80, 0x10A9C }, WordBreak::ALetter }, //
      { { 0x10AC0, 0x10AC7 }, WordBreak::ALetter }, //
      { { 0x10AC9, 0x10AE4 }, WordBreak::ALetter }, //
      { { 0x10AE5, 0x10AE6 }, WordBreak::Extend }, //
      { { 0x10B00, 0x10B35 }, WordBreak::ALetter }, //
      { { 0x10B40, 0x10B55 }, WordBreak::ALetter }, //
      { { 0x10B60, 0x10B72 }, WordBreak::ALetter }, //
      { { 0x10B80, 0x10B91 }, WordBreak::ALetter }, //
      { { 0x10C00, 0x10C48 }, WordBreak::ALetter }, //
      { { 0x10C80, 0x10CB2 }, WordBreak::ALetter }, //
      { { 0x10CC0, 0x10CF2 }, WordBreak::ALetter }, //
      { { 0x10D00, 0x10D23 }, WordBreak::ALetter }, //
      { { 0x10D24, 0x10D27 }, WordBreak::Extend }, //
      { { 0x10D30, 0x10D39 }, WordBreak::Numeric }, //
      { { 0x10E80, 0x10EA9 }, WordBreak::ALetter }, //
      { { 0x10EAB, 0x10EAC }, WordBreak::Extend }, //
      { { 0x10EB0, 0x10EB1 }, WordBreak::ALetter }, //
      { { 0x10F00, 0x10F1C }, WordBreak::ALetter }, //
      { { 0x10F27, 0x10F27 }, WordBreak::ALetter }, //
      { { 0x10F30, 0x10F45 }, WordBreak::ALetter }, //
      { { 0x10F46, 0x10F50 }, WordBreak::Extend }, //
      { { 0x10FB0, 0x10FC4 }, WordBreak::ALetter }, //
      { { 0x10FE0, 0x10FF6 }, WordBreak::ALetter }, //
      { { 0x11000, 0x11000 }, WordBreak::Extend }, //
      { { 0x11001, 0x11001 }, WordBreak::Extend }, //
      { { 0x11002, 0x11002 }, WordBreak::Extend }, //
      { { 0x11003, 0x11037 }, WordBreak::ALetter }, //
      { { 0x11038, 0x11046 }, WordBreak::Extend }, //
      { { 0x11066, 0x1106F }, WordBreak::Numeric }, //
      { { 0x1107F, 0x11081 }, WordBreak::Extend }, //
      { { 0x11082, 0x11082 }, WordBreak::Extend }, //
      { { 0x11083, 0x110AF }, WordBreak::ALetter }, //
      { { 0x110B0, 0x110B2 }, WordBreak::Extend }, //
      { { 0x110B3, 0x110B6 }, WordBreak::Extend }, //
      { { 0x110B7, 0x110B8 }, WordBreak::Extend }, //
      { { 0x110B9, 0x110BA }, WordBreak::Extend }, //
      { { 0x110BD, 0x110BD }, WordBreak::Format }, //
      { { 0x110CD, 0x110CD }, WordBreak::Format }, //
      { { 0x110D0, 0x110E8 }, WordBreak::ALetter }, //
      { { 0x110F0, 0x110F9 }, WordBreak::Numeric }, //
      { { 0x11100, 0x11102 }, WordBreak::Extend }, //
      { { 0x11103, 0x11126 }, WordBreak::ALetter }, //
      { { 0x11127, 0x1112B }, WordBreak::Extend }, //
      { { 0x1112C, 0x1112C }, WordBreak::Extend }, //
      { { 0x1112D, 0x11134 }, WordBreak::Extend }, //
      { { 0x11136, 0x1113F }, WordBreak::Numeric }, //
      { { 0x11144, 0x11144 }, WordBreak::ALetter }, //
      { { 0x11145, 0x11146 }, WordBreak::Extend }, //
      { { 0x11147, 0x11147 }, WordBreak::ALetter }, //
      { { 0x11150, 0x11172 }, WordBreak::ALetter }, //
      { { 0x11173, 0x11173 }, WordBreak::Extend }, //
      { { 0x11176, 0x11176 }, WordBreak::ALetter }, //
      { { 0x11180, 0x11181 }, WordBreak::Extend }, //
      { { 0x11182, 0x11182 }, WordBreak::Extend }, //
      { { 0x11183, 0x111B2 }, WordBreak::ALetter }, //
      { { 0x111B3, 0x111B5 }, WordBreak::Extend }, //
      { { 0x111B6, 0x111BE }, WordBreak::Extend }, //
      { { 0x111BF, 0x111C0 }, WordBreak::Extend }, //
      { { 0x111C1, 0x111C4 }, WordBreak::ALetter }, //
      { { 0x111C9, 0x111CC }, WordBreak::Extend }, //
      { { 0x111CE, 0x111CE }, WordBreak::Extend }, //
      { { 0x111CF, 0x111CF }, WordBreak::Extend }, //
      { { 0x111D0, 0x111D9 }, WordBreak::Numeric }, //
      { { 0x111DA, 0x111DA }, WordBreak::ALetter }, //
      { { 0x111DC, 0x111DC }, WordBreak::ALetter }, //
      { { 0x11200, 0x11211 }, WordBreak::ALetter }, //
      { { 0x11213, 0x1122B }, WordBreak::ALetter }, //
      { { 0x1122C, 0x1122E }, WordBreak::Extend }, //
      { { 0x1122F, 0x11231 }, WordBreak::Extend }, //
      { { 0x11232, 0x11233 }, WordBreak::Extend }, //
      { { 0x11234, 0x11234 }, WordBreak::Extend }, //
      { { 0x11235, 0x11235 }, WordBreak::Extend }, //
      { { 0x11236, 0x11237 }, WordBreak::Extend }, //
      { { 0x1123E, 0x1123E }, WordBreak::Extend }, //
      { { 0x11280, 0x11286 }, WordBreak::ALetter }, //
      { { 0x11288, 0x11288 }, WordBreak::ALetter }, //
      { { 0x1128A, 0x1128D }, WordBreak::ALetter }, //
      { { 0x1128F, 0x1129D }, WordBreak::ALetter }, //
      { { 0x1129F, 0x112A8 }, WordBreak::ALetter }, //
      { { 0x112B0, 0x112DE }, WordBreak::ALetter }, //
      { { 0x112DF, 0x112DF }, WordBreak::Extend }, //
      { { 0x112E0, 0x112E2 }, WordBreak::Extend }, //
      { { 0x112E3, 0x112EA }, WordBreak::Extend }, //
      { { 0x112F0, 0x112F9 }, WordBreak::Numeric }, //
      { { 0x11300, 0x11301 }, WordBreak::Extend }, //
      { { 0x11302, 0x11303 }, WordBreak::Extend }, //
      { { 0x11305, 0x1130C }, WordBreak::ALetter }, //
      { { 0x1130F, 0x11310 }, WordBreak::ALetter }, //
      { { 0x11313, 0x11328 }, WordBreak::ALetter }, //
      { { 0x1132A, 0x11330 }, WordBreak::ALetter }, //
      { { 0x11332, 0x11333 }, WordBreak::ALetter }, //
      { { 0x11335, 0x11339 }, WordBreak::ALetter }, //
      { { 0x1133B, 0x1133C }, WordBreak::Extend }, //
      { { 0x1133D, 0x1133D }, WordBreak::ALetter }, //
      { { 0x1133E, 0x1133F }, WordBreak::Extend }, //
      { { 0x11340, 0x11340 }, WordBreak::Extend }, //
      { { 0x11341, 0x11344 }, WordBreak::Extend }, //
      { { 0x11347, 0x11348 }, WordBreak::Extend }, //
      { { 0x1134B, 0x1134D }, WordBreak::Extend }, //
      { { 0x11350, 0x11350 }, WordBreak::ALetter }, //
      { { 0x11357, 0x11357 }, WordBreak::Extend }, //
      { { 0x1135D, 0x11361 }, WordBreak::ALetter }, //
      { { 0x11362, 0x11363 }, WordBreak::Extend }, //
      { { 0x11366, 0x1136C }, WordBreak::Extend }, //
      { { 0x11370, 0x11374 }, WordBreak::Extend }, //
      { { 0x11400, 0x11434 }, WordBreak::ALetter }, //
      { { 0x11435, 0x11437 }, WordBreak::Extend }, //
      { { 0x11438, 0x1143F }, WordBreak::Extend }, //
      { { 0x11440, 0x11441 }, WordBreak::Extend }, //
      { { 0x11442, 0x11444 }, WordBreak::Extend }, //
      { { 0x11445, 0x11445 }, WordBreak::Extend }, //
      { { 0x11446, 0x11446 }, WordBreak::Extend }, //
      { { 0x11447, 0x1144A }, WordBreak::ALetter }, //
      { { 0x11450, 0x11459 }, WordBreak::Numeric }, //
      { { 0x1145E, 0x1145E }, WordBreak::Extend }, //
      { { 0x1145F, 0x11461 }, WordBreak::ALetter }, //
      { { 0x11480, 0x114AF }, WordBreak::ALetter }, //
      { { 0x114B0, 0x114B2 }, WordBreak::Extend }, //
      { { 0x114B3, 0x114B8 }, WordBreak::Extend }, //
      { { 0x114B9, 0x114B9 }, WordBreak::Extend }, //
      { { 0x114BA, 0x114BA }, WordBreak::Extend }, //
      { { 0x114BB, 0x114BE }, WordBreak::Extend }, //
      { { 0x114BF, 0x114C0 }, WordBreak::Extend }, //
      { { 0x114C1, 0x114C1 }, WordBreak::Extend }, //
      { { 0x114C2, 0x114C3 }, WordBreak::Extend }, //
      { { 0x114C4, 0x114C5 }, WordBreak::ALetter }, //
      { { 0x114C7, 0x114C7 }, WordBreak::ALetter }, //
      { { 0x114D0, 0x114D9 }, WordBreak::Numeric }, //
      { { 0x11580, 0x115AE }, WordBreak::ALetter }, //
      { { 0x115AF, 0x115B1 }, WordBreak::Extend }, //
      { { 0x115B2, 0x115B5 }, WordBreak::Extend }, //
      { { 0x115B8, 0x115BB }, WordBreak::Extend }, //
      { { 0x115BC, 0x115BD }, WordBreak::Extend }, //
      { { 0x115BE, 0x115BE }, WordBreak::Extend }, //
      { { 0x115BF, 0x115C0 }, WordBreak::Extend }, //
      { { 0x115D8, 0x115DB }, WordBreak::ALetter }, //
      { { 0x115DC, 0x115DD }, WordBreak::Extend }, //
      { { 0x11600, 0x1162F }, WordBreak::ALetter }, //
      { { 0x11630, 0x11632 }, WordBreak::Extend }, //
      { { 0x11633, 0x1163A }, WordBreak::Extend }, //
      { { 0x1163B, 0x1163C }, WordBreak::Extend }, //
      { { 0x1163D, 0x1163D }, WordBreak::Extend }, //
      { { 0x1163E, 0x1163E }, WordBreak::Extend }, //
      { { 0x1163F, 0x11640 }, WordBreak::Extend }, //
      { { 0x11644, 0x11644 }, WordBreak::ALetter }, //
      { { 0x11650, 0x11659 }, WordBreak::Numeric }, //
      { { 0x11680, 0x116AA }, WordBreak::ALetter }, //
      { { 0x116AB, 0x116AB }, WordBreak::Extend }, //
      { { 0x116AC, 0x116AC }, WordBreak::Extend }, //
      { { 0x116AD, 0x116AD }, WordBreak::Extend }, //
      { { 0x116AE, 0x116AF }, WordBreak::Extend }, //
      { { 0x116B0, 0x116B5 }, WordBreak::Extend }, //
      { { 0x116B6, 0x116B6 }, WordBreak::Extend }, //
      { { 0x116B7, 0x116B7 }, WordBreak::Extend }, //
      { { 0x116B8, 0x116B8 }, WordBreak::ALetter }, //
      { { 0x116C0, 0x116C9 }, WordBreak::Numeric }, //
      { { 0x1171D, 0x1171F }, WordBreak::Extend }, //
      { { 0x11720, 0x11721 }, WordBreak::Extend }, //
      { { 0x11722, 0x11725 }, WordBreak::Extend }, //
      { { 0x11726, 0x11726 }, WordBreak::Extend }, //
      { { 0x11727, 0x1172B }, WordBreak::Extend }, //
      { { 0x11730, 0x11739 }, WordBreak::Numeric }, //
      { { 0x11800, 0x1182B }, WordBreak::ALetter }, //
      { { 0x1182C, 0x1182E }, WordBreak::Extend }, //
      { { 0x1182F, 0x11837 }, WordBreak::Extend }, //
      { { 0x11838, 0x11838 }, WordBreak::Extend }, //
      { { 0x11839, 0x1183A }, WordBreak::Extend }, //
      { { 0x118A0, 0x118DF }, WordBreak::ALetter }, //
      { { 0x118E0, 0x118E9 }, WordBreak::Numeric }, //
      { { 0x118FF, 0x11906 }, WordBreak::ALetter }, //
      { { 0x11909, 0x11909 }, WordBreak::ALetter }, //
      { { 0x1190C, 0x11913 }, WordBreak::ALetter }, //
      { { 0x11915, 0x11916 }, WordBreak::ALetter }, //
      { { 0x11918, 0x1192F }, WordBreak::ALetter }, //
      { { 0x11930, 0x11935 }, WordBreak::Extend }, //
      { { 0x11937, 0x11938 }, WordBreak::Extend }, //
      { { 0x1193B, 0x1193C }, WordBreak::Extend }, //
      { { 0x1193D, 0x1193D }, WordBreak::Extend }, //
      { { 0x1193E, 0x1193E }, WordBreak::Extend }, //
      { { 0x1193F, 0x1193F }, WordBreak::ALetter }, //
      { { 0x11940, 0x11940 }, WordBreak::Extend }, //
      { { 0x11941, 0x11941 }, WordBreak::ALetter }, //
      { { 0x11942, 0x11942 }, WordBreak::Extend }, //
      { { 0x11943, 0x11943 }, WordBreak::Extend }, //
      { { 0x11950, 0x11959 }, WordBreak::Numeric }, //
      { { 0x119A0, 0x119A7 }, WordBreak::ALetter }, //
      { { 0x119AA, 0x119D0 }, WordBreak::ALetter }, //
      { { 0x119D1, 0x119D3 }, WordBreak::Extend }, //
      { { 0x119D4, 0x119D7 }, WordBreak::Extend }, //
      { { 0x119DA, 0x119DB }, WordBreak::Extend }, //
      { { 0x119DC, 0x119DF }, WordBreak::Extend }, //
      { { 0x119E0, 0x119E0 }, WordBreak::Extend }, //
      { { 0x119E1, 0x119E1 }, WordBreak::ALetter }, //
      { { 0x119E3, 0x119E3 }, WordBreak::ALetter }, //
      { { 0x119E4, 0x119E4 }, WordBreak::Extend }, //
      { { 0x11A00, 0x11A00 }, WordBreak::ALetter }, //
      { { 0x11A01, 0x11A0A }, WordBreak::Extend }, //
      { { 0x11A0B, 0x11A32 }, WordBreak::ALetter }, //
      { { 0x11A33, 0x11A38 }, WordBreak::Extend }, //
      { { 0x11A39, 0x11A39 }, WordBreak::Extend }, //
      { { 0x11A3A, 0x11A3A }, WordBreak::ALetter }, //
      { { 0x11A3B, 0x11A3E }, WordBreak::Extend }, //
      { { 0x11A47, 0x11A47 }, WordBreak::Extend }, //
      { { 0x11A50, 0x11A50 }, WordBreak::ALetter }, //
      { { 0x11A51, 0x11A56 }, WordBreak::Extend }, //
      { { 0x11A57, 0x11A58 }, WordBreak::Extend }, //
      { { 0x11A59, 0x11A5B }, WordBreak::Extend }, //
      { { 0x11A5C, 0x11A89 }, WordBreak::ALetter }, //
      { { 0x11A8A, 0x11A96 }, WordBreak::Extend }, //
      { { 0x11A97, 0x11A97 }, WordBreak::Extend }, //
      { { 0x11A98, 0x11A99 }, WordBreak::Extend }, //
      { { 0x11A9D, 0x11A9D }, WordBreak::ALetter }, //
      { { 0x11AC0, 0x11AF8 }, WordBreak::ALetter }, //
      { { 0x11C00, 0x11C08 }, WordBreak::ALetter }, //
      { { 0x11C0A, 0x11C2E }, WordBreak::ALetter }, //
      { { 0x11C2F, 0x11C2F }, WordBreak::Extend }, //
      { { 0x11C30, 0x11C36 }, WordBreak::Extend }, //
      { { 0x11C38, 0x11C3D }, WordBreak::Extend }, //
      { { 0x11C3E, 0x11C3E }, WordBreak::Extend }, //
      { { 0x11C3F, 0x11C3F }, WordBreak::Extend }, //
      { { 0x11C40, 0x11C40 }, WordBreak::ALetter }, //
      { { 0x11C50, 0x11C59 }, WordBreak::Numeric }, //
      { { 0x11C72, 0x11C8F }, WordBreak::ALetter }, //
      { { 0x11C92, 0x11CA7 }, WordBreak::Extend }, //
      { { 0x11CA9, 0x11CA9 }, WordBreak::Extend }, //
      { { 0x11CAA, 0x11CB0 }, WordBreak::Extend }, //
      { { 0x11CB1, 0x11CB1 }, WordBreak::Extend }, //
      { { 0x11CB2, 0x11CB3 }, WordBreak::Extend }, //
      { { 0x11CB4, 0x11CB4 }, WordBreak::Extend }, //
      { { 0x11CB5, 0x11CB6 }, WordBreak::Extend }, //
      { { 0x11D00, 0x11D06 }, WordBreak::ALetter }, //
      { { 0x11D08, 0x11D09 }, WordBreak::ALetter }, //
      { { 0x11D0B, 0x11D30 }, WordBreak::ALetter }, //
      { { 0x11D31, 0x11D36 }, WordBreak::Extend }, //
      { { 0x11D3A, 0x11D3A }, WordBreak::Extend }, //
      { { 0x11D3C, 0x11D3D }, WordBreak::Extend }, //
      { { 0x11D3F, 0x11D45 }, WordBreak::Extend }, //
      { { 0x11D46, 0x11D46 }, WordBreak::ALetter }, //
      { { 0x11D47, 0x11D47 }, WordBreak::Extend }, //
      { { 0x11D50, 0x11D59 }, WordBreak::Numeric }, //
      { { 0x11D60, 0x11D65 }, WordBreak::ALetter }, //
      { { 0x11D67, 0x11D68 }, WordBreak::ALetter }, //
      { { 0x11D6A, 0x11D89 }, WordBreak::ALetter }, //
      { { 0x11D8A, 0x11D8E }, WordBreak::Extend }, //
      { { 0x11D90, 0x11D91 }, WordBreak::Extend }, //
      { { 0x11D93, 0x11D94 }, WordBreak::Extend }, //
      { { 0x11D95, 0x11D95 }, WordBreak::Extend }, //
      { { 0x11D96, 0x11D96 }, WordBreak::Extend }, //
      { { 0x11D97, 0x11D97 }, WordBreak::Extend }, //
      { { 0x11D98, 0x11D98 }, WordBreak::ALetter }, //
      { { 0x11DA0, 0x11DA9 }, WordBreak::Numeric }, //
      { { 0x11EE0, 0x11EF2 }, WordBreak::ALetter }, //
      { { 0x11EF3, 0x11EF4 }, WordBreak::Extend }, //
      { { 0x11EF5, 0x11EF6 }, WordBreak::Extend }, //
      { { 0x11FB0, 0x11FB0 }, WordBreak::ALetter }, //
      { { 0x12000, 0x12399 }, WordBreak::ALetter }, //
      { { 0x12400, 0x1246E }, WordBreak::ALetter }, //
      { { 0x12480, 0x12543 }, WordBreak::ALetter }, //
      { { 0x13000, 0x1342E }, WordBreak::ALetter }, //
      { { 0x13430, 0x13438 }, WordBreak::Format }, //
      { { 0x14400, 0x14646 }, WordBreak::ALetter }, //
      { { 0x16800, 0x16A38 }, WordBreak::ALetter }, //
      { { 0x16A40, 0x16A5E }, WordBreak::ALetter }, //
      { { 0x16A60, 0x16A69 }, WordBreak::Numeric }, //
      { { 0x16AD0, 0x16AED }, WordBreak::ALetter }, //
      { { 0x16AF0, 0x16AF4 }, WordBreak::Extend }, //
      { { 0x16B00, 0x16B2F }, WordBreak::ALetter }, //
      { { 0x16B30, 0x16B36 }, WordBreak::Extend }, //
      { { 0x16B40, 0x16B43 }, WordBreak::ALetter }, //
      { { 0x16B50, 0x16B59 }, WordBreak::Numeric }, //
      { { 0x16B63, 0x16B77 }, WordBreak::ALetter }, //
      { { 0x16B7D, 0x16B8F }, WordBreak::ALetter }, //
      { { 0x16E40, 0x16E7F }, WordBreak::ALetter }, //
      { { 0x16F00, 0x16F4A }, WordBreak::ALetter }, //
      { { 0x16F4F, 0x16F4F }, WordBreak::Extend }, //
      { { 0x16F50, 0x16F50 }, WordBreak::ALetter }, //
      { { 0x16F51, 0x16F87 }, WordBreak::Extend }, //
      { { 0x16F8F, 0x16F92 }, WordBreak::Extend }, //
      { { 0x16F93, 0x16F9F }, WordBreak::ALetter }, //
      { { 0x16FE0, 0x16FE1 }, WordBreak::ALetter }, //
      { { 0x16FE3, 0x16FE3 }, WordBreak::ALetter }, //
      { { 0x16FE4, 0x16FE4 }, WordBreak::Extend }, //
      { { 0x16FF0, 0x16FF1 }, WordBreak::Extend }, //
      { { 0x1B000, 0x1B000 }, WordBreak::Katakana }, //
      { { 0x1B164, 0x1B167 }, WordBreak::Katakana }, //
      { { 0x1BC00, 0x1BC6A }, WordBreak::ALetter }, //
      { { 0x1BC70, 0x1BC7C }, WordBreak::ALetter }, //
      { { 0x1BC80, 0x1BC88 }, WordBreak::ALetter }, //
      { { 0x1BC90, 0x1BC99 }, WordBreak::ALetter }, //
      { { 0x1BC9D, 0x1BC9E }, WordBreak::Extend }, //
      { { 0x1BCA0, 0x1BCA3 }, WordBreak::Format }, //
      { { 0x1D165, 0x1D166 }, WordBreak::Extend }, //
      { { 0x1D167, 0x1D169 }, WordBreak::Extend }, //
      { { 0x1D16D, 0x1D172 }, WordBreak::Extend }, //
      { { 0x1D173, 0x1D17A }, WordBreak::Format }, //
      { { 0x1D17B, 0x1D182 }, WordBreak::Extend }, //
      { { 0x1D185, 0x1D18B }, WordBreak::Extend }, //
      { { 0x1D1AA, 0x1D1AD }, WordBreak::Extend }, //
      { { 0x1D242, 0x1D244 }, WordBreak::Extend }, //
      { { 0x1D400, 0x1D454 }, WordBreak::ALetter }, //
      { { 0x1D456, 0x1D49C }, WordBreak::ALetter }, //
      { { 0x1D49E, 0x1D49F }, WordBreak::ALetter }, //
      { { 0x1D4A2, 0x1D4A2 }, WordBreak::ALetter }, //
      { { 0x1D4A5, 0x1D4A6 }, WordBreak::ALetter }, //
      { { 0x1D4A9, 0x1D4AC }, WordBreak::ALetter }, //
      { { 0x1D4AE, 0x1D4B9 }, WordBreak::ALetter }, //
      { { 0x1D4BB, 0x1D4BB }, WordBreak::ALetter }, //
      { { 0x1D4BD, 0x1D4C3 }, WordBreak::ALetter }, //
      { { 0x1D4C5, 0x1D505 }, WordBreak::ALetter }, //
      { { 0x1D507, 0x1D50A }, WordBreak::ALetter }, //
      { { 0x1D50D, 0x1D514 }, WordBreak::ALetter }, //
      { { 0x1D516, 0x1D51C }, WordBreak::ALetter }, //
      { { 0x1D51E, 0x1D539 }, WordBreak::ALetter }, //
      { { 0x1D53B, 0x1D53E }, WordBreak::ALetter }, //
      { { 0x1D540, 0x1D544 }, WordBreak::ALetter }, //
      { { 0x1D546, 0x1D546 }, WordBreak::ALetter }, //
      { { 0x1D54A, 0x1D550 }, WordBreak::ALetter }, //
      { { 0x1D552, 0x1D6A5 }, WordBreak::ALetter }, //
      { { 0x1D6A8, 0x1D6C0 }, WordBreak::ALetter }, //
      { { 0x1D6C2, 0x1D6DA }, WordBreak::ALetter }, //
      { { 0x1D6DC, 0x1D6FA }, WordBreak::ALetter }, //
      { { 0x1D6FC, 0x1D714 }, WordBreak::ALetter }, //
      { { 0x1D716, 0x1D734 }, WordBreak::ALetter }, //
      { { 0x1D736, 0x1D74E }, WordBreak::ALetter }, //
      { { 0x1D750, 0x1D76E }, WordBreak::ALetter }, //
      { { 0x1D770, 0x1D788 }, WordBreak::ALetter }, //
      { { 0x1D78A, 0x1D7A8 }, WordBreak::ALetter }, //
      { { 0x1D7AA, 0x1D7C2 }, WordBreak::ALetter }, //
      { { 0x1D7C4, 0x1D7CB }, WordBreak::ALetter }, //
      { { 0x1D7CE, 0x1D7FF }, WordBreak::Numeric }, //
      { { 0x1DA00, 0x1DA36 }, WordBreak::Extend }, //
      { { 0x1DA3B, 0x1DA6C }, WordBreak::Extend }, //
      { { 0x1DA75, 0x1DA75 }, WordBreak::Extend }, //
      { { 0x1DA84, 0x1DA84 }, WordBreak::Extend }, //
      { { 0x1DA9B, 0x1DA9F }, WordBreak::Extend }, //
      { { 0x1DAA1, 0x1DAAF }, WordBreak::Extend }, //
      { { 0x1E000, 0x1E006 }, WordBreak::Extend }, //
      { { 0x1E008, 0x1E018 }, WordBreak::Extend }, //
      { { 0x1E01B, 0x1E021 }, WordBreak::Extend }, //
      { { 0x1E023, 0x1E024 }, WordBreak::Extend }, //
      { { 0x1E026, 0x1E02A }, WordBreak::Extend }, //
      { { 0x1E100, 0x1E12C }, WordBreak::ALetter }, //
      { { 0x1E130, 0x1E136 }, WordBreak::Extend }, //
      { { 0x1E137, 0x1E13D }, WordBreak::ALetter }, //
      { { 0x1E140, 0x1E149 }, WordBreak::Numeric }, //
      { { 0x1E14E, 0x1E14E }, WordBreak::ALetter }, //
      { { 0x1E2C0, 0x1E2EB }, WordBreak::ALetter }, //
      { { 0x1E2EC, 0x1E2EF }, WordBreak::Extend }, //
      { { 0x1E2F0, 0x1E2F9 }, WordBreak::Numeric }, //
      { { 0x1E800, 0x1E8C4 }, WordBreak::ALetter }, //
      { { 0x1E8D0, 0x1E8D6 }, WordBreak::Extend }, //
      { { 0x1E900, 0x1E943 }, WordBreak::ALetter }, //
      { { 0x1E944, 0x1E94A }, WordBreak::Extend }, //
      { { 0x1E94B, 0x1E94B }, WordBreak::ALetter }, //
      { { 0x1E950, 0x1E959 }, WordBreak::Numeric }, //
      { { 0x1EE00, 0x1EE03 }, WordBreak::ALetter }, //
      { { 0x1EE05, 0x1EE1F }, WordBreak::ALetter }, //
      { { 0x1EE21, 0x1EE22 }, WordBreak::ALetter }, //
      { { 0x1EE24, 0x1EE24 }, WordBreak::ALetter }, //
      { { 0x1EE27, 0x1EE27 }, WordBreak::ALetter }, //
      { { 0x1EE29, 0x1EE32 }, WordBreak::ALetter }, //
      { { 0x1EE34, 0x1EE37 }, WordBreak::ALetter }, //
      { { 0x1EE39, 0x1EE39 }, WordBreak::ALetter }, //
      { { 0x1EE3B, 0x1EE3B }, WordBreak::ALetter }, //
      { { 0x1EE42, 0x1EE42 }, WordBreak::ALetter }, //
      { { 0x1EE47, 0x1EE47 }, WordBreak::ALetter }, //
      { { 0x1EE49, 0x1EE49 }, WordBreak::ALetter }, //
      { { 0x1EE4B, 0x1EE4B }, WordBreak::ALetter }, //
      { { 0x1EE4D, 0x1EE4F }, WordBreak::ALetter }, //
      { { 0x1EE51, 0x1EE52 }, WordBreak::ALetter }, //
      { { 0x1EE54, 0x1EE54 }, WordBreak::ALetter }, //
      { { 0x1EE57, 0x1EE57 }, WordBreak::ALetter }, //
      { { 0x1EE59, 0x1EE59 }, WordBreak::ALetter }, //
      { { 0x1EE5B, 0x1EE5B }, WordBreak::ALetter }, //
      { { 0x1EE5D, 0x1EE5D }, WordBreak::ALetter }, //
      { { 0x1EE5F, 0x1EE5F }, WordBreak::ALetter }, //
      { { 0x1EE61, 0x1EE62 }, WordBreak::ALetter }, //
      { { 0x1EE64, 0x1EE64 }, WordBreak::ALetter }, //
      { { 0x1EE67, 0x1EE6A }, WordBreak::ALetter }, //
      { { 0x1EE6C, 0x1EE72 }, WordBreak::ALetter }, //
      { { 0x1EE74, 0x1EE77 }, WordBreak::ALetter }, //
      { { 0x1EE79, 0x1EE7C }, WordBreak::ALetter }, //
      { { 0x1EE7E, 0x1EE7E }, WordBreak::ALetter }, //
      { { 0x1EE80, 0x1EE89 }, WordBreak::ALetter }, //
      { { 0x1EE8B, 0x1EE9B }, WordBreak::ALetter }, //
      { { 0x1EEA1, 0x1EEA3 }, WordBreak::ALetter }, //
      { { 0x1EEA5, 0x1EEA9 }, WordBreak::ALetter }, //
      { { 0x1EEAB, 0x1EEBB }, WordBreak::ALetter }, //
      { { 0x1F130, 0x1F149 }, WordBreak::ALetter }, //
      { { 0x1F150, 0x1F169 }, WordBreak::ALetter }, //
      { { 0x1F170, 0x1F189 }, WordBreak::ALetter }, //
      { { 0x1F1E6, 0x1F1FF }, WordBreak::Regional_Indicator }, //
      { { 0x1F3FB, 0x1F3FF }, WordBreak::Extend }, //
      { { 0x1FBF0, 0x1FBF9 }, WordBreak::Numeric }, //
      { { 0xE0001, 0xE0001 }, WordBreak::Format }, //
      { { 0xE0020, 0xE007F }, WordBreak::Extend }, //
      { { 0xE0100, 0xE01EF }, WordBreak::Extend }, //
    };

constexpr class FullWidthMap {
  std::bitset<UNICODE_CHAR_COUNT> map { };
public:
  constexpr FullWidthMap(std::initializer_list<CodePointInterval> list) {
    for (auto &&interval : list) {
      for (auto ch = interval.from; ch <= interval.to; ++ch) {
        this->map[ch] = true;
      }
    }
  }

  constexpr bool operator[](char32_t cp) const {
    if (cp < std::size(this->map)) {
      return this->map[cp];
    }
    return false;
  }
} full_width_map = //
    // As of Unicode 13.0.0
    { { 0x01100, 0x0115F }, { 0x0231A, 0x0231B }, { 0x02329, 0x0232A }, //
      { 0x023E9, 0x023EC }, { 0x023F0, 0x023F0 }, { 0x023F3, 0x023F3 }, //
      { 0x025FD, 0x025FE }, { 0x02614, 0x02615 }, { 0x02648, 0x02653 }, //
      { 0x0267F, 0x0267F }, { 0x02693, 0x02693 }, { 0x026A1, 0x026A1 }, //
      { 0x026AA, 0x026AB }, { 0x026BD, 0x026BE }, { 0x026C4, 0x026C5 }, //
      { 0x026CE, 0x026CE }, { 0x026D4, 0x026D4 }, { 0x026EA, 0x026EA }, //
      { 0x026F2, 0x026F3 }, { 0x026F5, 0x026F5 }, { 0x026FA, 0x026FA }, //
      { 0x026FD, 0x026FD }, { 0x02705, 0x02705 }, { 0x0270A, 0x0270B }, //
      { 0x02728, 0x02728 }, { 0x0274C, 0x0274C }, { 0x0274E, 0x0274E }, //
      { 0x02753, 0x02755 }, { 0x02757, 0x02757 }, { 0x02795, 0x02797 }, //
      { 0x027B0, 0x027B0 }, { 0x027BF, 0x027BF }, { 0x02B1B, 0x02B1C }, //
      { 0x02B50, 0x02B50 }, { 0x02B55, 0x02B55 }, { 0x02E80, 0x02E99 }, //
      { 0x02E9B, 0x02EF3 }, { 0x02F00, 0x02FD5 }, { 0x02FF0, 0x02FFB }, //
      { 0x03000, 0x0303E }, { 0x03041, 0x03096 }, { 0x03099, 0x030FF }, //
      { 0x03105, 0x0312F }, { 0x03131, 0x0318E }, { 0x03190, 0x031E3 }, //
      { 0x031F0, 0x0321E }, { 0x03220, 0x03247 }, { 0x03250, 0x04DBF }, //
      { 0x04E00, 0x0A48C }, { 0x0A490, 0x0A4C6 }, { 0x0A960, 0x0A97C }, //
      { 0x0AC00, 0x0D7A3 }, { 0x0F900, 0x0FAFF }, { 0x0FE10, 0x0FE19 }, //
      { 0x0FE30, 0x0FE52 }, { 0x0FE54, 0x0FE66 }, { 0x0FE68, 0x0FE6B }, //
      { 0x0FF01, 0x0FF60 }, { 0x0FFE0, 0x0FFE6 }, { 0x16FE0, 0x16FE4 }, //
      { 0x16FF0, 0x16FF1 }, { 0x17000, 0x187F7 }, { 0x18800, 0x18CD5 }, //
      { 0x18D00, 0x18D08 }, { 0x1B000, 0x1B11E }, { 0x1B150, 0x1B152 }, //
      { 0x1B164, 0x1B167 }, { 0x1B170, 0x1B2FB }, { 0x1F004, 0x1F004 }, //
      { 0x1F0CF, 0x1F0CF }, { 0x1F18E, 0x1F18E }, { 0x1F191, 0x1F19A }, //
      { 0x1F200, 0x1F202 }, { 0x1F210, 0x1F23B }, { 0x1F240, 0x1F248 }, //
      { 0x1F250, 0x1F251 }, { 0x1F260, 0x1F265 }, { 0x1F300, 0x1F320 }, //
      { 0x1F32D, 0x1F335 }, { 0x1F337, 0x1F37C }, { 0x1F37E, 0x1F393 }, //
      { 0x1F3A0, 0x1F3CA }, { 0x1F3CF, 0x1F3D3 }, { 0x1F3E0, 0x1F3F0 }, //
      { 0x1F3F4, 0x1F3F4 }, { 0x1F3F8, 0x1F43E }, { 0x1F440, 0x1F440 }, //
      { 0x1F442, 0x1F4FC }, { 0x1F4FF, 0x1F53D }, { 0x1F54B, 0x1F54E }, //
      { 0x1F550, 0x1F567 }, { 0x1F57A, 0x1F57A }, { 0x1F595, 0x1F596 }, //
      { 0x1F5A4, 0x1F5A4 }, { 0x1F5FB, 0x1F64F }, { 0x1F680, 0x1F6C5 }, //
      { 0x1F6CC, 0x1F6CC }, { 0x1F6D0, 0x1F6D2 }, { 0x1F6D5, 0x1F6D7 }, //
      { 0x1F6EB, 0x1F6EC }, { 0x1F6F4, 0x1F6FC }, { 0x1F7E0, 0x1F7EB }, //
      { 0x1F90C, 0x1F93A }, { 0x1F93C, 0x1F945 }, { 0x1F947, 0x1F978 }, //
      { 0x1F97A, 0x1F9CB }, { 0x1F9CD, 0x1F9FF }, { 0x1FA70, 0x1FA74 }, //
      { 0x1FA78, 0x1FA7A }, { 0x1FA80, 0x1FA86 }, { 0x1FA90, 0x1FAA8 }, //
      { 0x1FAB0, 0x1FAB6 }, { 0x1FAC0, 0x1FAC2 }, { 0x1FAD0, 0x1FAD6 }, //
      { 0x20000, 0x2FFFD }, { 0x30000, 0x3FFFD }, };

constexpr bool is_control(char32_t cp) {
  if (cp == 0) {
    return true;
  } else if (cp < 32) {
    return cp != 10;  // 10 => Line feed.
  } else if (cp >= 0x7F and cp < 0xA0) {
    return true;
  }
  return false;
}

constexpr bool is_combining(char32_t cp) {
  return word_break_map[cp] == WordBreak::Extend;
}

constexpr bool is_full_width(char32_t cp) {
  return full_width_map[cp];
}

constexpr int glyph_width(char32_t cp) {
  if (is_control(cp)) {
    return -1;
  } else if (is_combining(cp)) {
    return 0;
  } else if (is_full_width(cp)) {
    return 2;
  } else {
    return 1;
  }
}

}
