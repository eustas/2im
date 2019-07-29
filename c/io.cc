#include "io.h"

#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "image.h"
#include "platform.h"
#include "png.h"

namespace twim {

void closeFile(FILE* f) {
  if (f) fclose(f);
}

std::unique_ptr<FILE, void (*)(FILE*)> openFile(const std::string& path,
                                                const char* mode) {
  return {fopen(path.c_str(), mode), closeFile};
}

std::unique_ptr<std::vector<uint8_t>> Io::readFile(const std::string& path) {
  std::unique_ptr<std::vector<uint8_t>> result(new std::vector<uint8_t>());
  auto f = openFile(path, "rb");
  if (!f) return result;

  std::vector<uint8_t> buffer(16384);
  while (!feof(f.get())) {
    size_t read_bytes =
        fread(buffer.data(), sizeof(char), buffer.size(), f.get());
    if (ferror(f.get())) {
      result->clear();
      return result;
    }
    result->insert(result->end(), buffer.data(), buffer.data() + read_bytes);
  }

  return result;
}

bool Io::writeFile(const std::string& path, const std::vector<uint8_t>& data) {
  auto f = openFile(path, "wb");
  if (!f) return false;
  size_t written = fwrite(data.data(), 1, data.size(), f.get());
  return written == data.size();
}

struct Png {
  png_structp png_ptr = nullptr;
  png_infop info_ptr = nullptr;
};

void destroyPngReadStruct(Png* p) {
  if (!p) return;
  png_destroy_read_struct(&p->png_ptr, &p->info_ptr, nullptr);
  delete p;
}

void destroyPngWriteStruct(Png* p) {
  if (!p) return;
  png_destroy_write_struct(&p->png_ptr, &p->info_ptr);
  delete p;
}

std::unique_ptr<Png, void (*)(Png*)> createPngStruct(bool read) {
  Png* png = new Png();
  auto destroy = read ? destroyPngReadStruct : destroyPngWriteStruct;
  png->png_ptr = read ? png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr,
                                               nullptr, nullptr)
                      : png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr,
                                                nullptr, nullptr);
  if (!png->png_ptr) {
    png = nullptr;
  } else {
    png->info_ptr = png_create_info_struct(png->png_ptr);
    if (!png->info_ptr) {
      destroy(png);
      png = nullptr;
    }
  }
  return {png, destroy};
}

struct Stream {
  std::vector<uint8_t>* data;
  size_t pos;
};

void pngReadStream(png_structp png_ptr, png_bytep out, png_size_t count) {
  Stream* stream = reinterpret_cast<Stream*>(png_get_io_ptr(png_ptr));
  if (stream->pos + count > stream->data->size()) {
    png_error(png_ptr, "unexpected end of data");
  }
  memcpy(out, stream->data->data() + stream->pos, count);
  stream->pos += count;
}

void pngWriteStream(png_structp png_ptr, png_bytep in, png_size_t count) {
  Stream* stream = reinterpret_cast<Stream*>(png_get_io_ptr(png_ptr));
  stream->data->insert(stream->data->end(), in, in + count);
}

void pngFlushStream(png_structp png_ptr) {}

static const uint8_t kPngHdr[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};

Image Io::readPng(const std::string& path) {
  Image result;

  auto data = readFile(path);
  if (data->size() < 8) return result;
  Stream input = {data.get(), 0};

  if (memcmp(data->data(), kPngHdr, sizeof(kPngHdr)) != 0) return result;

  auto png = createPngStruct(/* read */ true);
  if (!png) return result;

  if (setjmp(png_jmpbuf(png->png_ptr)) != 0) {
    // Burn in hell, authors of linpng API.
    return result;
  }

  png_set_read_fn(png->png_ptr, &input, pngReadStream);
  constexpr const uint32_t kTransforms = PNG_TRANSFORM_STRIP_ALPHA |
      PNG_TRANSFORM_PACKING | PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_STRIP_16;
  png_read_png(png->png_ptr, png->info_ptr, kTransforms, nullptr);

  const png_bytep* row_pointers = png_get_rows(png->png_ptr, png->info_ptr);
  const size_t width = png_get_image_width(png->png_ptr, png->info_ptr);
  const size_t height = png_get_image_height(png->png_ptr, png->info_ptr);
  const int32_t components = png_get_channels(png->png_ptr, png->info_ptr);

  // Only RGB is supported.
  if (components != 3) return result;

  result.r.resize(width * height);
  result.g.resize(width * height);
  result.b.resize(width * height);

  for (size_t y = 0; y < height; ++y) {
    const uint8_t* from = row_pointers[y];
    uint8_t* to_r = &result.r[width * y];
    uint8_t* to_g = &result.g[width * y];
    uint8_t* to_b = &result.b[width * y];
    for (size_t x = 0; x < width; ++x) {
      to_r[x] = from[3 * x];
      to_g[x] = from[3 * x + 1];
      to_b[x] = from[3 * x + 2];
    }
  }

  result.width = width;
  result.height = height;

  return result;
}

bool Io::writePng(const std::string& path, const Image& img) {
  auto png = createPngStruct(/* read */ false);
  if (!png) return false;

  std::vector<uint8_t> data;
  Stream output = {&data, 0};
  const size_t width = img.width;
  std::vector<png_byte> row(width * 3);

  if (setjmp(png_jmpbuf(png->png_ptr)) != 0) {
    // Burn in hell, authors of linpng API.
    return false;
  }

  png_set_write_fn(png->png_ptr, &output, pngWriteStream, pngFlushStream);

  png_set_IHDR(png->png_ptr, png->info_ptr, width, img.height, 8,
               PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
  png_write_info(png->png_ptr, png->info_ptr);

  uint8_t* RESTRICT rgb = row.data();
  for (size_t y = 0; y < img.height; y++) {
    const uint8_t* RESTRICT r = img.r.data() + width * y;
    const uint8_t* RESTRICT g = img.g.data() + width * y;
    const uint8_t* RESTRICT b = img.b.data() + width * y;
    for (size_t x = 0; x < width; ++x) {
      rgb[3 * x + 2] = r[x];
      rgb[3 * x + 1] = g[x];
      rgb[3 * x] = b[x];
    }
    png_write_row(png->png_ptr, rgb);
  }

  png_write_end(png->png_ptr, nullptr);

  return writeFile(path, data);
}

}  // namespace twim
