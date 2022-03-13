# Type

The *simplest* OpenGL 1.1 FreeType library.

 * basic FontConfig support
 * can acquire DPI from GLFW
 * pixel-based
 * full UTF-8 support
 * a name so generic, that no one ever thought of it

Example:

```
...
TypeFont *font = type_load_font("Ubuntu:pixelsize=32");
...

main loop {
    ...
    glColor4f(0, 0, 0, 1); // black text

    glTranslatef(type_font_ascent(font)); // set the baseline so that we see the text
    type_draw_text(font, "The quick brown fox is a loser");
    ...
}
```
