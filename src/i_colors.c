/*
========================================================================

                           D O O M  R e t r o
         The classic, refined DOOM source port. For Windows PC.

========================================================================

  Copyright © 1993-2012 id Software LLC, a ZeniMax Media company.
  Copyright © 2013-2017 Brad Harding.

  DOOM Retro is a fork of Chocolate DOOM.
  For a list of credits, see <http://wiki.doomretro.com/credits>.

  This file is part of DOOM Retro.

  DOOM Retro is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the
  Free Software Foundation, either version 3 of the License, or (at your
  option) any later version.

  DOOM Retro is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with DOOM Retro. If not, see <https://www.gnu.org/licenses/>.

  DOOM is a registered trademark of id Software LLC, a ZeniMax Media
  company, in the US and/or other countries and is used without
  permission. All other trademarks are the property of their respective
  holders. DOOM Retro is in no way affiliated with nor endorsed by
  id Software.

========================================================================
*/

#include "i_colors.h"
#include "m_fixed.h"
#include "w_wad.h"
#include "z_zone.h"

#define ADDITIVE       -1

#define R               1
#define W               2
#define G               4
#define B               8
#define X              16

static byte general[PALETTESIZE] =
{
    0,   X,   0,   0,   R|B, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // 000 to 015
    R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R, // 016 to 031
    R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R, // 032 to 047
    W,   W,   W,   W,   W,   W,   W,   W,   W,   W,   W,   W,   W,   W,   W,   W, // 048 to 063
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   X,   X,   X, // 064 to 079
    R,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // 080 to 095
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // 096 to 111
    G,   G,   G,   G,   G,   G,   G,   G,   G,   G,   G,   G,   G,   G,   G,   G, // 112 to 127
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // 128 to 143
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // 144 to 159
    R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R, // 160 to 175
    R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R, // 176 to 191
    B,   B,   B,   B,   B,   B,   B,   B,   B,   B,   B,   B,   B,   B,   B,   B, // 192 to 207
    R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R, // 208 to 223
    R|B, R|B, R,   R,   R,   R,   R,   R,   X,   X,   X,   X,   0,   0,   0,   0, // 224 to 239
    B,   B,   B,   B,   B,   B,   B,   B,   R,   R,   0,   0,   0,   0,   0,   0  // 240 to 255
};

#define ALL             0
#define REDS            R
#define WHITES          W
#define GREENS          G
#define BLUES           B
#define EXTRAS          X

byte    *tinttab;

byte    *tinttab20;
byte    *tinttab25;
byte    *tinttab33;
byte    *tinttab40;
byte    *tinttab50;
byte    *tinttab60;
byte    *tinttab66;
byte    *tinttab75;
byte    *tinttab80;

byte    *tranmap;

byte    *tinttabred;
byte    *tinttabredwhite1;
byte    *tinttabredwhite2;
byte    *tinttabgreen;
byte    *tinttabblue;

byte    *tinttabred33;
byte    *tinttabredwhite50;
byte    *tinttabgreen33;
byte    *tinttabblue25;

static struct
{
    byte        red;
    byte        green;
    byte        blue;
} originalcolors[PALETTESIZE] = {
    {   0,   0,   0 }, {  31,  23,  11 }, {  23,  15,   7 }, {  75,  75,  75 },
    { 255, 255, 255 }, {  27,  27,  27 }, {  19,  19,  19 }, {  11,  11,  11 },
    {   7,   7,   7 }, {  47,  55,  31 }, {  35,  43,  15 }, {  23,  31,   7 },
    {  15,  23,   0 }, {  79,  59,  43 }, {  71,  51,  35 }, {  63,  43,  27 },
    { 255, 183, 183 }, { 247, 171, 171 }, { 243, 163, 163 }, { 235, 151, 151 },
    { 231, 143, 143 }, { 223, 135, 135 }, { 219, 123, 123 }, { 211, 115, 115 },
    { 203, 107, 107 }, { 199,  99,  99 }, { 191,  91,  91 }, { 187,  87,  87 },
    { 179,  79,  79 }, { 175,  71,  71 }, { 167,  63,  63 }, { 163,  59,  59 },
    { 155,  51,  51 }, { 151,  47,  47 }, { 143,  43,  43 }, { 139,  35,  35 },
    { 131,  31,  31 }, { 127,  27,  27 }, { 119,  23,  23 }, { 115,  19,  19 },
    { 107,  15,  15 }, { 103,  11,  11 }, {  95,   7,   7 }, {  91,   7,   7 },
    {  83,   7,   7 }, {  79,   0,   0 }, {  71,   0,   0 }, {  67,   0,   0 },
    { 255, 235, 223 }, { 255, 227, 211 }, { 255, 219, 199 }, { 255, 211, 187 },
    { 255, 207, 179 }, { 255, 199, 167 }, { 255, 191, 155 }, { 255, 187, 147 },
    { 255, 179, 131 }, { 247, 171, 123 }, { 239, 163, 115 }, { 231, 155, 107 },
    { 223, 147,  99 }, { 215, 139,  91 }, { 207, 131,  83 }, { 203, 127,  79 },
    { 191, 123,  75 }, { 179, 115,  71 }, { 171, 111,  67 }, { 163, 107,  63 },
    { 155,  99,  59 }, { 143,  95,  55 }, { 135,  87,  51 }, { 127,  83,  47 },
    { 119,  79,  43 }, { 107,  71,  39 }, {  95,  67,  35 }, {  83,  63,  31 },
    {  75,  55,  27 }, {  63,  47,  23 }, {  51,  43,  19 }, {  43,  35,  15 },
    { 239, 239, 239 }, { 231, 231, 231 }, { 223, 223, 223 }, { 219, 219, 219 },
    { 211, 211, 211 }, { 203, 203, 203 }, { 199, 199, 199 }, { 191, 191, 191 },
    { 183, 183, 183 }, { 179, 179, 179 }, { 171, 171, 171 }, { 167, 167, 167 },
    { 159, 159, 159 }, { 151, 151, 151 }, { 147, 147, 147 }, { 139, 139, 139 },
    { 131, 131, 131 }, { 127, 127, 127 }, { 119, 119, 119 }, { 111, 111, 111 },
    { 107, 107, 107 }, {  99,  99,  99 }, {  91,  91,  91 }, {  87,  87,  87 },
    {  79,  79,  79 }, {  71,  71,  71 }, {  67,  67,  67 }, {  59,  59,  59 },
    {  55,  55,  55 }, {  47,  47,  47 }, {  39,  39,  39 }, {  35,  35,  35 },
    { 119, 255, 111 }, { 111, 239, 103 }, { 103, 223,  95 }, {  95, 207,  87 },
    {  91, 191,  79 }, {  83, 175,  71 }, {  75, 159,  63 }, {  67, 147,  55 },
    {  63, 131,  47 }, {  55, 115,  43 }, {  47,  99,  35 }, {  39,  83,  27 },
    {  31,  67,  23 }, {  23,  51,  15 }, {  19,  35,  11 }, {  11,  23,   7 },
    { 191, 167, 143 }, { 183, 159, 135 }, { 175, 151, 127 }, { 167, 143, 119 },
    { 159, 135, 111 }, { 155, 127, 107 }, { 147, 123,  99 }, { 139, 115,  91 },
    { 131, 107,  87 }, { 123,  99,  79 }, { 119,  95,  75 }, { 111,  87,  67 },
    { 103,  83,  63 }, {  95,  75,  55 }, {  87,  67,  51 }, {  83,  63,  47 },
    { 159, 131,  99 }, { 143, 119,  83 }, { 131, 107,  75 }, { 119,  95,  63 },
    { 103,  83,  51 }, {  91,  71,  43 }, {  79,  59,  35 }, {  67,  51,  27 },
    { 123, 127,  99 }, { 111, 115,  87 }, { 103, 107,  79 }, {  91,  99,  71 },
    {  83,  87,  59 }, {  71,  79,  51 }, {  63,  71,  43 }, {  55,  63,  39 },
    { 255, 255, 115 }, { 235, 219,  87 }, { 215, 187,  67 }, { 195, 155,  47 },
    { 175, 123,  31 }, { 155,  91,  19 }, { 135,  67,   7 }, { 115,  43,   0 },
    { 255, 255, 255 }, { 255, 219, 219 }, { 255, 187, 187 }, { 255, 155, 155 },
    { 255, 123, 123 }, { 255,  95,  95 }, { 255,  63,  63 }, { 255,  31,  31 },
    { 255,   0,   0 }, { 239,   0,   0 }, { 227,   0,   0 }, { 215,   0,   0 },
    { 203,   0,   0 }, { 191,   0,   0 }, { 179,   0,   0 }, { 167,   0,   0 },
    { 155,   0,   0 }, { 139,   0,   0 }, { 127,   0,   0 }, { 115,   0,   0 },
    { 103,   0,   0 }, {  91,   0,   0 }, {  79,   0,   0 }, {  67,   0,   0 },
    { 231, 231, 255 }, { 199, 199, 255 }, { 171, 171, 255 }, { 143, 143, 255 },
    { 115, 115, 255 }, {  83,  83, 255 }, {  55,  55, 255 }, {  27,  27, 255 },
    {   0,   0, 255 }, {   0,   0, 227 }, {   0,   0, 203 }, {   0,   0, 179 },
    {   0,   0, 155 }, {   0,   0, 131 }, {   0,   0, 107 }, {   0,   0,  83 },
    { 255, 255, 255 }, { 255, 235, 219 }, { 255, 215, 187 }, { 255, 199, 155 },
    { 255, 179, 123 }, { 255, 163,  91 }, { 255, 143,  59 }, { 255, 127,  27 },
    { 243, 115,  23 }, { 235, 111,  15 }, { 223, 103,  15 }, { 215,  95,  11 },
    { 203,  87,   7 }, { 195,  79,   0 }, { 183,  71,   0 }, { 175,  67,   0 },
    { 255, 255, 255 }, { 255, 255, 215 }, { 255, 255, 179 }, { 255, 255, 143 },
    { 255, 255, 107 }, { 255, 255,  71 }, { 255, 255,  35 }, { 255, 255,   0 },
    { 167,  63,   0 }, { 159,  55,   0 }, { 147,  47,   0 }, { 135,  35,   0 },
    {  79,  59,  39 }, {  67,  47,  27 }, {  55,  35,  19 }, {  47,  27,  11 },
    {   0,   0,  83 }, {   0,   0,  71 }, {   0,   0,  59 }, {   0,   0,  47 },
    {   0,   0,  35 }, {   0,   0,  23 }, {   0,   0,  11 }, {   0, 255, 255 },
    { 255, 159,  67 }, { 255, 231,  75 }, { 255, 123, 255 }, { 255,   0, 255 },
    { 207,   0, 207 }, { 159,   0, 155 }, { 111,   0, 107 }, { 167, 107, 107 }
};

byte    nearestcolors[PALETTESIZE];

int FindNearestColor(byte *palette, int red, int green, int blue)
{
    double      best_difference = INT_MAX;
    int         best_color = 0;
    int         i;

    for (i = 0; i < PALETTESIZE; ++i)
    {
        int     r1 = red;
        int     g1 = green;
        int     b1 = blue;
        int     r2 = *palette++;
        int     g2 = *palette++;
        int     b2 = *palette++;

        // From http://www.compuphase.com/cmetric.htm
        int     rmean = (r1 + r2) / 2;
        int     r = r1 - r2;
        int     g = g1 - g2;
        int     b = b1 - b2;
        double  difference = (((512 + rmean) * r * r) >> 8) + 4 * g * g + (((767 - rmean) * b * b) >> 8);

        if (difference < best_difference)
        {
            if (!difference)
                return i;

            best_color = i;
            best_difference = difference;
        }
    }
    return best_color;
}

void FindNearestColors(byte *palette)
{
    int i;

    if (W_CheckMultipleLumps("PLAYPAL") > 1)
        for (i = 0; i < PALETTESIZE; ++i)
            nearestcolors[i] = FindNearestColor(palette, originalcolors[i].red,
                originalcolors[i].green, originalcolors[i].blue);
    else
        for (i = 0; i < PALETTESIZE; ++i)
            nearestcolors[i] = i;
}

static byte *GenerateTintTable(byte *palette, int percent, byte filter[PALETTESIZE], int colors)
{
    byte        *result = Z_Malloc(PALETTESIZE * PALETTESIZE, PU_STATIC, NULL);
    int         foreground, background;

    for (foreground = 0; foreground < PALETTESIZE; ++foreground)
    {
        if ((filter[foreground] & colors) || colors == ALL)
        {
            for (background = 0; background < PALETTESIZE; ++background)
            {
                byte    *color1 = palette + background * 3;
                byte    *color2 = palette + foreground * 3;
                int     r, g, b;

                if (percent == ADDITIVE)
                {
                    if ((filter[background] & BLUES) && !(filter[foreground] & WHITES))
                    {
                        r = ((int)color1[0] * 25 + (int)color2[0] * 75) / 100;
                        g = ((int)color1[1] * 25 + (int)color2[1] * 75) / 100;
                        b = ((int)color1[2] * 25 + (int)color2[2] * 75) / 100;
                    }
                    else
                    {
                        r = MIN(color1[0] + color2[0], 255);
                        g = MIN(color1[1] + color2[1], 255);
                        b = MIN(color1[2] + color2[2], 255);
                    }
                }
                else
                {
                    r = ((int)color1[0] * percent + (int)color2[0] * (100 - percent)) / 100;
                    g = ((int)color1[1] * percent + (int)color2[1] * (100 - percent)) / 100;
                    b = ((int)color1[2] * percent + (int)color2[2] * (100 - percent)) / 100;
                }
                *(result + (background << 8) + foreground) = FindNearestColor(palette, r, g, b);
            }
        }
        else
            for (background = 0; background < PALETTESIZE; ++background)
                *(result + (background << 8) + foreground) = foreground;
    }

    return result;
}

void I_InitTintTables(byte *palette)
{
    lumpindex_t lump;

    tinttab = GenerateTintTable(palette, ADDITIVE, general, ALL);

    tinttab20 = GenerateTintTable(palette, 20, general, ALL);
    tinttab25 = GenerateTintTable(palette, 25, general, ALL);
    tinttab33 = GenerateTintTable(palette, 33, general, ALL);
    tinttab40 = GenerateTintTable(palette, 40, general, ALL);
    tinttab50 = GenerateTintTable(palette, 50, general, ALL);
    tinttab60 = GenerateTintTable(palette, 60, general, ALL);
    tinttab66 = GenerateTintTable(palette, 66, general, ALL);
    tinttab75 = GenerateTintTable(palette, 75, general, ALL);
    tinttab80 = GenerateTintTable(palette, 80, general, ALL);

    tranmap = ((lump = W_CheckNumForName("TRANMAP")) != -1 ? W_CacheLumpNum(lump, PU_STATIC) :
        tinttab25);

    tinttabred = GenerateTintTable(palette, ADDITIVE, general, REDS);
    tinttabredwhite1 = GenerateTintTable(palette, ADDITIVE, general, (REDS | WHITES));
    tinttabredwhite2 = GenerateTintTable(palette, ADDITIVE, general, (REDS | WHITES | EXTRAS));
    tinttabgreen = GenerateTintTable(palette, ADDITIVE, general, GREENS);
    tinttabblue = GenerateTintTable(palette, ADDITIVE, general, BLUES);

    tinttabred33 = GenerateTintTable(palette, 33, general, REDS);
    tinttabredwhite50 = GenerateTintTable(palette, 50, general, (REDS | WHITES));
    tinttabgreen33 = GenerateTintTable(palette, 33, general, GREENS);
    tinttabblue25 = GenerateTintTable(palette, 25, general, BLUES);
}
