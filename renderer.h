#define WIDTH 400
#define HEIGHT 400

typedef unsigned char byte;

extern unsigned char pal_idx[WIDTH][HEIGHT];

void initSystem();
void startLoop(void (*renderFrame)(void));
void setPalette(unsigned char srcPal[][3], int len);
