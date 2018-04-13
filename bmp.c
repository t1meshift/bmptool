#include <malloc.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "bmp.h"

#define MIN(a, b) ((a)<(b)?(a):(b))

BITMAP *bmp_read(const char *file) {
    FILE *bmp_file;
    BITMAP *bitmap;

    BITMAPFILEHEADER file_header;
    uint32_t bitmapinfo_size;
    BITMAPCOREHEADER *bitmapinfo_core = NULL;
    BITMAPINFOHEADER *bitmapinfo_v3 = NULL;
    //TODO: BITMAPINFO v4, v5

    uint16_t bitmap_depth;
    BITMAP_COLOR_PALETTE palette = NULL;

    // Try to open file for binary reading
    if ((bmp_file = fopen(file, "rb")) == NULL) {
        return NULL;
    }

    // Reading BITMAPFILEHEADER
    file_header.bfType = read_uint16(bmp_file);
    if (file_header.bfType != 0x4D42) {
        // BM support only
        fclose(bmp_file);
        return NULL;
    }
    file_header.bfSize = read_uint32(bmp_file);
    file_header.bfReserved1 = read_uint16(bmp_file);
    file_header.bfReserved2 = read_uint16(bmp_file);
    file_header.bfOffBits = read_uint32(bmp_file);

    // Reading BITMAPINFO's size to detect its own version
    bitmapinfo_size = read_uint32(bmp_file);
    if ((bitmap = calloc(1, sizeof(BITMAP))) == NULL) {
        // Memory hasn't been allocated
        fclose(bmp_file);
        return NULL;
    }

    // Reading bitmap info header
    switch (bitmapinfo_size) {
        case BITMAPCOREHEADER_SIZE:
            // CORE version
            if ((bitmapinfo_core = calloc(1, sizeof(BITMAPCOREHEADER))) == NULL) {
                free(bitmap);
                fclose(bmp_file);
                return NULL;
            };
            bitmapinfo_core->bcSize = bitmapinfo_size;
            bitmapinfo_core->bcWidth = read_uint16(bmp_file);
            bitmapinfo_core->bcHeight = read_uint16(bmp_file);
            bitmapinfo_core->bcPlanes = read_uint16(bmp_file);
            bitmapinfo_core->bcBitCount = read_uint16(bmp_file);

            if (bitmapinfo_core->bcBitCount >= 8) {
                // Reading color palette
                uint32_t palette_size = (uint32_t) 1 << (bitmapinfo_core->bcBitCount); // 2^BitCount
                if ((palette = calloc(palette_size, sizeof(BITMAP_COLOR))) == NULL) {
                    free(bitmapinfo_core);
                    free(bitmap);
                    fclose(bmp_file);
                    return NULL;
                }
                for (uint32_t i = 0; i < palette_size; ++i) {
                    palette[i].value = read_uint32(bmp_file);
                }
            }
            bitmap->width = bitmapinfo_core->bcWidth;
            bitmap->height = bitmapinfo_core->bcHeight;
            bitmap_depth = bitmapinfo_core->bcBitCount;

            free(bitmapinfo_core);
            break;
        case BITMAPINFOHEADER_SIZE:
            // v3 version
            if ((bitmapinfo_v3 = calloc(1, sizeof(BITMAPINFOHEADER))) == NULL) {
                free(bitmap);
                fclose(bmp_file);
                return NULL;
            }
            bitmapinfo_v3->biSize = bitmapinfo_size;
            bitmapinfo_v3->biWidth = read_uint32(bmp_file);
            bitmapinfo_v3->biHeight = read_uint32(bmp_file);
            bitmapinfo_v3->biPlanes = read_uint16(bmp_file);
            bitmapinfo_v3->biBitCount = read_uint16(bmp_file);
            bitmapinfo_v3->biCompression = read_uint32(bmp_file);
            bitmapinfo_v3->biSizeImage = read_uint32(bmp_file);
            bitmapinfo_v3->biXPelsPerMeter = read_uint32(bmp_file);
            bitmapinfo_v3->biYPelsPerMeter = read_uint32(bmp_file);
            bitmapinfo_v3->biClrUsed = read_uint32(bmp_file);
            bitmapinfo_v3->biClrImportant = read_uint32(bmp_file);

            if (bitmapinfo_v3->biBitCount <= 8) {
                // Reading color palette
                uint32_t palette_size = bitmapinfo_v3->biClrUsed;
                if (palette_size == 0) {
                    palette_size = (uint32_t) 1 << (bitmapinfo_v3->biBitCount); // 2^BitCount
                }
                if ((palette = calloc(palette_size, sizeof(BITMAP_COLOR))) == NULL) {
                    free(bitmapinfo_v3);
                    free(bitmap);
                    fclose(bmp_file);
                    return NULL;
                }
                for (uint32_t i = 0; i < palette_size; ++i) {
                    palette[i].value = read_uint32(bmp_file);
                }
            }

            bitmap->width = bitmapinfo_v3->biWidth;
            bitmap->height = bitmapinfo_v3->biHeight;
            bitmap_depth = bitmapinfo_v3->biBitCount;

            free(bitmapinfo_v3);
            break;
        default:
            // Non-standart BITMAPINFO header
            free(bitmap);
            fclose(bmp_file);
            return NULL;
    }
    // Seeking to bitmap data
    fseek(bmp_file, file_header.bfOffBits, SEEK_SET);
    // Allocating an array for bitmap data
    if ((bitmap->data = calloc(bitmap->width * bitmap->height, sizeof(BITMAP_COLOR))) == NULL) {
        if (palette != NULL) {
            free(palette);
        }
        free(bitmap);
        fclose(bmp_file);
        return NULL;
    }

    // Reading "scans" as they are, bottom-to-top, left-to-right
    // TODO Microsoft says that in case of negative height the data are being read top-to-bottom
    for (int32_t y = bitmap->height - 1; y >= 0; --y) {
        uint32_t bytes_read = 0;
        for (int32_t x = 0; x < bitmap->width; ++x) {
            uint8_t pixels;
            BITMAP_COLOR color;
            switch (bitmap_depth) {
                case 1:
                case 2:
                case 4:
                    // Unpacking pixels from byte
                    pixels = read_uint8(bmp_file);
                    ++bytes_read;
                    int16_t offset = 8 - bitmap_depth;
                    uint16_t bit_mask = 0xFF >> offset;
                    do {
                        uint16_t color_index = (pixels >> offset) & bit_mask;
                        bitmap->data[y * bitmap->width + (x++)] = palette[color_index];
                        offset -= bitmap_depth;
                    } while ((x < bitmap->width) && (offset >= 0));
                    break;
                case 8:
                    pixels = read_uint8(bmp_file);
                    ++bytes_read;
                    bitmap->data[y * bitmap->width + x] = palette[pixels];
                    break;
                case 24:
                    color.rgb.b = read_uint8(bmp_file);
                    color.rgb.g = read_uint8(bmp_file);
                    color.rgb.r = read_uint8(bmp_file);
                    bytes_read += 3;
                    bitmap->data[y * bitmap->width + x] = color;
                    break;
                case 16:
                case 32:
                case 48:
                case 64:
                default:
                    // I don't support those depths for now.
                    //TODO!!!
                    break;
            }
        }
        // Skipping "scan" padding.
        // BMP's image row size must be divisible by 4 so usually there are a few odd bytes.
        while (bytes_read % 4) {
            read_uint8(bmp_file);
            ++bytes_read;
        }
    }
    fclose(bmp_file);
    if (palette != NULL) {
        free(palette);
    }
    return bitmap;
}

int bmp_write(BITMAP *bitmap, const char *file) {
    // Save to 24-bit BMP file with v3 header.
    if (bitmap == NULL) {
        return 1; //TODO error codes
    }
    FILE *bmp_file;
    if ((bmp_file = fopen(file, "wb")) == NULL) {
        return 2; //TODO error codes
    };
    BITMAPFILEHEADER file_header;
    BITMAPINFOHEADER info_header;
    uint32_t bitmapdata_size = ((bitmap->width % 4) ?
                                (bitmap->width + (4 - (bitmap->width % 4))) :
                                (bitmap->width))
                               * bitmap->height * 3;

    file_header.bfType = 0x4D42;
    file_header.bfSize = 14 + BITMAPINFOHEADER_SIZE + bitmapdata_size;
    file_header.bfReserved1 = 0;
    file_header.bfReserved2 = 0;
    file_header.bfOffBits = 14 + BITMAPINFOHEADER_SIZE;

    info_header.biSize = BITMAPINFOHEADER_SIZE;
    info_header.biWidth = bitmap->width;
    info_header.biHeight = bitmap->height;
    info_header.biPlanes = 1;
    info_header.biBitCount = 24;
    info_header.biCompression = 0;
    info_header.biSizeImage = 0;
    info_header.biXPelsPerMeter = 0;
    info_header.biYPelsPerMeter = 0;
    info_header.biClrUsed = 0;
    info_header.biClrImportant = 0;

    write_uint16(bmp_file, file_header.bfType);
    write_uint32(bmp_file, file_header.bfSize);
    write_uint16(bmp_file, file_header.bfReserved1);
    write_uint16(bmp_file, file_header.bfReserved2);
    write_uint32(bmp_file, file_header.bfOffBits);

    write_uint32(bmp_file, info_header.biSize);
    write_uint32(bmp_file, info_header.biWidth);
    write_uint32(bmp_file, info_header.biHeight);
    write_uint16(bmp_file, info_header.biPlanes);
    write_uint16(bmp_file, info_header.biBitCount);
    write_uint32(bmp_file, info_header.biCompression);
    write_uint32(bmp_file, info_header.biSizeImage);
    write_uint32(bmp_file, info_header.biXPelsPerMeter);
    write_uint32(bmp_file, info_header.biYPelsPerMeter);
    write_uint32(bmp_file, info_header.biClrUsed);
    write_uint32(bmp_file, info_header.biClrImportant);


    for (int32_t y = bitmap->height - 1; y >= 0; --y) {
        uint32_t bytes_written = 0;
        for (int32_t x = 0; x < bitmap->width; ++x) {
            write_uint8(bmp_file, bitmap->data[y * bitmap->width + x].rgb.b);
            write_uint8(bmp_file, bitmap->data[y * bitmap->width + x].rgb.g);
            write_uint8(bmp_file, bitmap->data[y * bitmap->width + x].rgb.r);
            bytes_written += 3;
        }
        // Skipping "scan" padding.
        // BMP's image row size must be divisible by 4 so usually there are a few odd bytes.
        while (bytes_written % 4) {
            write_uint8(bmp_file, 0);
            ++bytes_written;
        }
    }
    fclose(bmp_file);
    return 0;
}

uint8_t bmp_channel_blend(uint8_t a, uint8_t b, uint8_t alpha) {
    return (uint8_t) round(MIN(a, b) + abs(a - b) * (1.0 * alpha / 255.0));
}

BITMAP_COLOR bmp_color_blend(BITMAP_COLOR a, BITMAP_COLOR b, uint8_t alpha) {
    BITMAP_COLOR dst;
    dst.rgb.r = bmp_channel_blend(a.rgb.r, b.rgb.r, alpha);
    dst.rgb.g = bmp_channel_blend(a.rgb.g, b.rgb.g, alpha);
    dst.rgb.b = bmp_channel_blend(a.rgb.b, b.rgb.b, alpha);
    return dst;
}

BITMAP *bmp_combine(BITMAP *background, BITMAP *foreground, uint8_t alpha) {
    if (background == NULL || foreground == NULL) {
        return NULL;
    }
    if (background->width != foreground->width || background->height != foreground->height) {
        return NULL;
    }
    BITMAP *result = calloc(1, sizeof(BITMAP));
    result->width = background->width;
    result->height = background->height;
    result->data = calloc(result->width * result->height, sizeof(BITMAP_COLOR));
    for (int32_t y = 0; y < background->height; ++y) {
        for (int32_t x = 0; x < background->width; ++x) {
            BITMAP_COLOR a = background->data[y * result->width + x];
            BITMAP_COLOR b = foreground->data[y * result->width + x];
            result->data[y * result->width + x] = bmp_color_blend(a, b, alpha);
        }
    }
    return result;
}

BITMAP *bmp_grayscale(BITMAP *src) {
    return bmp_apply_shader(src, bmp_shader_grayscale, NULL);
}

BITMAP *bmp_apply_shader(BITMAP *src, BITMAP_SHADER shader, void *shader_args) {
    if (src == NULL) {
        return NULL;
    }
    BITMAP *result = calloc(1, sizeof(BITMAP));
    result->width = src->width;
    result->height = src->height;
    result->data = calloc(result->width * result->height, sizeof(BITMAP_COLOR));
    memcpy(result->data, src->data, result->width * result->height * sizeof(BITMAP_COLOR));

    for (uint32_t y = 0; y < src->height; ++y) {
        for (uint32_t x = 0; x < src->width; ++x) {
            result->data[y * result->width + x] = shader(result->data, x, y, result->width, result->height,
                                                         shader_args);
        }
    }
    return result;
}

BITMAP_COLOR bmp_shader_grayscale(BITMAP_COLOR *pixels, uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                                  void *shader_args) {
    BITMAP_COLOR dst = pixels[y * w + x];
    uint16_t channel_sum = dst.rgb.r + dst.rgb.g + dst.rgb.b;
    uint8_t grayscale = (uint8_t) (channel_sum / 3);
    dst.rgb.r = grayscale;
    dst.rgb.g = grayscale;
    dst.rgb.b = grayscale;
    return dst;
}

BITMAP_COLOR bmp_shader_noise(BITMAP_COLOR *pixels, uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                              void *shader_args) {
    BITMAP_COLOR dst = pixels[y * w + x];
    BITMAP_COLOR noise;

    NOISE_SHADER_DATA *data = shader_args;
    uint8_t noise_level = data->level;
    uint8_t noise_alpha = data->alpha;

    if (noise_level < 0)
        noise_level = 0;
    if (noise_level > 100)
        noise_level = 100;

    if (rand() % 100 < noise_level) {
        noise.rgb.r = (uint8_t) (rand() % 256);
        noise.rgb.g = (uint8_t) (rand() % 256);
        noise.rgb.b = (uint8_t) (rand() % 256);
        dst = bmp_color_blend(dst, noise, (uint8_t) noise_alpha);
    }
    return dst;
}

// Reading from file functions
uint8_t read_uint8(FILE *f) {
    return (uint8_t) fgetc(f);
}

uint16_t read_uint16(FILE *f) {
    return (uint16_t) (fgetc(f) | (fgetc(f) << 8));
}

uint32_t read_uint32(FILE *f) {
    return (uint32_t) (fgetc(f) | (fgetc(f) << 8) | (fgetc(f) << 16) | (fgetc(f) << 24));
}

// Writing to file functions
void write_uint8(FILE *f, uint8_t value) {
    fputc(value, f);
}

void write_uint16(FILE *f, uint16_t value) {
    fputc(value & 0xFF, f);
    fputc(value >> 8, f);
}

void write_uint32(FILE *f, uint32_t value) {
    fputc(value & 0xFF, f);
    fputc((value >> 8) & 0xFF, f);
    fputc((value >> 16) & 0xFF, f);
    fputc(value >> 24, f);
}

