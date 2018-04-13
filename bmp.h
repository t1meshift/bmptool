#ifndef USES_BMP_H
#define USES_BMP_H

#include <stdint.h>
#include <stdio.h>

#define ARR_SIZE(x) (sizeof(x)/sizeof((x)[0]))

#define BITMAPCOREHEADER_SIZE 12
#define BITMAPINFOHEADER_SIZE 40

// TODO: LOTS OF DOCUMENTATION!

typedef enum {
    BI_RGB = 0,
    BI_RLE8 = 1,
    BI_RLE4 = 2,
    BI_BITFIELDS = 3,
    BI_JPEG = 4,
    BI_PNG = 5,
    BI_ALPHABITFIELDS = 6
} BITMAP_COMPRESSION;

/*!
 * @brief BMPFILEHEADER according to Microsoft's BMP specification. Contains main information about file.
 */
typedef struct {
    uint16_t bfType; /**< BMP signature (usually 0x424D in big-endian or 0x4D42 in little-endian which means BM). */
    uint32_t bfSize; /**< BMP file size in bytes. */
    uint16_t bfReserved1, bfReserved2; /**< Reserved fields. Usually should be 0. */
    uint32_t bfOffBits; /**< Bitmap data offset in bytes. */
} BITMAPFILEHEADER;

/*!
 * @brief CORE version of BITMAPINFO. Is 12 bytes long (bcSize = 12).
 */
typedef struct {
    uint32_t bcSize; /**< Size of header in bytes (should be 12). */
    uint16_t bcWidth; /**< Width of bitmap. */
    uint16_t bcHeight; /**< Height of bitmap. */
    uint16_t bcPlanes; /**< Color planes. The value of this field must be 1 in BMP files. */
    uint16_t bcBitCount; /**< Bits-per-pixel count. Allowed values are 1, 2, 4, 8, 24. */
} BITMAPCOREHEADER;

/*!
 * @brief Version 3 of BITMAPINFO. Is 40 bytes long (biSize = 40)
 */
typedef struct {
    uint32_t biSize; /**< Size of header in bytes (should be 40). */
    int32_t biWidth; /**< Width of bitmap. */
    int32_t biHeight; /**< Height of bitmap. */
    uint16_t biPlanes; /**< Color planes. The value of this field must be 1 in BMP files. */
    uint16_t biBitCount; /**< Bits-per-pixel count. Allowed values are 1, 2, 4, 8, 16, 24, 32, 48, 64. */
    uint32_t biCompression; /**< Compression used for bitmap data. Allowed values are stored in BITMAP_COMPRESSION enum. */
    uint32_t biSizeImage; /**< Size of bitmap data. Can be 0 if BITMAP_COMPRESSION is 0, 3 or 6. */
    int32_t biXPelsPerMeter; /**< Pixels-per-meter (horizontal). */
    int32_t biYPelsPerMeter; /**< Pixels-per-meter (vertical). */
    uint32_t biClrUsed; /**< Number of colors used in palette. */
    uint32_t biClrImportant; /**< Number of colors used in bitmap data (from the start of palette). */
} BITMAPINFOHEADER;

/*!
 * @brief A struct for color representation in BGRA format.
 */
typedef struct {
    uint8_t b; /**< A blue channel. */
    uint8_t g; /**< A green channel. */
    uint8_t r; /**< A red channel. */
    uint8_t reserved; /**< A reserved channel (sometimes it's used for alpha). */
} RGB_COLOR;

/*!
 * @brief A union for color representation.
 */
typedef union {
    RGB_COLOR rgb; /**< A BGRA representation of color. */
    uint32_t value; /**< An integer representation of color. */
} BITMAP_COLOR;


typedef struct noise_shader_data {
    uint8_t level;
    uint8_t alpha;
} NOISE_SHADER_DATA;


/*!
 * @brief An alias for palette (array of color).
 */
typedef BITMAP_COLOR *BITMAP_COLOR_PALETTE;

/*!
 * @brief A bitmap structure. Stores bitmap width, height and pixel data.
 */
typedef struct {
    uint32_t width; /**< Bitmap width. */
    uint32_t height; /**< Bitmap height. */
    BITMAP_COLOR *data; /**< Raw pixel data. */
} BITMAP;

typedef BITMAP_COLOR (*BITMAP_SHADER)(BITMAP_COLOR *pixels, uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                                      void *shader_args);

/*
 *  Utilitarian functions for reading from file.
 */

/*!
 * @brief Reads unsigned 8bit value from file.
 * @param File
 * @return A value read from file.
 */
uint8_t read_uint8(FILE *f);

/*!
 * @brief Reads unsigned 16bit value from file (little-endian).
 * @param File
 * @return A value read from file.
 */
uint16_t read_uint16(FILE *f);

/*!
 * @brief Reads unsigned 32bit value from file (little-endian).
 * @param File
 * @return A value read from file.
 */
uint32_t read_uint32(FILE *f);

void write_uint8(FILE *f, uint8_t value);

void write_uint16(FILE *f, uint16_t value);

void write_uint32(FILE *f, uint32_t value);


/*
 * Main BMP file worker functions.
 */

/*!
 * @brief Reads a bitmap from file.
 * @param A file path to read from
 * @return A pointer to BITMAP, NULL in case of errors.
 */
BITMAP *bmp_read(const char *file);

int bmp_write(BITMAP *bitmap, const char *file); //TODO documentation

uint8_t bmp_channel_blend(uint8_t a, uint8_t b, uint8_t alpha);

BITMAP_COLOR bmp_color_blend(BITMAP_COLOR a, BITMAP_COLOR b, uint8_t alpha);

BITMAP *bmp_combine(BITMAP *background, BITMAP *foreground, uint8_t alpha);

BITMAP *bmp_grayscale(BITMAP *src);

BITMAP *bmp_apply_shader(BITMAP *src, BITMAP_SHADER shader, void *shader_args);

BITMAP_COLOR bmp_shader_grayscale(BITMAP_COLOR *pixels, uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                                  void *shader_args);

BITMAP_COLOR bmp_shader_noise(BITMAP_COLOR *pixels, uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                              void *shader_args);

#endif //USES_BMP_H
