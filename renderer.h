#define WIDTH 400
#define HEIGHT 300
#define WINDOW_TITLE "Doom Fire PSX"
#define SCALE 2

typedef unsigned char byte;

extern unsigned char pal_idx[HEIGHT][WIDTH];

void initSystem();
void startLoop(void (*renderFrame)(void));
void setPalette(unsigned char srcPal[][3], int len);
