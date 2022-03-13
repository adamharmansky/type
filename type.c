#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/ftglyph.h>
#include <freetype/ftoutln.h>
#include <freetype/fttrigon.h>
#include <fontconfig/fontconfig.h>
#include <pthread.h>
#include <GLFW/glfw3.h>

#include "type.h"
#include "utf8.h"

static FT_Library freetype_library;
static FcConfig *fontconfig;
static int DPI = 96;

/* we need thread safety */
static volatile unsigned char _type_initialized = 0;
static pthread_mutex_t library_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t loading_mutex = PTHREAD_MUTEX_INITIALIZER;

static inline void die(const char *text) {
    fputs(text, stderr);
    exit(1);
}

typedef struct Glyph {
    unsigned int texture;
    int x, y, w, h;
    int x_advance, y_advance;
    GLuint list_id;
} Glyph;

struct TypeFont {
    int height;
    int ascent;
    int descent;
    FT_Face face;
    Glyph* blocks[256];
    FcPattern *pattern;
    char *file;
};

static inline int
next_p2(int x) {
    int y = 1;
    while (y<x) y<<=1;
    return y;
}

static void
draw_glyph_template(Glyph glyph) {
    float ox, oy;
    int voffset;
    int w, h;

    w = next_p2(glyph.w);
    h = next_p2(glyph.h);

    voffset=glyph.h-glyph.y;
    ox = (float)glyph.w / (float)w;
    oy = (float)glyph.h / (float)h;
    glBindTexture(GL_TEXTURE_2D, glyph.texture);
        glBegin(GL_QUADS);
            glTexCoord2d(0 ,0); glVertex2f(glyph.x,voffset-glyph.h);
            glTexCoord2d(0 ,oy); glVertex2f(glyph.x,voffset);
            glTexCoord2d(ox,oy); glVertex2f(glyph.x+glyph.w,voffset);
            glTexCoord2d(ox,0); glVertex2f(glyph.x+glyph.w,voffset-glyph.h);
        glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);
    glTranslatef(glyph.x_advance, 0, 0);
}


static void
load_char(TypeFont* font, unsigned int index) {
    FT_Glyph glyph;
    FT_Bitmap* bitmap;
    int w, h;
    GLubyte* data;
    unsigned int i, j;
    Glyph* g;
    GLuint list;

    if (FT_Load_Glyph(font->face, FT_Get_Char_Index(font->face, index), FT_LOAD_DEFAULT))
        die("Cannot load glyph");
    if (FT_Get_Glyph(font->face->glyph, &glyph))
        die("Cannot get glyph");
    FT_Glyph_To_Bitmap(&glyph, ft_render_mode_normal, 0, 1);

    bitmap = &(((FT_BitmapGlyph)glyph)->bitmap);

    w = next_p2(bitmap->width);
    h = next_p2(bitmap->rows);

    data = malloc(2*w*h*sizeof(GLubyte));

    for (i = 0; i < h; i++) {
        for (j = 0; j < w; j++) {
            /* the luminance is always 255 */
            data[2*(j+i*w)] = 255;
            if (i >= bitmap->rows || j >= bitmap->width) {
                data[2*(j+i*w)+1] = 0;
            } else {
                data[2*(j+i*w)+1] = bitmap->buffer[j+bitmap->width*i];
            }
        }
    }

    g = &font->blocks[index>>8][index&0xff];
    glGenTextures(1, &g->texture);
    glBindTexture(GL_TEXTURE_2D, g->texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);

    g->x = ((FT_BitmapGlyph)glyph)->left;
    g->y = ((FT_BitmapGlyph)glyph)->top;
    g->w = bitmap->width;
    g->h = bitmap->rows;
    g->x_advance = font->face->glyph->advance.x >> 6;
    g->y_advance = font->face->glyph->advance.y >> 6;

    if (g->y > font->ascent) {
        font->ascent = g->y;
    }
    if (g->h-g->y > font->descent) {
        font->descent = g->h-g->y;
    }

    list = g->list_id = glGenLists(1);

    glNewList(list, GL_COMPILE);
        draw_glyph_template(*g);
    glEndList();

    free(data);
}

static void
load_block(TypeFont* font, unsigned int block) {
    unsigned int i;

    pthread_mutex_lock(&library_mutex);
#ifdef DEBUG
    fprintf(stderr, "\x1b[33mLoading font block \x1b[31m%d\x1b[0m\n", (signed)block);
#endif

    font->blocks[block] = malloc(sizeof(Glyph)*256);
    for (i = 0; i < 256; i++) {
        load_char(font, (block<<8) + i);
    }
    pthread_mutex_unlock(&library_mutex);
}

static Glyph
get_glyph(TypeFont* font, unsigned int index) {
    unsigned int bi = index >> 8;
    unsigned int ci = index & 0xff;

    if (!font->blocks[bi]) {
        load_block(font, bi);
    }
    return font->blocks[bi][ci];
}

static void
draw_glyph(Glyph g) {
    glCallList(g.list_id);
}

static void
get_font_config(TypeFont *font, const char *name) {
    FcResult result;
    FcPattern *pattern = FcNameParse((const FcChar8*)name);
    FcConfigSubstitute(fontconfig, pattern, FcMatchPattern);
    FcPatternAddDouble(pattern, FC_DPI, DPI);
    FcDefaultSubstitute(pattern);
    font->pattern = FcFontMatch(fontconfig, pattern, &result);
    if (!font->pattern) {
        fprintf(stderr, "Cannot find font \x1b[1m%s\x1b[0m\n", name);
        exit(1);
    }
    font->file = NULL;
    if (FcPatternGetString(font->pattern, FC_FILE, 0, (FcChar8**)&font->file) != FcResultMatch) {
        fprintf(stderr, "Cannot find font file for font \x1b[1m%s\x1b[0m\n", name);
        exit(1);
    }
    if (FcPatternGetInteger(font->pattern, FC_PIXEL_SIZE, 0, &font->height) != FcResultMatch) {
        fprintf(stderr, "Cannot find font size for font \x1b[1m%s\x1b[0m\n", name);
        exit(1);
    }
    FcPatternDestroy(pattern);
}

static void
type_init() {
#ifdef DEBUG
    fprintf(stderr, "\x1b[32mINITIALIZING LIBTYPE\x1b[0m\n");
#endif
    _type_initialized = 1;
    if (FT_Init_FreeType(&freetype_library))
        die("Cannot initialize FreeType");
    fontconfig = FcInitLoadConfigAndFonts();
}

// TODO:
// void
// type_finish() {
//     pthread_mutex_lock(&library_mutex);
//     _type_initialized = 0;
//     FT_Done_FreeType(freetype_library);
//     FcConfigDestroy(fontconfig);
//     FcFini();
//     pthread_mutex_unlock(&library_mutex);
// }

void
type_destroy_font(TypeFont *font) {
    FT_Done_Face(font->face);
    FcPatternDestroy(font->pattern);
    free(font->file);
}

TypeFont*
type_load_font(const char* name) {
    pthread_mutex_lock(&loading_mutex);

    if (!_type_initialized)
        type_init();

#ifdef DEBUG
    fprintf(stderr, "\x1b[34mloading font %s\x1b[0m\n", name);
#endif
    TypeFont* font = malloc(sizeof(TypeFont));
    unsigned int i;

    font->ascent = 0;
    font->descent = 0;

    get_font_config(font, name);

    if (FT_New_Face(freetype_library, font->file, 0, &font->face))
        die("Cannot initialize FreeType face");

    FT_Set_Char_Size(font->face, font->height<<6, font->height<<6, 96, 96);

    for (i = 0; i < 256; i++) {
        font->blocks[i] = 0;
    }

    pthread_mutex_unlock(&loading_mutex);

    return font;
}

int
type_draw_text(TypeFont* font, const char *text) {
    int x = 0;
    Glyph g;
    glPushMatrix();
    for (;*text;) {
        g = get_glyph(font, nextUTF8(&text));
        draw_glyph(g);
        x += g.x_advance;
    }
    glPopMatrix();
    return x;
}

int
type_draw_char(TypeFont* font, unsigned int ch) {
    Glyph g = get_glyph(font, ch);
    draw_glyph(g);
    return g.x_advance;
}

int
type_text_width(TypeFont* font, const char* text) {
    int w = 0;
    Glyph g;

    for (;*text;) {
        g = get_glyph(font, nextUTF8(&text));
        w += g.x_advance;
    }

    return w;
}

int
type_font_height(TypeFont *font) {
    return font->ascent + font->descent;
}

int
type_font_ascent(TypeFont *font) {
    return font->ascent;
}

int
type_font_descent(TypeFont *font) {
    return font->descent;
}

void
type_set_dpi_for_glfw() {
    float w, h;
    int count;
    GLFWmonitor **monitors = glfwGetMonitors(&count);
    glfwGetMonitorContentScale(*monitors, &w, &h);
    pthread_mutex_lock(&loading_mutex);
    DPI = w * 96;
    pthread_mutex_unlock(&loading_mutex);
}
