#ifndef _TYPE_H
#define _TYPE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TypeFont TypeFont;

TypeFont* type_load_font(const char* name);
void type_destroy_font(TypeFont *font);

int type_text_width(TypeFont* font, const char* text);

int type_draw_text(TypeFont* font, const char* text);
int type_draw_char(TypeFont* font, unsigned int ch);

int type_font_height(TypeFont *font);
int type_font_ascent(TypeFont *font);
int type_font_descent(TypeFont *font);

#ifdef __cplusplus
}
#endif

#endif
