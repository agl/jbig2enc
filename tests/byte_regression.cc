#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include <leptonica/allheaders.h>

using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;

#include "../src/jbig2enc.h"
#include "../src/jbig2segments.h"
#include "../src/jbig2structs.h"

static void
expect_bytes(const char *name, const std::vector<u8> &actual,
             const std::vector<u8> &expected) {
  if (actual == expected) return;

  std::fprintf(stderr, "%s mismatch\nexpected:", name);
  for (u8 b : expected) std::fprintf(stderr, " %02x", b);
  std::fprintf(stderr, "\nactual:  ");
  for (u8 b : actual) std::fprintf(stderr, " %02x", b);
  std::fprintf(stderr, "\n");
  std::exit(1);
}

static void
test_segment_header() {
  Segment seg;
  seg.number = 5;
  seg.type = segment_imm_text_region;
  seg.retain_bits = 3;
  seg.referred_to.push_back(1);
  seg.referred_to.push_back(2);
  seg.page = 7;
  seg.len = 0x01020304;

  std::vector<u8> out(seg.size());
  seg.write(out.data());

  expect_bytes("segment header", out,
               {0x00, 0x00, 0x00, 0x05, 0x06, 0x43, 0x01, 0x02, 0x07,
                0x01, 0x02, 0x03, 0x04});
}

static void
test_explicit_payload_serializers() {
  // The on-the-wire JBIG2 structs in jbig2structs.h are plain packed PODs
  // with no serialisation helpers, so the test serialises them the same way
  // the encoder does: zero-initialise, set bitfields directly, store the
  // multi-byte fields in network (big-endian) order with htonl/htons, then
  // memcpy the raw struct out.
  auto serialize = [](const auto &s) {
    std::vector<u8> out(sizeof(s));
    std::memcpy(out.data(), &s, sizeof(s));
    return out;
  };

  jbig2_file_header header;
  std::memset(&header, 0, sizeof(header));
  std::memcpy(header.id, JBIG2_FILE_MAGIC, 8);
  header.organisation_type = 1;
  header.n_pages = htonl(1);
  expect_bytes("file header", serialize(header),
               {0x97, 0x4a, 0x42, 0x32, 0x0d, 0x0a, 0x1a, 0x0a, 0x01,
                0x00, 0x00, 0x00, 0x01});

  jbig2_page_info page;
  std::memset(&page, 0, sizeof(page));
  page.width = htonl(2);
  page.height = htonl(3);
  page.xres = htonl(300);
  page.yres = htonl(301);
  page.is_lossless = 1;
  page.contains_refinements = 1;
  expect_bytes("page info", serialize(page),
               {0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x03,
                0x00, 0x00, 0x01, 0x2c, 0x00, 0x00, 0x01, 0x2d,
                0x03, 0x00, 0x00});

  jbig2_generic_region gen;
  std::memset(&gen, 0, sizeof(gen));
  gen.width = htonl(2);
  gen.height = htonl(3);
  gen.tpgdon = 1;
  gen.a1x = 3;
  gen.a1y = -1;
  gen.a2x = -3;
  gen.a2y = -1;
  gen.a3x = 2;
  gen.a3y = -2;
  gen.a4x = -2;
  gen.a4y = -2;
  expect_bytes("generic region", serialize(gen),
               {0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x03,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x08, 0x03, 0xff, 0xfd, 0xff, 0x02, 0xfe,
                0xfe, 0xfe});

  jbig2_symbol_dict sym;
  std::memset(&sym, 0, sizeof(sym));
  sym.a1x = 3;
  sym.a1y = -1;
  sym.a2x = -3;
  sym.a2y = -1;
  sym.a3x = 2;
  sym.a3y = -2;
  sym.a4x = -2;
  sym.a4y = -2;
  sym.exsyms = htonl(4);
  sym.newsyms = htonl(5);
  expect_bytes("symbol dictionary", serialize(sym),
               {0x00, 0x00, 0x03, 0xff, 0xfd, 0xff, 0x02, 0xfe, 0xfe,
                0xfe, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x05});

  jbig2_text_region text;
  std::memset(&text, 0, sizeof(text));
  text.width = htonl(2);
  text.height = htonl(3);
  text.sbrefine = 1;
  text.refcorner = 2;
  expect_bytes("text region", serialize(text),
               {0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x03,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x22});
}

static std::vector<u8>
encode_one_pixel_generic() {
  PIX *pix = pixCreate(1, 1, 1);
  if (!pix) {
    std::fprintf(stderr, "pixCreate failed\n");
    std::exit(1);
  }
  pixSetPadBits(pix, 0);

  int length = 0;
  u8 *encoded = jbig2_encode_generic(pix, true, 0, 0, false, &length);
  pixDestroy(&pix);
  if (!encoded || length <= 0) {
    std::fprintf(stderr, "jbig2_encode_generic failed\n");
    std::exit(1);
  }

  std::vector<u8> out(encoded, encoded + length);
  std::free(encoded);
  return out;
}

static void
test_known_generic_stream() {
  const std::vector<u8> stream = encode_one_pixel_generic();
  const std::vector<u8> expected = {
      0x97, 0x4a, 0x42, 0x32, 0x0d, 0x0a, 0x1a, 0x0a, 0x01,
      0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x30,
      0x00, 0x01, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00,
      0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x01, 0x26, 0x00, 0x01, 0x00, 0x00, 0x00, 0x1d,
      0x00, 0x00,
      0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0xfd,
      0xff, 0x02, 0xfe, 0xfe, 0xfe, 0x7f, 0xff, 0xac, 0x00,
      0x00, 0x00, 0x02, 0x31, 0x00, 0x01, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x03, 0x33, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00};

  expect_bytes("known generic stream", stream, expected);
}

int
main() {
  test_segment_header();
  test_explicit_payload_serializers();
  test_known_generic_stream();
  return 0;
}
