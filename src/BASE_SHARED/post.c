#include "post.h"

uint8_t  postLine;
uint16_t postColor;
uint16_t postBg;

void postInit() {
  postLine = 0;
  postColor = 0;
  postBg = LCD_MixColor(230, 230, 230);
  LCD_Fill(LCD_MixColor(10, 10, 10));
  LCD_setDrawArea(0, 0, 319, 479);
  postMessage((uint8_t *)"SDA-OS v."SDA_OS_VERSION" on SDA Wonder");

  postColor = 0xFFFF;
  postBg = LCD_MixColor(10, 10, 10);
}

void postMessage(uint8_t *message) {
  postLine++;
  LCD_FillRect(0, 32*(postLine) - 30, 319, 32*(postLine + 1) - 30, postBg);
  LCD_DrawText_ext(10, 32*postLine - 20, postColor, message);
}

void postError(uint8_t *message) {
  uint16_t bg = postBg;
  postBg = LCD_MixColor(255, 0, 0);
  postMessage(message);
  postBg = bg;
}

void postSuccess(uint8_t *message) {
  uint16_t bg = postBg;
  postBg = LCD_MixColor(0, 230, 0);
  postMessage(message);
  postBg = bg;
}