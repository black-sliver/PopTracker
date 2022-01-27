#ifndef _UILIB_TEXTUTIL_H
#define _UILIB_TEXTUTIL_H

// helper functions for text rendering

#include "label.h"


namespace Ui {


static inline SDL_Surface* _RenderText(Label::FONT font, const char* text,
        SDL_Color color, Label::HAlign halign,
        int passes=2, int* w=nullptr, int* h=nullptr, int* result=nullptr)
{
    if (w) *w = 0;
    if (h) *h = 0;
    if (result) *result = 0;
    const char* firstlf = strchr(text, '\n');
    SDL_Surface* surf = nullptr;
    if (!firstlf && passes>1) {
        surf = TTF_RenderUTF8_Blended(font, text, color);
        if (surf && w) *w = surf->w;
        if (surf && h) *h = surf->h;
        return surf;
    } else if (!firstlf) {
        int res = TTF_SizeUTF8(font, text, w, h);
        if (result) *result = res;
        return nullptr;
    }
    char* buf = strdup(text);
    int maxW = 0, totalH = 0, curY = 0;
    int res = 0;
    int linespace = 1; // TODO: line pitch instead?
    for (int pass=0; pass<passes; pass++) {
        char* lf = buf+(firstlf-text);
        char* p = buf;
        if (pass == 1) {
            // pass1: render text
            // create surface in same format as TTF_*
            surf = SDL_CreateRGBSurfaceWithFormat(0, maxW, totalH, 32,
                    SDL_PIXELFORMAT_ARGB8888);
            if (!surf) goto err;
        }
        while (true) {
            // if not end of string, insert NUL at \n and optionally \r
            if (lf) {
                *lf = '\0';
                if (lf>buf && *(lf-1)=='\r') *(lf-1) = 0;
            }
            if (pass == 0) {
                // pass0: measure text
                int w=0,h=0;
                res = TTF_SizeUTF8(font, p, &w, &h);
                if (res != 0) goto err;
                if (w>maxW) maxW=w;
                totalH += h + linespace;
            } else {
                // pass1: render text
                SDL_Surface* line = TTF_RenderUTF8_Blended(font, p, color);
                if (line) {
                    SDL_SetSurfaceBlendMode(line, SDL_BLENDMODE_NONE); // copy alpha value instead of blending
                    SDL_Rect rect = { 0, curY, line->w, line->h };
                    if (halign == Label::HAlign::RIGHT)
                        rect.x = maxW - rect.w;
                    else if (halign == Label::HAlign::CENTER)
                        rect.x = (maxW - rect.w)/2;
                    SDL_BlitSurface(line, nullptr, surf, &rect);
                    curY += line->h + linespace;
                    SDL_FreeSurface(line);
                }
            }
            if (!lf) break; // pass done
            // restore string for next pass
            if (pass < passes-1) {
                *lf = '\n';
                if (lf>buf && *(lf-1)=='\0') *(lf-1) = '\r';
            }
            // find next \n
            p = lf+1;
            lf = strchr(p, '\n');
        }
        // undo space after last line
        if (pass == 0) totalH -= linespace;
    }
    if (w) *w = maxW;
    if (h) *h = totalH;
err:
    if (result) *result = res;
    free(buf);
    return surf;
}

static SDL_Surface* RenderText(Label::FONT font, const char* text, SDL_Color color, Label::HAlign halign)
{
    return _RenderText(font, text, color, halign);
}

static int SizeText(Label::FONT font, const char* text, int* w, int* h)
{
    int res;
    _RenderText(font, text, {0,0,0,0}, Label::HAlign::LEFT, 1, w, h, &res);
    return res;
}


} // namespace

#endif
