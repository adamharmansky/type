#ifndef _UTF8_H
#define _UTF8_H

static inline void
UTF8_encode(char *text, unsigned int codepoint) {
    if (codepoint < 0x80) {
        text[0] = codepoint;
        text[1] = 0;
    } else if (codepoint < 0x800) {
        text[0] = 0xC0 | ((codepoint >> 6 ) & 0x1F);
        text[1] = 0x80 | (codepoint         & 0x3F);
        text[2] = 0;
    } else if (codepoint < 0x10000) {
        text[0] = 0xE0 | ((codepoint >> 12) & 0x0F);
        text[1] = 0x80 | ((codepoint >> 6 ) & 0x3F);
        text[2] = 0x80 | (codepoint         & 0x3F);
        text[3] = 0;
    } else {
        text[0] = 0xF0 | ((codepoint >> 18) & 0x07);
        text[1] = 0x80 | ((codepoint >> 12) & 0x3F);
        text[2] = 0x80 | ((codepoint >> 6 ) & 0x3F);
        text[3] = 0x80 | (codepoint         & 0x3F);
        text[4] = 0;
    }
}

static unsigned int
nextUTF8(const char** text) {
    unsigned int ret = 0;

    if ((**text & 0x80) == 0) {
        ret = *((*text)++);
    } else if ((**text & 0xE0) == 0xC0) {
        ret  = (*((*text)++) & 0x1F) << 6;
        ret |= (*((*text)++) & 0x3F);
    } else if ((**text & 0xF0) == 0xE0) {
        ret  = (*((*text)++) & 0x0F) << 12;
        ret |= (*((*text)++) & 0x3F) << 6;
        ret |= (*((*text)++) & 0x3F);
    } else if ((**text & 0xF8) == 0xF0) {
        ret  = (*((*text)++) & 0x07) << 18;
        ret |= (*((*text)++) & 0x3F) << 12;
        ret |= (*((*text)++) & 0x3F) << 6;
        ret |= (*((*text)++) & 0x3F);
    }

    return ret;
}

static inline unsigned int
UTF8_length(unsigned int codepoint) {
    if (codepoint < 0x80)         return 1;
    else if (codepoint < 0x800)   return 2;
    else if (codepoint < 0x10000) return 3;
    else                          return 4;
}

#endif
