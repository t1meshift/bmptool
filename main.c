#include <stdio.h>
#include <stdint.h>
#include "bmp.h"

BITMAP *a;
BITMAP *b;
BITMAP *blend;

int main(int argc, char **argv) {
    a = bmp_read("bg.bmp");
    b = bmp_read("fg.bmp");

    a = bmp_grayscale(a);
    blend = bmp_combine(a, b, 119);

    NOISE_SHADER_DATA noise_args = {
            77, // noise level (0-100)
            140 // noise alpha (0-255)
    };

    blend = bmp_apply_shader(blend, bmp_shader_noise, &noise_args);
    bmp_write(blend, "tlen.bmp");
    return 0;
}


