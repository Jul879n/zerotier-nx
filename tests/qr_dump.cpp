// Genera un PGM con el mismo trazado que QrGui::Draw (caja 276, módulos floor(276/(size+8))).
#include <cstdio>
#include <vector>
extern "C" {
#include "qrcodegen.h"
}
int main(int, char **argv) {
    uint8_t qr[qrcodegen_BUFFER_LEN_MAX], tmp[qrcodegen_BUFFER_LEN_MAX];
    if (!qrcodegen_encodeText(argv[1], tmp, qr, qrcodegen_Ecc_MEDIUM, qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true)) return 1;
    const int box = 276, size = qrcodegen_getSize(qr), px = box / (size + 8), total = px * (size + 8);
    std::vector<unsigned char> img(box * box, 255);
    const int o = (box - total) / 2 + 4 * px;
    for (int j = 0; j < size; ++j) for (int i = 0; i < size; ++i)
        if (qrcodegen_getModule(qr, i, j))
            for (int y = 0; y < px; ++y) for (int x = 0; x < px; ++x) img[(o + j * px + y) * box + o + i * px + x] = 0;
    std::printf("P5\n%d %d\n255\n", box, box);
    fwrite(img.data(), 1, img.size(), stdout);
    std::fprintf(stderr, "size=%d px=%d\n", size, px);
}
