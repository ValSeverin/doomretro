/*
========================================================================

                           D O O M  R e t r o
         The classic, refined DOOM source port. For Windows PC.

========================================================================

  Copyright © 1993-2022 by id Software LLC, a ZeniMax Media company.
  Copyright © 2013-2022 by Brad Harding <mailto:brad@doomretro.com>.

  DOOM Retro is a fork of Chocolate DOOM. For a list of credits, see
  <https://github.com/bradharding/doomretro/wiki/CREDITS>.

  This file is a part of DOOM Retro.

  DOOM Retro is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the
  Free Software Foundation, either version 3 of the license, or (at your
  option) any later version.

  DOOM Retro is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with DOOM Retro. If not, see <https://www.gnu.org/licenses/>.

  DOOM is a registered trademark of id Software LLC, a ZeniMax Media
  company, in the US and/or other countries, and is used without
  permission. All other trademarks are the property of their respective
  holders. DOOM Retro is in no way affiliated with nor endorsed by
  id Software.

========================================================================
*/

#include <ctype.h>

#include "am_map.h"
#include "c_console.h"
#include "d_deh.h"
#include "d_iwad.h"
#include "doomstat.h"
#include "dstrings.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "i_colors.h"
#include "i_gamecontroller.h"
#include "i_swap.h"
#include "i_system.h"
#include "i_timer.h"
#include "m_config.h"
#include "m_menu.h"
#include "m_misc.h"
#include "m_random.h"
#include "p_local.h"
#include "p_setup.h"
#include "s_sound.h"
#include "st_lib.h"
#include "st_stuff.h"
#include "v_data.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

#define LINEHEIGHT  17
#define OFFSET      17

int             episode = episode_default;
int             expansion = expansion_default;
int             m_sensitivity = m_sensitivity_default;
boolean         messages = messages_default;
int             r_detail = r_detail_default;
int             r_screensize = r_screensize_default;
int             savegame = savegame_default;
int             skilllevel = skilllevel_default;

// -1 = no quicksave slot picked!
int             quickSaveSlot;

// true = message to be printed
boolean         messagetoprint;
// ...and here is the message string!
static char     *messageString;

static int      messageLastMenuActive;

// timed message = no input from user
static boolean  messageNeedsInput;

static void (*messageRoutine)(int);

// we are going to be entering a savegame string
static boolean  saveStringEnter;
static int      saveSlot;               // which slot to save in
static int      saveCharIndex;          // which char we're editing

// old save description before edit
static char     saveOldString[SAVESTRINGSIZE];

boolean         inhelpscreens;
boolean         menuactive;
boolean         savegames;
boolean         quitting;

char            savegamestrings[6][SAVESTRINGSIZE];

static short    itemOn;                 // menu item skull is on
static short    skullAnimCounter;       // skull animation counter
static short    whichSkull;             // which skull to draw

static int      functionkey;

static boolean  usinggamecontroller;

// current menudef
static menu_t   *currentMenu;

int             spindirection;
int             spinspeed;
static angle_t  playerangle;

//
// PROTOTYPES
//
static void M_NewGame(int choice);
static void M_Episode(int choice);
static void M_Expansion(int choice);
static void M_ChooseSkill(int choice);
static void M_LoadGame(int choice);
static void M_SaveGame(int choice);
static void M_Options(int choice);
static void M_EndGame(int choice);

static void M_ChangeMessages(int choice);
static void M_ChangeSensitivity(int choice);
static void M_SfxVol(int choice);
static void M_MusicVol(int choice);
static void M_ChangeDetail(int choice);
static void M_SizeDisplay(int choice);
static void M_Sound(int choice);

static void M_FinishReadThis(int choice);
static void M_LoadSelect(int choice);
static void M_SaveSelect(int choice);
static void M_ReadSaveStrings(void);
static void M_QuickSave(void);
static void M_QuickLoad(void);

static void M_DrawMainMenu(void);
static void M_DrawReadThis(void);
static void M_DrawNewGame(void);
static void M_DrawEpisode(void);
static void M_DrawExpansion(void);
static void M_DrawOptions(void);
static void M_DrawSound(void);
static void M_DrawLoad(void);
static void M_DrawSave(void);

static void M_DrawSaveLoadBorder(int x, int y);
static void M_SetupNextMenu(menu_t *menudef);
static void M_DrawThermo(int x, int y, int thermWidth, float thermDot, float factor, int offset);
static void M_WriteText(int x, int y, char *string, boolean shadow);

//
// DOOM MENU
//

enum
{
    new_game,
    options,
    load_game,
    save_game,
    quit_doom,
    main_end
};

static menuitem_t MainMenu[] =
{
    { 1, "M_NGAME",  &M_NewGame,  &s_M_NEWGAME  },
    { 1, "M_OPTION", &M_Options,  &s_M_OPTIONS  },
    { 1, "M_LOADG",  &M_LoadGame, &s_M_LOADGAME },
    { 1, "M_SAVEG",  &M_SaveGame, &s_M_SAVEGAME },
    { 1, "M_QUITG",  &M_QuitDOOM, &s_M_QUITGAME }
};

menu_t MainDef =
{
    5,
    NULL,
    MainMenu,
    &M_DrawMainMenu,
    98, 77,
    new_game
};

//
// EPISODE SELECT
//

enum
{
    ep1,
    ep2,
    ep3,
    ep4,
    ep5,
    ep6,
    ep7,
    ep8,
    ep_end
};

static menuitem_t EpisodeMenu[] =
{
    { 1, "M_EPI1", &M_Episode, &s_M_EPISODE1 },
    { 1, "M_EPI2", &M_Episode, &s_M_EPISODE2 },
    { 1, "M_EPI3", &M_Episode, &s_M_EPISODE3 },
    { 1, "M_EPI4", &M_Episode, &s_M_EPISODE4 },
    { 1, "M_EPI5", &M_Episode, &s_M_EPISODE5 },

    // Some extra empty episodes for extensibility through UMAPINFO
    { 1, "M_EPI6", &M_Episode, &s_M_EPISODE6 },
    { 1, "M_EPI7", &M_Episode, &s_M_EPISODE7 },
    { 1, "M_EPI8", &M_Episode, &s_M_EPISODE8 }
};

menu_t EpiDef =
{
    ep_end,
    &MainDef,
    EpisodeMenu,
    &M_DrawEpisode,
    41, 69,
    ep1
};

//
// EXPANSION SELECT
//

enum
{
    ex1,
    ex2,
    ex_end
};

static menuitem_t ExpansionMenu[] =
{
    { 1, "M_EPI1", &M_Expansion, &s_M_EXPANSION1 },
    { 1, "M_EPI2", &M_Expansion, &s_M_EXPANSION2 }
};

menu_t ExpDef =
{
    ex_end,
    &MainDef,
    ExpansionMenu,
    &M_DrawExpansion,
    41, 69,
    ex1
};

//
// NEW GAME
//

enum
{
    killthings,
    toorough,
    hurtme,
    violence,
    nightmare,
    newg_end
};

static menuitem_t NewGameMenu[] =
{
    { 1, "M_JKILL", &M_ChooseSkill, &s_M_SKILLLEVEL1 },
    { 1, "M_ROUGH", &M_ChooseSkill, &s_M_SKILLLEVEL2 },
    { 1, "M_HURT",  &M_ChooseSkill, &s_M_SKILLLEVEL3 },
    { 1, "M_ULTRA", &M_ChooseSkill, &s_M_SKILLLEVEL4 },
    { 1, "M_NMARE", &M_ChooseSkill, &s_M_SKILLLEVEL5 }
};

menu_t NewDef =
{
    newg_end,
    &EpiDef,
    NewGameMenu,
    &M_DrawNewGame,
    45, 69,
    hurtme
};

//
// OPTIONS MENU
//

enum
{
    endgame,
    msgs,
    detail,
    scrnsize,
    option_empty1,
    mousesens,
    option_empty2,
    soundvol,
    opt_end
};

static menuitem_t OptionsMenu[] =
{
    {  1, "M_ENDGAM", &M_EndGame,           &s_M_ENDGAME          },
    {  1, "M_MESSG",  &M_ChangeMessages,    &s_M_MESSAGES         },
    {  1, "M_DETAIL", &M_ChangeDetail,      &s_M_GRAPHICDETAIL    },
    {  2, "M_SCRNSZ", &M_SizeDisplay,       &s_M_SCREENSIZE       },
    { -1, "",         NULL,                 NULL                  },
    {  2, "M_MSENS",  &M_ChangeSensitivity, &s_M_MOUSESENSITIVITY },
    { -1, "",         NULL,                 NULL                  },
    {  1, "M_SVOL",   &M_Sound,             &s_M_SOUNDVOLUME      }
};

static menu_t OptionsDef =
{
    opt_end,
    &MainDef,
    OptionsMenu,
    &M_DrawOptions,
    56, 33,
    endgame
};

enum
{
    rdthsempty,
    read_end
};

static menuitem_t ReadMenu[] =
{
    { 1, "", &M_FinishReadThis, NULL }
};

static menu_t ReadDef =
{
    read_end,
    &ReadDef,
    ReadMenu,
    &M_DrawReadThis,
    330, 175,
    rdthsempty
};

//
// SOUND VOLUME MENU
//

enum
{
    sfx_vol,
    sfx_empty1,
    music_vol,
    sfx_empty2,
    sound_end
};

static menuitem_t SoundMenu[] =
{
    {  2, "M_SFXVOL", &M_SfxVol,   &s_M_SFXVOLUME   },
    { -1, "",         NULL,        NULL             },
    {  2, "M_MUSVOL", &M_MusicVol, &s_M_MUSICVOLUME },
    { -1, "",         NULL,        NULL             }
};

static menu_t SoundDef =
{
    sound_end,
    &OptionsDef,
    SoundMenu,
    &M_DrawSound,
    89, 64,
    sfx_vol
};

//
// LOAD GAME MENU
//

enum
{
    load1,
    load2,
    load3,
    load4,
    load5,
    load6,
    load_end
};

static menuitem_t LoadGameMenu[] =
{
    { 1, "", &M_LoadSelect, NULL },
    { 1, "", &M_LoadSelect, NULL },
    { 1, "", &M_LoadSelect, NULL },
    { 1, "", &M_LoadSelect, NULL },
    { 1, "", &M_LoadSelect, NULL },
    { 1, "", &M_LoadSelect, NULL }
};

menu_t LoadDef =
{
    load_end,
    &MainDef,
    LoadGameMenu,
    &M_DrawLoad,
    67, 51,
    load1
};

//
// SAVE GAME MENU
//

static menuitem_t SaveGameMenu[] =
{
    { 1, "", &M_SaveSelect, NULL },
    { 1, "", &M_SaveSelect, NULL },
    { 1, "", &M_SaveSelect, NULL },
    { 1, "", &M_SaveSelect, NULL },
    { 1, "", &M_SaveSelect, NULL },
    { 1, "", &M_SaveSelect, NULL }
};

menu_t SaveDef =
{
    load_end,
    &MainDef,
    SaveGameMenu,
    &M_DrawSave,
    67, 51,
    load1
};

static void BlurScreen(byte *src, byte *dest, int width, int area)
{
    for (int i = 0; i < area; i++)
        dest[i] = grays[src[i]];

    for (int y = 0; y <= area - width; y += width)
        for (int x = y; x <= y + width - 2; x++)
            dest[x] = tinttab50[(dest[x + 1] << 8) + dest[x]];

    for (int y = 0; y <= area - width; y += width)
        for (int x = y + width - 2; x > y; x--)
            dest[x] = tinttab50[(dest[x - 1] << 8) + dest[x]];

    for (int y = width; y <= area - width * 3; y += width)
        for (int x = y; x <= y + width - 3; x++)
            dest[x] = tinttab50[(dest[x + width * ((M_BigRandom() & 3) - 1) + (M_BigRandom() & 3) - 1] << 8) + dest[x]];

    for (int y = area - width; y >= width; y -= width)
        for (int x = y + width - 1; x >= y + 1; x--)
            dest[x] = tinttab50[(dest[x - width - 1] << 8) + dest[x]];

    for (int y = 0; y <= area - width * 2; y += width)
        for (int x = y; x <= y + width - 1; x++)
            dest[x] = tinttab50[(dest[x + width] << 8) + dest[x]];

    for (int y = area - width; y >= width; y -= width)
        for (int x = y; x <= y + width - 1; x++)
            dest[x] = tinttab50[(dest[x - width] << 8) + dest[x]];

    for (int y = 0; y <= area - width * 2; y += width)
        for (int x = y + width - 1; x >= y + 1; x--)
            dest[x] = tinttab50[(dest[x + width - 1] << 8) + dest[x]];

    for (int y = area - width; y >= width; y -= width)
        for (int x = y; x <= y + width - 2; x++)
            dest[x] = tinttab50[(dest[x - width + 1] << 8) + dest[x]];
}

static int  blurtic = -1;

//
// M_DarkBackground
//  darken and blur background while menu is displayed
//
void M_DarkBackground(void)
{
    static byte blurscreen1[MAXSCREENAREA];

    if (gametime != blurtic)
    {
        if (vid_widescreen)
        {
            for (int y = 2 * SCREENWIDTH; y < SCREENAREA; y += 4 * SCREENWIDTH)
                for (int x = 0; x < SCREENWIDTH; x++)
                {
                    byte    *dot = *screens + x + y;

                    *dot = white25[*dot];
                }
        }
        else
        {
            for (int i = 0; i < SCREENAREA; i += SCREENWIDTH)
            {
                screens[0][i] = nearestblack;
                screens[0][i + 1] = nearestblack;
                screens[0][i + SCREENWIDTH - 2] = nearestblack;
                screens[0][i + SCREENWIDTH - 1] = nearestblack;
            }

            for (int y = 2 * SCREENWIDTH; y < SCREENAREA; y += 4 * SCREENWIDTH)
                for (int x = 2; x < SCREENWIDTH - 2; x++)
                {
                    byte    *dot = *screens + x + y;

                    *dot = white25[*dot];
                }
        }

        BlurScreen(screens[0], blurscreen1, SCREENWIDTH, SCREENAREA);

        for (int i = 0; i < SCREENAREA; i++)
        {
            byte    *dot = blurscreen1 + i;

            *dot = black40[*dot];
        }

        blurtic = gametime;
    }

    memcpy(screens[0], blurscreen1, SCREENAREA);

    if (mapwindow)
        memset(mapscreen, nearestblack, MAPAREA);

    if (r_detail == r_detail_low)
        V_LowGraphicDetail_Menu();
}

static byte blues[] =
{
    245, 245, 245, 242, 197, 245, 245, 245, 245, 244, 245, 245, 245, 243, 244, 244,
    200, 201, 201, 202, 203, 203, 204, 204, 205, 206, 206, 206, 207, 207, 207, 207,
    241, 242, 242, 243, 243, 244, 244, 244, 245, 245, 245, 245, 245, 245, 245, 245,
    197, 198, 198, 199, 199, 199, 200, 200, 200, 201, 202, 202, 203, 203, 204, 204,
    205, 206, 206, 206, 207, 207, 207, 207, 241, 242, 242, 243, 243, 244, 245, 245,
    197, 198, 198, 199, 199, 200, 200, 201, 201, 202, 202, 203, 203, 204, 204, 205,
    205, 206, 206, 207, 207, 207, 207, 241, 241, 242, 243, 243, 244, 244, 245, 245,
    200, 201, 202, 203, 204, 205, 206, 207, 207, 241, 242, 243, 244, 245, 245, 245,
    202, 203, 204, 204, 205, 205, 206, 206, 207, 207, 207, 207, 241, 241, 242, 243,
    205, 206, 207, 207, 241, 242, 243, 244, 206, 207, 207, 207, 241, 242, 243, 243,
    197, 199, 202, 204, 206, 207, 241, 243, 197, 198, 200, 201, 203, 205, 206, 207,
    242, 242, 243, 243, 243, 244, 244, 244, 244, 245, 245, 245, 245, 245, 245, 245,
    198, 200, 202, 203, 205, 207, 242, 244, 245, 245, 245, 245, 245, 245, 245, 245,
    197, 197, 198, 199, 201, 201, 203, 204, 205, 205, 206, 206, 207, 207, 207, 207,
    197, 197, 197, 197, 197, 198, 198, 198, 241, 241, 242, 243, 243, 244, 245, 245,
    245, 245, 245, 245, 245, 245, 245, 245, 202, 199, 202, 207, 241, 243, 244, 245
};

//
// M_DarkBlueBackground
//  darken background, make it blue and pixelate while help screen is displayed
//
static void M_DarkBlueBackground(void)
{
    for (int y = 0; y < SCREENAREA; y += 2 * SCREENWIDTH)
        for (int x = 0; x < SCREENWIDTH; x += 2)
        {
            byte        *dot1 = *screens + y + x;
            byte        *dot2 = dot1 + 1;
            byte        *dot3 = dot2 + SCREENWIDTH;
            byte        *dot4 = dot3 - 1;
            const byte  color = blues[tinttab50[(tinttab50[(*dot1 << 8) + *dot2] << 8) + tinttab50[(*dot3 << 8) + *dot4]]];

            *dot1 = color;
            *dot2 = color;
            *dot3 = color;
            *dot4 = color;
        }

    if (automapactive)
        memset(screens[0], nearestcolors[245], SCREENAREA);
    else if (mapwindow)
        memset(mapscreen, nearestcolors[245], MAPAREA);
}

//
// M_DrawChar
//  draw a character on screen
//
static void M_DrawChar(int x, int y, int i, boolean overlapping)
{
    int w = (int)strlen(redcharset[i]) / 18;

    for (int y1 = 0; y1 < 18; y1++)
        for (int x1 = 0; x1 < w; x1++)
        {
            char    dot = redcharset[i][y1 * w + x1];

            if (dot == '\xC8')
            {
                if (!overlapping)
                    V_DrawPixel(x + x1, y + y1, PINK, true);
            }
            else
                V_DrawPixel(x + x1, y + y1, (int)dot, true);
        }
}

static const int chartoi[123] =
{
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1,  0, -1, -1, -1, -1, -1,  1,
    -1, -1, -1, -1,  2,  3,  4, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,  5, -1,
    -1, -1, -1,  6, -1,  7,  8,  9, 10, 11,
    12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
    22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
    32, -1, -1, -1, -1, -1, -1, 33, 34, 35,
    36, 37, 38, 39, 40, 41, 42, 43, 44, 45,
    46, 47, 48, 49, 50, 51, 52, 53, 54, 55,
    56, 57, 58
};

static struct
{
    char    char1;
    char    char2;
    int     adjust;
} bigkern[] = {
    { '-', 'V', -2 }, { 'O', 'A', -1 }, { 'P', 'a', -3 }, { 'V', 'o', -2 },
    { 'f', 'e', -1 }, { 'f', 'f', -1 }, { 'f', 'o', -1 }, { 'l', 'e', -1 },
    { 'l', 't', -1 }, { 'o', 'a', -1 }, { 'o', 't', -1 }, { 'p', 'a', -2 },
    { 't', 'o', -1 }, { 'v', 'e', -1 }, { 'y', ',', -3 }, { 'y', '.', -2 },
    { 'y', 'o', -1 }, { 't', 'a', -1 }, { 'l', 'o', -1 }, { ' ', 'V', -2 },
    { ' ', 'y', -2 }, { ' ', 't', -1 }, { 'l', ' ', -1 }, { 'L', 'S', -1 },
    { 't', ' ', -1 }, {  0,   0,   0 }
};

static struct
{
    char    char1;
    char    char2;
} overlap[] = {
    { 'A', 'D' }, { 'A', 'M' }, { 'E', 'a' }, { 'E', 'n' }, { 'E', 'p' },
    { 'E', 'x' }, { 'G', 'A' }, { 'G', 'a' }, { 'I', 'n' }, { 'K', 'n' },
    { 'L', 'i' }, { 'a', 'd' }, { 'a', 'm' }, { 'a', 'n' }, { 'a', 'r' },
    { 'c', 'h' }, { 'c', 'r' }, { 'e', 'a' }, { 'e', 'd' }, { 'e', 'n' },
    { 'e', 'p' }, { 'e', 'r' }, { 'e', 's' }, { 'g', 'h' }, { 'h', 'i' },
    { 'i', 'n' }, { 'i', 's' }, { 'i', 'z' }, { 'k', 'i' }, { 'p', 'i' },
    { 'p', 't' }, { 'r', 'a' }, { 'r', 'n' }, { 'x', 'p' }, { 'G', 'r' },
    { 'a', 'p' }, { 'a', 'i' }, { 'e', 't' }, { 'i', 't' }, { 'o', 't' },
    { 'P', 'T' }, { 'r', 't' }, { 's', 't' }, { 'n', 't' }, {  0,   0  }
};

//
// M_DrawString
//  draw a string on screen
//
void M_DrawString(int x, int y, char *string)
{
    static char prev;
    const int   len = (int)strlen(string);

    for (int i = 0; i < len; i++)
    {
        int     j = -1;
        int     k = 0;
        boolean overlapping = false;

        if (string[i] < 123)
            j = chartoi[(int)string[i]];

        while (bigkern[k].char1)
        {
            if (prev == bigkern[k].char1 && string[i] == bigkern[k].char2)
            {
                x += bigkern[k].adjust;
                break;
            }

            k++;
        }

        k = 0;

        while (overlap[k].char1)
        {
            if (prev == overlap[k].char1 && string[i] == overlap[k].char2)
            {
                overlapping = true;
                break;
            }

            k++;
        }

        if (j == -1)
            x += 7;
        else
        {
            M_DrawChar(x, y, j, overlapping);
            x += (int)strlen(redcharset[j]) / 18 - 2;
        }

        prev = string[i];
    }
}

//
// M_BigStringWidth
//  return width of string in pixels
//
static int M_BigStringWidth(char *string)
{
    int         w = 0;
    static char prev;
    const int   len = (int)strlen(string);

    for (int i = 0; i < len; i++)
    {
        int j = chartoi[(int)string[i]];
        int k = 0;

        while (bigkern[k].char1)
        {
            if (prev == bigkern[k].char1 && string[i] == bigkern[k].char2)
                w += bigkern[k].adjust;

            k++;
        }

        w += (j == -1 ? 7 : (int)strlen(redcharset[j]) / 18 - 2);
        prev = string[i];
    }

    return w;
}

//
// M_DrawCenteredString
//  draw a string centered horizontally on screen
//
void M_DrawCenteredString(int y, char *string)
{
    M_DrawString((VANILLAWIDTH - M_BigStringWidth(string) - 1) / 2, y, string);
}

//
// M_SplitString
//  split string of words into two lines
//
static void M_SplitString(char *string)
{
    const int   len = (int)strlen(string);

    for (int i = len / 2 - 1; i < len; i++)
        if (string[i] == ' ')
        {
            string[i] = '\n';
            break;
        }
}

//
// M_DrawPatchWithShadow
//  draw patch with shadow on screen
//
static void M_DrawPatchWithShadow(int x, int y, patch_t *patch)
{
    if (!patch)
        return;

    if (SHORT(patch->height) < VANILLAHEIGHT)
        V_DrawPatchWithShadow(x, y, patch, false);
    else
        V_DrawPagePatch(patch);
}

//
// M_DrawCenteredPatchWithShadow
//  draw patch with shadow horizontally centered on screen
//
static void M_DrawCenteredPatchWithShadow(int y, patch_t *patch)
{
    if (!patch)
        return;

    if (SHORT(patch->height) < VANILLAHEIGHT)
        V_DrawPatchWithShadow((VANILLAWIDTH - SHORT(patch->width)) / 2 + SHORT(patch->leftoffset), y, patch, false);
    else
        V_DrawPagePatch(patch);
}

//
// M_ReadSaveStrings
//  read the strings from the savegame files
//
static void M_ReadSaveStrings(void)
{
    char    name[256];

    savegames = false;

    for (int i = 0; i < load_end; i++)
    {
        FILE    *handle;

        M_StringCopy(name, P_SaveGameFile(i), sizeof(name));

        if (!(handle = fopen(name, "rb")))
        {
            M_StringCopy(&savegamestrings[i][0], s_EMPTYSTRING, sizeof(savegamestrings[i]));
            LoadGameMenu[i].status = 0;

            continue;
        }

        if (fread(&savegamestrings[i], 1, SAVESTRINGSIZE, handle))
        {
            if (savegamestrings[i][0])
            {
                savegames = true;
                LoadGameMenu[i].status = 1;
            }
            else
            {
                M_StringCopy(&savegamestrings[i][0], s_EMPTYSTRING, sizeof(savegamestrings[i]));
                LoadGameMenu[i].status = 0;
            }
        }

        fclose(handle);
    }
}

static byte saveg_read8(FILE *file)
{
    byte    result = -1;

    if (fread(&result, 1, 1, file) < 1)
        return 0;

    return result;
}

//
// M_CheckSaveGame
//
static boolean M_CheckSaveGame(int *ep, int *map, int slot)
{
    FILE    *file = fopen(P_SaveGameFile(slot), "rb");
    int     mission;

    if (!file)
        return false;

    for (int i = 0; i < SAVESTRINGSIZE + VERSIONSIZE + 1; i++)
        saveg_read8(file);

    *ep = saveg_read8(file);
    *map = saveg_read8(file);
    mission = saveg_read8(file);
    fclose(file);

    // switch expansions if necessary
    if (mission == doom2)
    {
        if (gamemission == doom2)
            return true;

        if (gamemission == pack_nerve)
        {
            ExpDef.lastOn = ex1;
            expansion = 1;
            gamemission = doom2;
            M_SaveCVARs();

            return true;
        }
        else
            return false;
    }

    if (mission == pack_nerve)
    {
        if (gamemission == pack_nerve)
            return true;

        if (gamemission == doom2 && nerve)
        {
            ExpDef.lastOn = ex2;
            expansion = 2;
            gamemission = pack_nerve;
            M_SaveCVARs();

            return true;
        }
        else
            return false;
    }

    if (mission != gamemission)
        return false;

    if (*ep > 1 && gamemode == shareware)
        return false;

    if (*ep > 3 && gamemode == registered)
        return false;

    return true;
}

int M_CountSaveGames(void)
{
    int count = 0;

    for (int i = 0; i < load_end; i++)
        if (M_FileExists(P_SaveGameFile(i)))
            count++;

    return count;
}

//
// Draw border for the savegame description
//
static void M_DrawSaveLoadBorder(int x, int y)
{
    if (M_LSCNTR)
    {
        x += 3;
        M_DrawPatchWithShadow(x, y + 11, W_CacheLumpName("M_LSLEFT"));
        x += 8;

        for (int i = 0; i < 24; i++)
        {
            M_DrawPatchWithShadow(x, y + 11, W_CacheLumpName("M_LSCNTR"));
            x += 8;
        }

        M_DrawPatchWithShadow(x, y + 11, W_CacheLumpName("M_LSRGHT"));
    }
    else
    {
        for (int yy = 0; yy < 16; yy++)
            for (int xx = 0; xx < 8; xx++)
                V_DrawPixel(x + xx, y + yy, lsleft[yy * 8 + xx], true);

        x += 8;

        for (int i = 0; i < 24; i++)
        {
            for (int yy = 0; yy < 16; yy++)
                for (int xx = 0; xx < 8; xx++)
                    V_DrawPixel(x + xx, y + yy, lscntr[yy * 8 + xx], true);

            x += 8;
        }

        for (int yy = 0; yy < 16; yy++)
            for (int xx = 0; xx < 9; xx++)
                V_DrawPixel(x + xx, y + yy, lsrght[yy * 9 + xx], true);
    }
}

//
// M_LoadGame
//
static void M_DrawLoad(void)
{
    M_DarkBackground();

    if (M_LGTTL)
        M_DrawCenteredPatchWithShadow(23 + OFFSET, W_CacheLumpName("M_LGTTL"));
    else if (M_LOADG)
        M_DrawCenteredPatchWithShadow(23 + OFFSET, W_CacheLumpName("M_LOADG"));
    else
    {
        char    *temp = uppercase(s_M_LOADGAME);

        M_DrawCenteredString(23 + OFFSET, temp);
        free(temp);
    }

    for (int i = 0; i < load_end; i++)
    {
        int     y = LoadDef.y + LINEHEIGHT * i + OFFSET;
        int     len;
        char    buffer[SAVESTRINGSIZE];

        M_DrawSaveLoadBorder(LoadDef.x - 11, y - 4);

        M_StringCopy(buffer, savegamestrings[i], sizeof(buffer));
        len = (int)strlen(buffer);

        while (M_StringWidth(buffer) > SAVESTRINGPIXELWIDTH)
        {
            if (len >= 2 && buffer[len - 2] == ' ')
            {
                buffer[len - 2] = '.';
                buffer[len - 1] = '.';
                buffer[len] = '.';
                buffer[len + 1] = '\0';
            }
            else if (len >= 1)
            {
                buffer[len - 1] = '.';
                buffer[len] = '.';
                buffer[len + 1] = '.';
                buffer[len + 2] = '\0';
            }

            len--;
        }

        M_WriteText(LoadDef.x - 2 + (M_StringCompare(buffer, s_EMPTYSTRING) && s_EMPTYSTRING[0] == '-'
            && s_EMPTYSTRING[1] == '\0') * 6, y - !M_LSCNTR, buffer, false);
    }
}

//
// User wants to load this game
//
static void M_LoadSelect(int choice)
{
    int ep;
    int map;

    if (M_CheckSaveGame(&ep, &map, choice))
    {
        char    name[SAVESTRINGSIZE];

        M_StringCopy(name, P_SaveGameFile(choice), sizeof(name));
        S_StartSound(NULL, sfx_pistol);
        functionkey = 0;
        quickSaveSlot = choice;
        G_LoadGame(name);
    }
    else
    {
        C_ShowConsole();
        C_Warning(1, "This savegame requires a different WAD.");
    }

    M_ClearMenus();
}

//
// Selected from DOOM menu
//
static void M_LoadGame(int choice)
{
    M_SetupNextMenu(&LoadDef);
    M_ReadSaveStrings();
}

static boolean  showcaret;
static int      caretwait;
int             caretcolor;

//
//  M_SaveGame
//
static void M_DrawSave(void)
{
    M_DarkBackground();

    // draw menu subtitle
    if (M_SGTTL)
        M_DrawCenteredPatchWithShadow(23 + OFFSET, W_CacheLumpName("M_SGTTL"));
    else if (M_SAVEG)
        M_DrawCenteredPatchWithShadow(23 + OFFSET, W_CacheLumpName("M_SAVEG"));
    else
    {
        char    *temp = uppercase(s_M_SAVEGAME);

        M_DrawCenteredString(23 + OFFSET, temp);
        free(temp);
    }

    // draw each save game slot
    for (int i = 0; i < load_end; i++)
    {
        int     y = LoadDef.y + i * LINEHEIGHT + OFFSET;
        int     len;
        char    buffer[SAVESTRINGSIZE];

        M_DrawSaveLoadBorder(LoadDef.x - 11, y - 4);

        M_StringCopy(buffer, savegamestrings[i], sizeof(buffer));
        len = (int)strlen(buffer);

        while (M_StringWidth(buffer) > SAVESTRINGPIXELWIDTH)
        {
            if (len >= 2 && buffer[len - 2] == ' ')
            {
                buffer[len - 2] = '.';
                buffer[len - 1] = '.';
                buffer[len] = '.';
                buffer[len + 1] = '\0';
            }
            else if (len >= 1)
            {
                buffer[len - 1] = '.';
                buffer[len] = '.';
                buffer[len + 1] = '.';
                buffer[len + 2] = '\0';
            }

            len--;
        }

        // draw save game slot background
        M_DrawSaveLoadBorder(LoadDef.x - 11, y - 4);

        // draw save game description
        if (saveStringEnter && i == saveSlot)
        {
            char        left[256] = "";
            char        right[256] = "";
            int         j;
            int         x;

            // draw text to left of text caret
            for (j = 0; j < saveCharIndex; j++)
                left[j] = buffer[j];

            left[j] = '\0';
            M_WriteText(LoadDef.x - 2, y - !M_LSCNTR, left, false);
            x = LoadDef.x - 2 + M_StringWidth(left);

            // draw text to right of text caret
            for (j = 0; j < len - saveCharIndex; j++)
                right[j] = buffer[j + saveCharIndex];

            right[j] = '\0';
            M_WriteText(x + 1, y - !M_LSCNTR, right, false);

            // draw text caret
            if (windowfocused)
            {
                if (caretwait < I_GetTimeMS())
                {
                    showcaret = !showcaret;
                    caretwait = I_GetTimeMS() + CARETBLINKTIME;
                }

                if (showcaret)
                {
                    int h = y + SHORT(hu_font[0]->height);

                    while (y < h)
                        V_DrawPixel(x, y++, caretcolor, false);
                }
            }
            else
            {
                showcaret = false;
                caretwait = 0;
            }
        }
        else
            M_WriteText(LoadDef.x - 2 + (M_StringCompare(buffer, s_EMPTYSTRING) && s_EMPTYSTRING[0] == '-'
                && s_EMPTYSTRING[1] == '\0') * 6, y - !M_LSCNTR, buffer, false);
    }
}

//
// M_Responder calls this when user is finished
//
static void M_DoSave(int slot)
{
    M_ClearMenus();
    G_SaveGame(slot, savegamestrings[slot], "");
    functionkey = 0;
    quickSaveSlot = slot;
}

//
// User wants to save. Start string input for M_Responder
//
static char *RemoveMapNum(char *string)
{
    char    *newstr = M_StringDuplicate(string);
    char    *pos = strchr(newstr, ':');

    if (pos)
    {
        newstr = pos + 1;

        while (newstr[0] == ' ')
            newstr++;
    }

    return newstr;
}

void M_UpdateSaveGameName(int i)
{
    boolean match = false;
    int     len = (int)strlen(savegamestrings[i]);

    if (M_StringCompare(savegamestrings[i], s_EMPTYSTRING))
        match = true;
    else if (gamemission == doom && len == 4 && savegamestrings[i][0] == 'E' && isdigit((int)savegamestrings[i][1])
        && savegamestrings[i][2] == 'M' && isdigit((int)savegamestrings[i][3]) && W_CheckNumForName(savegamestrings[i]) >= 0)
        match = true;
    else if (gamemission != doom && len == 5 && savegamestrings[i][0] == 'M' && savegamestrings[i][1] == 'A'
        && savegamestrings[i][2] == 'P' && isdigit((int)savegamestrings[i][3]) && isdigit((int)savegamestrings[i][4])
        && W_CheckNumForName(savegamestrings[i]) >= 0)
        match = true;

    if (!match && !M_StringCompare(mapnum, mapnumandtitle))
    {
        if (len >= 4 && savegamestrings[i][len - 1] == '.' && savegamestrings[i][len - 2] == '.'
            && savegamestrings[i][len - 3] == '.' && savegamestrings[i][len - 4] != '.')
            match = true;
        else
        {
            int ep;
            int map;

            if (M_CheckSaveGame(&ep, &map, i))
                switch (gamemission)
                {
                    case doom:
                        if ((map == 10 && M_StringCompare(savegamestrings[i], s_HUSTR_E1M4B))
                            || (map == 11 && M_StringCompare(savegamestrings[i], s_HUSTR_E1M8B))
                            || M_StringCompare(savegamestrings[i], RemoveMapNum(*mapnames[(ep - 1) * 9 + map - 1])))
                            match = true;

                        break;

                    case doom2:
                        if (M_StringCompare(savegamestrings[i],
                            RemoveMapNum(bfgedition ? *mapnames2_bfg[map - 1] : *mapnames2[map - 1])))
                            match = true;

                        break;

                    case pack_nerve:
                        if (M_StringCompare(savegamestrings[i], RemoveMapNum(*mapnamesn[map - 1])))
                            match = true;

                        break;

                    case pack_plut:
                        if (M_StringCompare(savegamestrings[i], RemoveMapNum(*mapnamesp[map - 1])))
                            match = true;

                        break;

                    case pack_tnt:
                        if (M_StringCompare(savegamestrings[i], RemoveMapNum(*mapnamest[map - 1])))
                            match = true;

                        break;

                    default:
                        break;
                }
        }
    }

    if (match)
        M_StringCopy(savegamestrings[i], maptitle, sizeof(savegamestrings[i]));
}

static void M_SaveSelect(int choice)
{
    // we are going to be intercepting all chars
    SDL_StartTextInput();
    saveStringEnter = true;
    saveSlot = choice;
    M_StringCopy(saveOldString, savegamestrings[saveSlot], sizeof(saveOldString));
    M_UpdateSaveGameName(saveSlot);
    saveCharIndex = (int)strlen(savegamestrings[saveSlot]);
    showcaret = !showcaret;
    caretwait = I_GetTimeMS() + CARETBLINKTIME;
}

//
// Selected from DOOM menu
//
static void M_SaveGame(int choice)
{
    M_SetupNextMenu(&SaveDef);
    M_ReadSaveStrings();
}

//
// M_QuickSave
//
static void M_QuickSave(void)
{
    if (quickSaveSlot < 0)
    {
        if (functionkey == KEY_F6)
        {
            functionkey = 0;
            M_ClearMenus();
            S_StartSound(NULL, sfx_swtchx);
        }
        else
        {
            functionkey = KEY_F6;
            M_StartControlPanel();
            M_ReadSaveStrings();
            M_SetupNextMenu(&SaveDef);
            S_StartSound(NULL, sfx_swtchn);
        }

        return;
    }

    M_UpdateSaveGameName(quickSaveSlot);
    M_DoSave(quickSaveSlot);
}

//
// M_QuickLoad
//
static void M_QuickLoadResponse(int key)
{
    messagetoprint = false;

    if (key == 'y')
    {
        M_LoadSelect(quickSaveSlot);
        S_StartSound(NULL, sfx_swtchx);
    }
}

static void M_QuickLoad(void)
{
    if (quickSaveSlot < 0)
    {
        functionkey = 0;
        return;
    }

    S_StartSound(NULL, sfx_swtchn);

    if (M_StringEndsWith(s_QLPROMPT, s_PRESSYN))
        M_StartMessage(s_QLPROMPT, &M_QuickLoadResponse, true);
    else
    {
        static char buffer[160];

        M_snprintf(buffer, sizeof(buffer), s_QLPROMPT, savegamestrings[quickSaveSlot]);
        M_SplitString(buffer);
        M_snprintf(buffer, sizeof(buffer), "%s\n\n%s", buffer, (usinggamecontroller ? s_PRESSA : s_PRESSYN));
        M_StartMessage(buffer, &M_QuickLoadResponse, true);
    }
}

static void M_DeleteSavegameResponse(int key)
{
    if (key == 'y')
    {
        static char buffer[1024];
        char        *temp;

        M_StringCopy(buffer, P_SaveGameFile(itemOn), sizeof(buffer));

        if (remove(buffer) == -1)
        {
            S_StartSound(NULL, sfx_oof);
            return;
        }

        temp = titlecase(savegamestrings[itemOn]);
        M_snprintf(buffer, sizeof(buffer), s_GGDELETED, temp);
        C_Output("%s", buffer);
        HU_SetPlayerMessage(buffer, false, false);
        message_dontfuckwithme = true;
        M_ReadSaveStrings();
        free(temp);

        if (itemOn == quickSaveSlot)
            quickSaveSlot = -1;

        if (currentMenu == &LoadDef)
        {
            if (savegames)
            {
                while (M_StringCompare(savegamestrings[itemOn], s_EMPTYSTRING))
                    itemOn = (!itemOn ? currentMenu->numitems - 1 : itemOn - 1);
            }
            else
            {
                M_SetupNextMenu(&MainDef);
                MainDef.lastOn = itemOn = save_game;
            }
        }
    }
}

static void M_DeleteSavegame(void)
{
    static char buffer[160];

    S_StartSound(NULL, sfx_swtchn);
    M_snprintf(buffer, sizeof(buffer), s_DELPROMPT, savegamestrings[saveSlot]);
    M_SplitString(buffer);
    M_snprintf(buffer, sizeof(buffer), "%s\n\n%s", buffer, (usinggamecontroller ? s_PRESSA : s_PRESSYN));
    M_StartMessage(buffer, &M_DeleteSavegameResponse, true);
}

//
// M_DrawReadThis
//
static void M_DrawReadThis(void)
{
    char    lumpname[6] = "HELP1";

    if (gamemode == shareware)
        M_StringCopy(lumpname, "HELP3", sizeof(lumpname));
    else if (gamemode == registered)
        M_StringCopy(lumpname, "HELP2", sizeof(lumpname));
    else if (gamemode == commercial)
        M_StringCopy(lumpname, "HELP", sizeof(lumpname));

    if (W_CheckNumForName(lumpname) >= 0)
    {
        if (chex || FREEDOOM || hacx || REKKRSA)
        {
            patch_t *lump = W_CacheLastLumpName(gamemode == commercial ? "HELP" : "HELP1");

            if (SCREENWIDTH != NONWIDEWIDTH)
                memset(screens[0], FindDominantEdgeColor(lump), SCREENAREA);

            V_DrawPatch(0, 0, 0, lump);

            if (mapwindow)
                memset(mapscreen, nearestblack, MAPAREA);
        }
        else if (autosigil)
        {
            viewplayer->fixedcolormap = 0;
            M_DarkBlueBackground();
            V_DrawPatchWithShadow(0, 0, W_CacheSecondLumpName(lumpname), false);
        }
        else if (W_CheckMultipleLumps(lumpname) > 2)
        {
            patch_t *lump = W_CacheLumpName(lumpname);

            if (SCREENWIDTH != NONWIDEWIDTH)
                memset(screens[0], FindDominantEdgeColor(lump), SCREENAREA);

            V_DrawPatch(0, 0, 0, lump);

            if (mapwindow)
                memset(mapscreen, nearestblack, MAPAREA);
        }
        else
        {
            viewplayer->fixedcolormap = 0;
            M_DarkBlueBackground();
            V_DrawPatchWithShadow(0, 0, W_CacheLumpName(lumpname), false);
        }
    }
}

//
// Change SFX and Music volumes
//
static void M_DrawSound(void)
{
    M_DarkBackground();

    if (M_SVOL)
    {
        M_DrawPatchWithShadow((chex ? 100 : 60), 38 + OFFSET, W_CacheLumpName("M_SVOL"));
        SoundDef.x = (chex ? 68 : 80);
        SoundDef.y = 64;
    }
    else
    {
        char    *temp = uppercase(s_M_SOUNDVOLUME);

        M_DrawCenteredString(38 + OFFSET, temp);
        free(temp);
    }

    M_DrawThermo(SoundDef.x - 1, SoundDef.y + 16 * (sfx_vol + 1) + OFFSET + !hacx, 16, (float)(sfxVolume * !nosfx), 4.0f, 6);
    M_DrawThermo(SoundDef.x - 1, SoundDef.y + 16 * (music_vol + 1) + OFFSET + !hacx, 16, (float)(musicVolume * !nomusic), 4.0f, 6);
}

static void M_Sound(int choice)
{
    M_SetupNextMenu(&SoundDef);
}

static void M_SfxVol(int choice)
{
    if (nosfx)
        return;

    switch (choice)
    {
        case 0:
            if (sfxVolume > 0)
            {
                S_SetSfxVolume(--sfxVolume * MIX_MAX_VOLUME / 31);
                S_StartSound(NULL, sfx_stnmov);
                s_sfxvolume = sfxVolume * 100 / 31;
                C_PctCVAROutput(stringize(s_sfxvolume), s_sfxvolume);
                M_SaveCVARs();
            }

            break;

        case 1:
            if (sfxVolume < 31)
            {
                S_SetSfxVolume(++sfxVolume * MIX_MAX_VOLUME / 31);
                S_StartSound(NULL, sfx_stnmov);
                s_sfxvolume = sfxVolume * 100 / 31;
                C_PctCVAROutput(stringize(s_sfxvolume), s_sfxvolume);
                M_SaveCVARs();
            }

            break;
    }
}

static void M_MusicVol(int choice)
{
    if (nomusic)
        return;

    switch (choice)
    {
        case 0:
            if (musicVolume > 0)
            {
                musicVolume--;
                S_LowerMusicVolume();
                S_StartSound(NULL, sfx_stnmov);
                s_musicvolume = musicVolume * 100 / 31;
                C_PctCVAROutput(stringize(s_musicvolume), s_musicvolume);
                M_SaveCVARs();
            }

            break;

        case 1:
            if (musicVolume < 31)
            {
                musicVolume++;
                S_LowerMusicVolume();
                S_StartSound(NULL, sfx_stnmov);
                s_musicvolume = musicVolume * 100 / 31;
                C_PctCVAROutput(stringize(s_musicvolume), s_musicvolume);
                M_SaveCVARs();
            }

            break;
    }
}

//
// M_DrawMainMenu
//
static void M_DrawMainMenu(void)
{
    patch_t *patch = W_CacheLumpName("M_DOOM");

    M_DarkBackground();

    if (M_DOOM)
    {
        if (SHORT(patch->height) == VANILLAHEIGHT)
            V_DrawPatch(94, 2, 0, patch);
        else
            M_DrawPatchWithShadow(94, 2 + OFFSET, patch);

        MainDef.x = 97;
        MainDef.y = 72;
    }
    else if (gamemode == commercial)
        M_DrawCenteredPatchWithShadow(11 + OFFSET, patch);
    else
    {
        int y = 11 + OFFSET;
        int dot1 = screens[0][(y * SCREENWIDTH + 98 + WIDESCREENDELTA) * SCREENSCALE];
        int dot2 = screens[0][((y + 1) * SCREENWIDTH + 99 + WIDESCREENDELTA) * SCREENSCALE];

        M_DrawCenteredPatchWithShadow(y, patch);
        V_DrawPixel(98, y, dot1, false);
        V_DrawPixel(99, y + 1, dot2, false);
    }
}

//
// M_Episode
//
static int      epi;
boolean         EpiCustom;
static short    EpiMenuMap[] = { 1, 1, 1, 1, -1, -1, -1, -1 };
static short    EpiMenuEpi[] = { 1, 2, 3, 4, -1, -1, -1, -1 };

void M_AddEpisode(int map, int ep, const char *lumpname, const char *string)
{
    if (!EpiCustom)
    {
        EpiCustom = true;
        NewDef.prevMenu = &EpiDef;

        if (gamemode == commercial)
            EpiDef.numitems = 0;
        else if (EpiDef.numitems > 4)
            EpiDef.numitems = 4;
    }

    if (!*lumpname && !*string)
        EpiDef.numitems = 0;
    else
    {
        if (EpiDef.numitems >= 8)
            return;

        EpiMenuEpi[EpiDef.numitems] = ep;
        EpiMenuMap[EpiDef.numitems] = map - (ep - 1) * 10;
        M_StringCopy(EpisodeMenu[EpiDef.numitems].name, lumpname, sizeof(EpisodeMenu[EpiDef.numitems].name));
        *EpisodeMenu[EpiDef.numitems].text = M_StringDuplicate(string);
        EpiDef.numitems++;
    }
}

static void M_DrawEpisode(void)
{
    M_DarkBackground();

    if (M_NEWG)
    {
        M_DrawCenteredPatchWithShadow(14 + OFFSET, W_CacheLumpName("M_NEWG"));
        EpiDef.x = 48;
        EpiDef.y = 63;
    }
    else if (M_NGAME)
    {
        M_DrawCenteredPatchWithShadow(14 + OFFSET, W_CacheLumpName("M_NGAME"));
        EpiDef.x = 48;
        EpiDef.y = 63;
    }
    else
    {
        char    *temp = uppercase(s_M_NEWGAME);

        M_DrawCenteredString(19 + OFFSET, temp);
        free(temp);
    }

    if (M_EPISOD)
    {
        M_DrawCenteredPatchWithShadow(38 + OFFSET, W_CacheLumpName("M_EPISOD"));
        EpiDef.x = 48;
        EpiDef.y = 63;
    }
    else
        M_DrawCenteredString(44 + OFFSET, s_M_WHICHEPISODE);
}

void M_SetWindowCaption(void)
{
    static char caption[128];

    if (gamestate == GS_LEVEL)
        M_StringCopy(caption, mapnumandtitle, sizeof(caption));
    else
    {
        if (nerve && (currentMenu == &ExpDef || currentMenu == &NewDef))
            M_snprintf(caption, sizeof(caption), "%s: %s", gamedescription,
                (expansion == 1 ? s_CAPTION_HELLONEARTH : s_CAPTION_NERVE));
        else
            M_StringCopy(caption, gamedescription, sizeof(caption));

        if (bfgedition && (nerve || !modifiedgame))
            M_snprintf(caption, sizeof(caption), "%s (%s)", caption, s_CAPTION_BFGEDITION);
    }

    SDL_SetWindowTitle(window, caption);
}

static void M_DrawExpansion(void)
{
    M_DarkBackground();

    if (M_NEWG)
    {
        M_DrawCenteredPatchWithShadow(14 + OFFSET, W_CacheLumpName("M_NEWG"));
        EpiDef.x = 48;
        EpiDef.y = 63;
    }
    else if (M_NGAME)
    {
        M_DrawCenteredPatchWithShadow(14 + OFFSET, W_CacheLumpName("M_NGAME"));
        EpiDef.x = 48;
        EpiDef.y = 63;
    }
    else
    {
        char    *temp = uppercase(s_M_NEWGAME);

        M_DrawCenteredString(19 + OFFSET, temp);
        free(temp);
    }

    if (M_EPISOD)
    {
        M_DrawCenteredPatchWithShadow(38 + OFFSET, W_CacheLumpName("M_EPISOD"));
        EpiDef.x = 48;
        EpiDef.y = 63;
    }
    else
        M_DrawCenteredString(44 + OFFSET, s_M_WHICHEXPANSION);
}

static void M_VerifyNightmare(int key)
{
    messagetoprint = false;

    if (key != 'y')
        M_SetupNextMenu(&NewDef);
    else
    {
        quickSaveSlot = -1;
        M_ClearMenus();
        viewplayer->cheats = 0;
        G_DeferredInitNew((skill_t)nightmare, epi + 1, 1);
    }
}

static void M_ChooseSkill(int choice)
{
    if (choice == nightmare && gameskill != sk_nightmare && !nomonsters)
    {
        if (M_StringEndsWith(s_NIGHTMARE, s_PRESSYN))
            M_StartMessage(s_NIGHTMARE, &M_VerifyNightmare, true);
        else
        {
            static char buffer[160];

            M_snprintf(buffer, sizeof(buffer), "%s\n\n%s", s_NIGHTMARE, (usinggamecontroller ? s_PRESSA : s_PRESSYN));
            M_StartMessage(buffer, &M_VerifyNightmare, true);
        }

        D_FadeScreen(false);
        return;
    }

    HU_DrawDisk();
    S_StartSound(NULL, sfx_pistol);
    quickSaveSlot = -1;
    M_ClearMenus();
    viewplayer->cheats = 0;

    if (!EpiCustom)
        G_DeferredInitNew((skill_t)choice, epi + 1, 1);
    else
        G_DeferredInitNew((skill_t)choice, EpiMenuEpi[epi], EpiMenuMap[epi]);
}

static void M_Episode(int choice)
{
    if (!EpiCustom)
    {
        if (gamemode == shareware && choice)
        {
            if (M_StringEndsWith(s_SWSTRING, s_PRESSYN))
                M_StartMessage(s_SWSTRING, NULL, false);
            else
            {
                static char buffer[160];

                M_snprintf(buffer, sizeof(buffer), "%s\n\n%s", s_SWSTRING, (usinggamecontroller ? s_PRESSA : s_PRESSKEY));
                M_StartMessage(buffer, NULL, false);
            }

            M_SetupNextMenu(&EpiDef);
            return;
        }
    }

    epi = choice;
    M_SetupNextMenu(&NewDef);
}

static void M_Expansion(int choice)
{
    gamemission = (choice == ex1 ? doom2 : pack_nerve);
    D_SetSaveGameFolder(false);
    M_ReadSaveStrings();
    M_SetupNextMenu(&NewDef);
}

//
// M_NewGame
//
static void M_DrawNewGame(void)
{
    M_DarkBackground();

    if (M_NEWG)
    {
        M_DrawCenteredPatchWithShadow(14 + OFFSET, W_CacheLumpName("M_NEWG"));
        NewDef.x = (chex ? 98 : 48);
        NewDef.y = 63;
    }
    else if (M_NGAME)
    {
        M_DrawCenteredPatchWithShadow(14 + OFFSET, W_CacheLumpName("M_NGAME"));
        NewDef.x = (chex ? 98 : 48);
        NewDef.y = 63;
    }
    else
    {
        char    *temp = uppercase(s_M_NEWGAME);

        M_DrawCenteredString(19 + OFFSET, temp);
        free(temp);
    }

    if (M_SKILL)
    {
        M_DrawCenteredPatchWithShadow(38 + OFFSET, W_CacheLumpName("M_SKILL"));
        NewDef.x = (chex ? 98 : 48);
        NewDef.y = 63;
    }
    else
        M_DrawCenteredString(44 + OFFSET, s_M_CHOOSESKILLLEVEL);
}

static void M_NewGame(int choice)
{
    M_SetupNextMenu(chex ? &NewDef : (gamemode == commercial && !EpiCustom ? (nerve ? &ExpDef : &NewDef) : &EpiDef));
}

//
// M_Options
//
static void M_DrawOptions(void)
{
    M_DarkBackground();

    if (M_OPTTTL)
    {
        M_DrawPatchWithShadow((chex ? 126 : 108), 15 + OFFSET, W_CacheLumpName("M_OPTTTL"));
        OptionsDef.x = (chex ? 69 : 60);
        OptionsDef.y = 37;
    }
    else
    {
        char    *temp = uppercase(s_M_OPTIONS);

        M_DrawCenteredString(8 + OFFSET, temp);
        free(temp);
    }

    if (messages)
    {
        if (M_MSGON)
            M_DrawPatchWithShadow(OptionsDef.x + SHORT(((patch_t *)W_CacheLumpName("M_MESSG"))->width) + 4,
                OptionsDef.y + 16 * msgs + OFFSET, W_CacheLumpName("M_MSGON"));
        else
            M_DrawString(OptionsDef.x + 122, OptionsDef.y + 16 * msgs + OFFSET, s_M_ON);
    }
    else
    {
        if (M_MSGOFF)
            M_DrawPatchWithShadow(OptionsDef.x + SHORT(((patch_t *)W_CacheLumpName("M_MESSG"))->width) + 4,
                OptionsDef.y + 16 * msgs + OFFSET, W_CacheLumpName("M_MSGOFF"));
        else
            M_DrawString(OptionsDef.x + 122, OptionsDef.y + 16 * msgs + OFFSET, s_M_OFF);
    }

    if (r_detail == r_detail_low)
    {
        if (M_GDLOW)
            M_DrawPatchWithShadow(OptionsDef.x + SHORT(((patch_t *)W_CacheLumpName("M_DETAIL"))->width) + 4,
                OptionsDef.y + 16 * detail + OFFSET, W_CacheLumpName("M_GDLOW"));
        else
            M_DrawString(OptionsDef.x + 173, OptionsDef.y + 16 * detail + OFFSET, s_M_LOW);
    }
    else
    {
        if (M_GDHIGH)
            M_DrawPatchWithShadow(OptionsDef.x + SHORT(((patch_t *)W_CacheLumpName("M_DETAIL"))->width) + 4,
                OptionsDef.y + 16 * detail + OFFSET, W_CacheLumpName("M_GDHIGH"));
        else
            M_DrawString(OptionsDef.x + 173, OptionsDef.y + 16 * detail + OFFSET, s_M_HIGH);
    }

    M_DrawThermo(OptionsDef.x - 1, OptionsDef.y + 16 * (scrnsize + 1) + OFFSET + !hacx, 9, (float)(r_screensize
        + (r_screensize < r_screensize_max - 1 ? 0 : (r_screensize == r_screensize_max - 1 ? vid_widescreen : 1 + !r_hud))), 6.54f, 8);

    if (usinggamecontroller && !M_MSENS)
        M_DrawThermo(OptionsDef.x - 1, OptionsDef.y + 16 * (mousesens + 1) + OFFSET + !hacx, 9,
            joy_sensitivity_horizontal / (float)joy_sensitivity_horizontal_max * 8.0f, 8.0f, 8);
    else
        M_DrawThermo(OptionsDef.x - 1, OptionsDef.y + 16 * (mousesens + 1) + OFFSET + !hacx, 9,
            m_sensitivity / (float)m_sensitivity_max * 8.0f, 8.0f, 8);
}

static void M_Options(int choice)
{
    if (!OptionsDef.change)
        OptionsDef.lastOn = (gamestate == GS_LEVEL ? endgame : msgs);

    M_SetupNextMenu(&OptionsDef);
}

//
// Toggle messages on/off
//
static void M_ChangeMessages(int choice)
{
    messages = !messages;
    C_StrCVAROutput(stringize(messages), (messages ? "on" : "off"));

    if (!menuactive)
    {
        if (messages)
        {
            C_Output(s_MSGON);
            HU_SetPlayerMessage(s_MSGON, false, false);
        }
        else
        {
            C_Output(s_MSGOFF);
            HU_SetPlayerMessage(s_MSGOFF, false, false);
        }

        message_dontfuckwithme = true;
    }
    else
        C_Output(messages ? s_MSGON : s_MSGOFF);

    M_SaveCVARs();
}

//
// M_EndGame
//
static boolean  endinggame;

void M_EndingGame(void)
{
    endinggame = true;

    if (gamemission == pack_nerve)
        gamemission = doom2;

    if (!M_StringCompare(console[consolestrings - 1].string, "endgame"))
        C_Input("endgame");

    C_AddConsoleDivider();
    M_SetWindowCaption();
    D_StartTitle(1);
}

static void M_EndGameResponse(int key)
{
    messagetoprint = false;

    if (key != 'y')
    {
        if (functionkey == KEY_F7)
            M_ClearMenus();
        else
            M_SetupNextMenu(&OptionsDef);

        return;
    }

    currentMenu->lastOn = itemOn;
    S_StopMusic();
    M_ClearMenus();
    viewactive = false;
    automapactive = false;
    S_StartSound(NULL, sfx_swtchx);
    MainDef.lastOn = 0;
    st_palette = 0;
    M_EndingGame();
}

static void M_EndGame(int choice)
{
    if (gamestate != GS_LEVEL)
        return;

    if (M_StringEndsWith(s_ENDGAME, s_PRESSYN))
        M_StartMessage(s_ENDGAME, &M_EndGameResponse, true);
    else
    {
        static char buffer[160];

        M_snprintf(buffer, sizeof(buffer), "%s\n\n%s", s_ENDGAME, (usinggamecontroller ? s_PRESSA : s_PRESSYN));
        M_StartMessage(buffer, &M_EndGameResponse, true);
    }
}

//
// M_FinishReadThis
//
static void M_FinishReadThis(int choice)
{
    M_SetupNextMenu(&MainDef);
}

//
// M_QuitDOOM
//
static const int quitsounds[8] =
{
    sfx_pldeth,
    sfx_dmpain,
    sfx_popain,
    sfx_slop,
    sfx_telept,
    sfx_posit1,
    sfx_posit3,
    sfx_sgtatk
};

static const int quitsounds2[8] =
{
    sfx_vilact,
    sfx_getpow,
    sfx_boscub,
    sfx_slop,
    sfx_skeswg,
    sfx_kntdth,
    sfx_bspact,
    sfx_sgtatk
};

static void M_QuitResponse(int key)
{
    messagetoprint = false;

    if (key != 'y')
    {
        quitting = false;

        if (waspaused)
        {
            waspaused = false;
            paused = true;
        }

        if (functionkey == KEY_F10)
            M_ClearMenus();
        else
            M_SetupNextMenu(&MainDef);

        return;
    }

    if (!nosfx && sfxVolume > 0)
    {
        int i = 30;

        if (gamemode == commercial)
            S_StartSound(NULL, quitsounds2[M_Random() & 7]);
        else
            S_StartSound(NULL, quitsounds[M_Random() & 7]);

        // wait until all sounds stopped or 3 seconds has passed
        while (i-- > 0 && I_AnySoundStillPlaying())
            I_Sleep(100);
    }

    I_Quit(true);
}

void M_QuitDOOM(int choice)
{
    static char endstring[320];
    static char line1[160];
    static char line2[160];

    quitting = true;

    if (deh_strlookup[p_QUITMSG].assigned == 2)
        M_StringCopy(line1, s_QUITMSG, sizeof(line1));
    else
    {
        static int  msg = -1;

        msg = M_RandomIntNoRepeat(0, NUM_QUITMESSAGES - 1, msg);

        if (devparm)
            M_StringCopy(line1, devendmsg[msg], sizeof(line1));
        else if (gamemission == doom)
            M_snprintf(line1, sizeof(line1), *endmsg[msg], WINDOWS);
        else
            M_snprintf(line1, sizeof(line1), *endmsg[NUM_QUITMESSAGES + msg], WINDOWS);
    }

    M_snprintf(line2, sizeof(line2), (usinggamecontroller ? s_DOSA : s_DOSY), DESKTOP);
    M_snprintf(endstring, sizeof(endstring), "%s\n\n%s", line1, line2);
    M_StartMessage(endstring, &M_QuitResponse, true);
}

static void M_SliderSound(void)
{
    static int  wait;

    if (wait < I_GetTime())
    {
        wait = I_GetTime() + 7;
        S_StartSound(NULL, sfx_stnmov);
    }
}

static void M_ChangeSensitivity(int choice)
{
    if (usinggamecontroller && !M_MSENS)
    {
        switch (choice)
        {
            case 0:
                if (joy_sensitivity_horizontal > joy_sensitivity_horizontal_min)
                {
                    if (joy_sensitivity_horizontal & 1)
                        joy_sensitivity_horizontal++;

                    joy_sensitivity_horizontal -= 2;
                    I_SetGameControllerHorizontalSensitivity();
                    C_IntCVAROutput(stringize(joy_sensitivity_horizontal), joy_sensitivity_horizontal);
                    M_SliderSound();
                    M_SaveCVARs();
                }

                break;

            case 1:
                if (joy_sensitivity_horizontal < joy_sensitivity_horizontal_max)
                {
                    if (joy_sensitivity_horizontal & 1)
                        joy_sensitivity_horizontal--;

                    joy_sensitivity_horizontal += 2;
                    I_SetGameControllerHorizontalSensitivity();
                    C_IntCVAROutput(stringize(joy_sensitivity_horizontal), joy_sensitivity_horizontal);
                    M_SliderSound();
                    M_SaveCVARs();
                }

                break;
        }
    }
    else
    {
        switch (choice)
        {
            case 0:
                if (m_sensitivity > m_sensitivity_min)
                {
                    if (m_sensitivity & 1)
                        m_sensitivity++;

                    m_sensitivity -= 2;
                    C_IntCVAROutput(stringize(m_sensitivity), m_sensitivity);
                    M_SliderSound();
                    M_SaveCVARs();
                }

                break;

            case 1:
                if (m_sensitivity < m_sensitivity_max)
                {
                    if (m_sensitivity & 1)
                        m_sensitivity--;

                    m_sensitivity += 2;
                    C_IntCVAROutput(stringize(m_sensitivity), m_sensitivity);
                    M_SliderSound();
                    M_SaveCVARs();
                }

                break;
        }
    }
}

static void M_ChangeDetail(int choice)
{
    r_detail = !r_detail;
    C_StrCVAROutput(stringize(r_detail), (r_detail == r_detail_low ? "low" : "high"));

    if (!menuactive)
    {
        if (r_detail == r_detail_low)
        {
            C_Output(s_DETAILLO);
            HU_SetPlayerMessage(s_DETAILLO, false, false);
        }
        else
        {
            C_Output(s_DETAILHI);
            HU_SetPlayerMessage(s_DETAILHI, false, false);
        }

        message_dontfuckwithme = true;
    }
    else
        C_Output(r_detail == r_detail_low ? s_DETAILLO : s_DETAILHI);

    M_SaveCVARs();
    STLib_Init();
    R_InitColumnFunctions();

    if (gamestate == GS_LEVEL)
        D_FadeScreen(false);
}

static void M_SizeDisplay(int choice)
{
    switch (choice)
    {
        case 0:
            if (r_screensize == r_screensize_max && !r_hud)
            {
                r_hud = true;
                C_StrCVAROutput(stringize(r_hud), "on");
                S_StartSound(NULL, sfx_stnmov);
            }
            else if (r_screensize == r_screensize_max - 1 && vid_widescreen)
            {
                vid_widescreen = false;
                C_StrCVAROutput(stringize(vid_widescreen), "off");
                I_RestartGraphics(false);
                S_StartSound(NULL, sfx_stnmov);
            }
            else if (r_screensize > r_screensize_min)
            {
                C_IntCVAROutput(stringize(r_screensize), --r_screensize);
                R_SetViewSize(menuactive && viewactive ? r_screensize_max : r_screensize);
                AM_SetAutomapSize(automapactive ? r_screensize_max : r_screensize);

                if (r_screensize == r_screensize_max - 1)
                    r_hud = false;

                S_StartSound(NULL, sfx_stnmov);
            }
            else
                return;

            break;

        case 1:
            if (r_screensize == r_screensize_max && r_hud)
            {
                r_hud = false;
                C_StrCVAROutput(stringize(r_hud), "off");
                S_StartSound(NULL, sfx_stnmov);
            }
            else if (r_screensize == r_screensize_max - 1 && !vid_widescreen && !nowidescreen)
            {
                vid_widescreen = true;
                C_StrCVAROutput(stringize(vid_widescreen), "on");
                I_RestartGraphics(false);
                S_StartSound(NULL, sfx_stnmov);
            }
            else if (r_screensize < r_screensize_max)
            {
                C_IntCVAROutput(stringize(r_screensize), ++r_screensize);
                R_SetViewSize(menuactive && viewactive ? r_screensize_max : r_screensize);
                AM_SetAutomapSize(automapactive ? r_screensize_max : r_screensize);

                if (r_screensize == r_screensize_max)
                    r_hud = true;

                S_StartSound(NULL, sfx_stnmov);
            }
            else
                return;

            break;
    }

    M_SaveCVARs();

    blurtic = -1;

    if (r_playersprites)
        skippsprinterp = true;
}

//
// Menu Functions
//
static void M_DrawThermo(int x, int y, int thermWidth, float thermDot, float factor, int offset)
{
    int xx = x;

    if (chex || hacx)
    {
        xx--;
        y -= 2;
    }

    M_DrawPatchWithShadow(xx, y, W_CacheLumpName("M_THERML"));
    xx += 8;

    for (int i = 0; i < thermWidth; i++)
    {
        V_DrawPatch(xx, y, 0, W_CacheLumpName("M_THERMM"));
        xx += 8;
    }

    M_DrawPatchWithShadow(xx, y, W_CacheLumpName("M_THERMR"));

    for (int i = x + 9; i < x + (thermWidth + 1) * 8 + 1; i++)
        V_DrawPixel(i - hacx, y + (hacx ? 9 : 13), PINK, true);

    V_DrawPatch(x + offset + (int)(thermDot * factor), y, 0, W_CacheLumpName("M_THERMO"));
}

void M_StartMessage(char *string, void *routine, boolean input)
{
    messageLastMenuActive = menuactive;
    messagetoprint = true;
    messageString = string;
    messageRoutine = (void (*)(int))routine;
    messageNeedsInput = input;
    menuactive = true;

    I_SetPalette(PLAYPAL);
    I_UpdateBlitFunc(false);

    D_FadeScreen(false);
}

//
// Find character width
//
static int M_CharacterWidth(char ch, char prev)
{
    int c = toupper(ch) - HU_FONTSTART;

    if (c < 0 || c >= HU_FONTSIZE)
        return (prev == '.' || prev == '!' || prev == '?' ? 5 : 3);
    else
        return (STCFN034 ? SHORT(hu_font[c]->width) : (int)strlen(smallcharset[c]) / 10 - 1);
}

//
// Find string width
//
int M_StringWidth(char *string)
{
    int         w = M_CharacterWidth(string[0], '\0');
    const int   len = (int)strlen(string);

    for (int i = 1; i < len; i++)
        w += M_CharacterWidth(string[i], string[i - 1]);

    return w;
}

//
// Find string height
//
static int M_StringHeight(char *string)
{
    int         h = (STCFN034 ? SHORT(hu_font[0]->height) : 8) + 1;
    const int   len = (int)strlen(string);

    for (int i = 1; i < len; i++)
        if (string[i] == '\n')
            h += (string[i - 1] == '\n' ? 3 : (STCFN034 ? SHORT(hu_font[0]->height) : 8) + 1);

    return (h - 1);
}

//
//  Write a char
//
void M_DrawSmallChar(int x, int y, int i, boolean shadow)
{
    int w = (int)strlen(smallcharset[i]) / 10;

    for (int y1 = 0; y1 < 10; y1++)
        for (int x1 = 0; x1 < w; x1++)
            if (x + x1 < VANILLAWIDTH && y + y1 < VANILLAHEIGHT)
                V_DrawPixel(x + x1, y + y1, (int)smallcharset[i][y1 * w + x1], shadow);
}

//
// Write a string
//
static void M_WriteText(int x, int y, char *string, boolean shadow)
{
    int     w;
    char    *ch = string;
    char    letter;
    char    prev = ' ';
    int     cx = x;
    int     cy = y;

    while (true)
    {
        int c = *ch++;

        if (!c)
            break;

        if (c == '\n')
        {
            cx = x;
            cy += 12;

            continue;
        }

        letter = c;
        c = toupper(c) - HU_FONTSTART;

        if (c < 0 || c >= HU_FONTSIZE)
        {
            cx += (prev == '.' || prev == '!' || prev == '?' ? 5 : 3);
            prev = letter;

            continue;
        }

        if (STCFN034)
        {
            w = SHORT(hu_font[c]->width);

            if (cx + w > VANILLAWIDTH)
                break;

            if (shadow)
                M_DrawPatchWithShadow(cx, cy, hu_font[c]);
            else
                V_DrawPatch(cx, cy, 0, hu_font[c]);
        }
        else
        {
            if (prev == ' ')
            {
                if (letter == '"')
                    c = 64;
                else if (letter == '\'')
                    c = 65;
            }

            w = (int)strlen(smallcharset[c]) / 10 - 1;

            if (cx + w > VANILLAWIDTH)
                break;

            M_DrawSmallChar(cx, cy, c, shadow);
        }

        prev = letter;
        cx += w;
    }
}

static void M_ShowHelp(int choice)
{
    functionkey = KEY_F1;
    inhelpscreens = true;
    M_StartControlPanel();
    currentMenu = &ReadDef;
    itemOn = 0;
    S_StartSound(NULL, sfx_swtchn);

    if (!automapactive && gamestate == GS_LEVEL)
        R_SetViewSize(r_screensize_max);
}

static void M_ChangeGamma(boolean shift)
{
    static int  gammawait;

    if (gammawait < I_GetTime())
    {
        if (shift)
        {
            if (--gammaindex < 0)
                gammaindex = GAMMALEVELS - 1;
        }
        else
        {
            if (++gammaindex > GAMMALEVELS - 1)
                gammaindex = 0;
        }

        r_gamma = gammalevels[gammaindex];

        if (r_gamma == 1.0f)
            C_StrCVAROutput(stringize(r_gamma), "off");
        else
        {
            static char buffer[128];
            int         len;

            M_snprintf(buffer, sizeof(buffer), "%.2f", r_gamma);
            len = (int)strlen(buffer);

            if (len >= 2 && buffer[len - 1] == '0' && buffer[len - 2] == '0')
                buffer[len - 1] = '\0';

            C_StrCVAROutput(stringize(r_gamma), buffer);
        }

        gammawait = I_GetTime() + 4;
        S_StartSound(NULL, sfx_stnmov);
    }

    if (r_gamma == 1.0f)
    {
        C_Output(s_GAMMAOFF);
        HU_SetPlayerMessage(s_GAMMAOFF, false, false);
    }
    else
    {
        static char buffer[128];
        int         len;

        M_snprintf(buffer, sizeof(buffer), s_GAMMALVL, r_gamma);
        len = (int)strlen(buffer);

        if (len >= 2 && buffer[len - 1] == '0' && buffer[len - 2] == '0')
            buffer[len - 1] = '\0';

        C_Output(buffer);
        HU_SetPlayerMessage(buffer, false, false);
    }

    message_dontfuckwithme = true;
    I_SetPalette(&PLAYPAL[st_palette * 768]);
    M_SaveCVARs();
}

//
// M_Responder
//
int     gamecontrollerwait = 0;
int     mousewait = 0;
boolean gamecontrollerpress = false;

boolean M_Responder(event_t *ev)
{
    int         key = -1;
    static int  keywait;

    if (idclevtics)
        return false;

    if (ev->type == ev_controller)
    {
        if (menuactive && gamecontrollerwait < I_GetTime())
        {
            // activate menu item
            if (gamecontrollerbuttons & GAMECONTROLLER_A)
            {
                key = (messagetoprint && messageNeedsInput ? 'y' : KEY_ENTER);
                gamecontrollerwait = I_GetTime() + 8 * !(currentMenu == &OptionsDef && itemOn == 5);
                usinggamecontroller = true;
            }

            // previous/exit menu
            else if (gamecontrollerbuttons & GAMECONTROLLER_B)
            {
                key = (messagetoprint && messageNeedsInput ? 'n' : KEY_BACKSPACE);
                gamecontrollerwait = I_GetTime() + 8;
                gamecontrollerpress = true;
                usinggamecontroller = true;
            }

            // exit menu
            else if (gamecontrollerbuttons & gamecontrollermenu)
            {
                key = keyboardmenu;
                currentMenu = &MainDef;
                itemOn = MainDef.lastOn;
                gamecontrollerwait = I_GetTime() + 8;
                usinggamecontroller = true;
            }

            else if (!messagetoprint)
            {
                // select previous menu item
                if (gamecontrollerthumbLY < 0 || gamecontrollerthumbRY < 0 || (gamecontrollerbuttons & GAMECONTROLLER_DPAD_UP))
                {
                    key = KEY_UPARROW;
                    keywait = 0;
                    gamecontrollerwait = I_GetTime() + 8;
                    usinggamecontroller = true;
                }

                // select next menu item
                else if (gamecontrollerthumbLY > 0 || gamecontrollerthumbRY > 0 || (gamecontrollerbuttons & GAMECONTROLLER_DPAD_DOWN))
                {
                    key = KEY_DOWNARROW;
                    keywait = 0;
                    gamecontrollerwait = I_GetTime() + 8;
                    usinggamecontroller = true;
                }

                // decrease slider
                else if ((gamecontrollerthumbLX < 0 || gamecontrollerthumbRX < 0 || (gamecontrollerbuttons & GAMECONTROLLER_DPAD_LEFT)) && !saveStringEnter
                    && !(currentMenu == &OptionsDef && itemOn == 1))
                {
                    key = KEY_LEFTARROW;
                    gamecontrollerwait = I_GetTime() + 6 * !(currentMenu == &OptionsDef && itemOn == 5);
                    usinggamecontroller = true;
                }

                // increase slider
                else if ((gamecontrollerthumbLX > 0 || gamecontrollerthumbRX > 0 || (gamecontrollerbuttons & GAMECONTROLLER_DPAD_RIGHT)) && !saveStringEnter
                    && !(currentMenu == &OptionsDef && itemOn == 1))
                {
                    key = KEY_RIGHTARROW;
                    gamecontrollerwait = I_GetTime() + 6 * !(currentMenu == &OptionsDef && itemOn == 5);
                    usinggamecontroller = true;
                }
            }
        }
        else
        {
            // open menu
            if ((gamecontrollerbuttons & gamecontrollermenu) && gamecontrollerwait < I_GetTime())
            {
                key = keyboardmenu;
                gamecontrollerwait = I_GetTime() + 8;
                usinggamecontroller = true;
            }

            // open console
            else if ((gamecontrollerbuttons & gamecontrollerconsole) && gamecontrollerwait < I_GetTime())
            {
                gamecontrollerwait = I_GetTime() + 8;
                usinggamecontroller = true;
                C_ShowConsole();

                return false;
            }
        }
    }

    if (ev->type == ev_mouse)
    {
        if (menuactive)
        {
            // activate menu item
            if ((ev->data1 & MOUSE_LEFTBUTTON) && mousewait < I_GetTime())
            {
                key = KEY_ENTER;
                mousewait = I_GetTime() + 8;
                usinggamecontroller = false;
            }

            // previous menu
            else if ((ev->data1 & MOUSE_RIGHTBUTTON) && mousewait < I_GetTime())
            {
                key = KEY_BACKSPACE;
                mousewait = I_GetTime() + 8;
                usinggamecontroller = false;
            }
        }

        // screenshot
        if (mousescreenshot != -1 && (ev->data1 & mousescreenshot) && mousewait < I_GetTime())
        {
            mousewait = I_GetTime() + 5;
            usinggamecontroller = false;
            G_ScreenShot();
            S_StartSound(NULL, sfx_scrsht);
            memset(screens[0], nearestwhite, SCREENAREA);
            D_FadeScreen(true);

            return false;
        }
    }
    else if (ev->type == ev_mousewheel && mousewait < I_GetTime())
    {
        if (!messagetoprint)
        {
            // select previous menu item
            if (ev->data1 > 0)
            {
                key = KEY_UPARROW;
                mousewait = I_GetTime() + 3;
                usinggamecontroller = false;
            }

            // select next menu item
            else if (ev->data1 < 0)
            {
                key = KEY_DOWNARROW;
                mousewait = I_GetTime() + 3;
                usinggamecontroller = false;
            }
        }
    }
    else if (ev->type == ev_keydown)
    {
        key = ev->data1;
        usinggamecontroller = false;
    }
    else if (ev->type == ev_keyup)
    {
        keydown = 0;

        if (ev->data1 == keyboardscreenshot && (keyboardscreenshot == KEY_PRINTSCREEN || (gamestate == GS_LEVEL && !consoleactive))
            && !splashscreen)
        {
            S_StartSound(NULL, sfx_scrsht);
            memset(screens[0], nearestwhite, SCREENAREA);
            D_FadeScreen(true);
        }

        return false;
    }

    // Save Game string input
    if (saveStringEnter)
    {
        if (ev->type == ev_textinput)
        {
            int ch = toupper(ev->data1);

            if (ch >= ' ' && ch <= '_' && M_StringWidth(savegamestrings[saveSlot]) + M_CharacterWidth(ch, 0) <= SAVESTRINGPIXELWIDTH)
            {
                const int   len = (int)strlen(savegamestrings[saveSlot]);

                savegamestrings[saveSlot][len + 1] = '\0';

                for (int i = len; i > saveCharIndex; i--)
                    savegamestrings[saveSlot][i] = savegamestrings[saveSlot][i - 1];

                savegamestrings[saveSlot][saveCharIndex++] = ch;
                caretwait = I_GetTimeMS() + CARETBLINKTIME;
                showcaret = true;

                return true;
            }

            return false;
        }

        if (key == keyboardscreenshot)
        {
            G_ScreenShot();
            return false;
        }

        switch (key)
        {
            // delete character left of caret
            case KEY_BACKSPACE:
                keydown = key;

                if (saveCharIndex > 0)
                {
                    const int   len = (int)strlen(savegamestrings[saveSlot]);

                    for (int j = saveCharIndex - 1; j < len; j++)
                        savegamestrings[saveSlot][j] = savegamestrings[saveSlot][j + 1];

                    saveCharIndex--;
                    caretwait = I_GetTimeMS() + CARETBLINKTIME;
                    showcaret = true;
                }

                break;

            // delete character right of caret
            case KEY_DELETE:
            {
                const int   len = (int)strlen(savegamestrings[saveSlot]);

                keydown = key;

                if (saveCharIndex < len)
                {
                    for (int j = saveCharIndex; j < len; j++)
                        savegamestrings[saveSlot][j] = savegamestrings[saveSlot][j + 1];

                    caretwait = I_GetTimeMS() + CARETBLINKTIME;
                    showcaret = true;
                }

                break;
            }

            // cancel
            case KEY_ESCAPE:
                if (!keydown)
                {
                    keydown = key;
                    SDL_StopTextInput();
                    saveStringEnter = false;
                    caretwait = 0;
                    showcaret = false;
                    M_StringCopy(&savegamestrings[saveSlot][0], saveOldString, sizeof(savegamestrings[saveSlot]));
                    S_StartSound(NULL, sfx_swtchx);
                }

                break;

            // confirm
            case KEY_ENTER:
                if (!keydown)
                {
                    const int   len = (int)strlen(savegamestrings[saveSlot]);
                    boolean     allspaces = true;

                    keydown = key;

                    for (int i = 0; i < len; i++)
                        if (savegamestrings[saveSlot][i] != ' ')
                            allspaces = false;

                    if (savegamestrings[saveSlot][0] && !allspaces)
                    {
                        SDL_StopTextInput();
                        saveStringEnter = false;
                        caretwait = I_GetTimeMS() + CARETBLINKTIME;
                        showcaret = true;
                        M_DoSave(saveSlot);
                        D_FadeScreen(false);
                    }
                }

                break;

            // move caret left
            case KEY_LEFTARROW:
                if (saveCharIndex > 0)
                {
                    saveCharIndex--;
                    caretwait = I_GetTimeMS() + CARETBLINKTIME;
                    showcaret = true;
                }

                break;

            // move caret right
            case KEY_RIGHTARROW:
                if (saveCharIndex < (int)strlen(savegamestrings[saveSlot]))
                {
                    saveCharIndex++;
                    caretwait = I_GetTimeMS() + CARETBLINKTIME;
                    showcaret = true;
                }

                break;

            // move caret to start
            case KEY_HOME:
                if (saveCharIndex > 0)
                {
                    saveCharIndex = 0;
                    caretwait = I_GetTimeMS() + CARETBLINKTIME;
                    showcaret = true;
                }

                break;

            // move caret to end
            case KEY_END:
            {
                const int   len = (int)strlen(savegamestrings[saveSlot]);

                if (saveCharIndex < len)
                {
                    saveCharIndex = len;
                    caretwait = I_GetTimeMS() + CARETBLINKTIME;
                    showcaret = true;
                }

                break;
            }
        }

        return true;
    }

    // Take care of any messages that need input
    if (messagetoprint && !keydown)
    {
        int ch = (key == KEY_ENTER ? 'y' : tolower(key));

        if (messageNeedsInput && key != keyboardmenu && ch != 'y' && ch != 'n' && key != KEY_BACKSPACE
            && !(SDL_GetModState() & (KMOD_ALT | KMOD_CTRL)) && key != functionkey)
        {
            functionkey = 0;
            return false;
        }

        keydown = key;
        menuactive = messageLastMenuActive;
        messagetoprint = false;

        if (messageRoutine)
            messageRoutine(ch);

        functionkey = 0;

        if (endinggame)
            endinggame = false;
        else
        {
            S_StartSound(NULL, (currentMenu == &ReadDef ? sfx_pistol : sfx_swtchx));
            D_FadeScreen(false);
        }

        return true;
    }

    // F-Keys
    if ((!menuactive || functionkey) && !paused && !splashscreen)
    {
        // Screen size down
        if (key == KEY_MINUS)
        {
            keydown = key;

            if (automapactive || inhelpscreens || gamestate == GS_INTERMISSION || gamestate == GS_FINALE)
                return false;

            if (viewactive)
                M_SizeDisplay(0);
            else if (vid_widescreen)
            {
                vid_widescreen = false;
                r_screensize = r_screensize_max - 1;
                r_hud = false;
                pagetic = PAGETICS;
                R_SetViewSize(r_screensize);
                I_RestartGraphics(false);
                S_StartSound(NULL, sfx_stnmov);
            }

            return false;
        }

        // Screen size up
        else if (key == KEY_EQUALS)
        {
            keydown = key;

            if (automapactive || inhelpscreens || gamestate == GS_INTERMISSION || gamestate == GS_FINALE)
                return false;

            if (viewactive)
                M_SizeDisplay(1);
            else if (!vid_widescreen && !nowidescreen)
            {
                vid_widescreen = true;
                r_screensize = r_screensize_max - 1;
                pagetic = PAGETICS;
                R_SetViewSize(r_screensize);
                I_RestartGraphics(false);
                S_StartSound(NULL, sfx_stnmov);
            }

            return false;
        }

        // Console
        else if (key == keyboardconsole && !keydown)
        {
            keydown = key;

            if (consoleheight < CONSOLEHEIGHT && consoledirection == -1 && !inhelpscreens && !dowipe)
            {
                C_ShowConsole();
                return true;
            }

            return false;
        }

        // Help key
        else if (key == KEY_F1 && (!functionkey || functionkey == KEY_F1) && !keydown)
        {
            if (gamestate == GS_INTERMISSION || gamestate == GS_FINALE)
                return false;

            keydown = key;

            if (functionkey == KEY_F1)
            {
                functionkey = 0;
                M_ClearMenus();
                S_StartSound(NULL, sfx_swtchx);
                D_FadeScreen(false);

                if (inhelpscreens)
                    R_SetViewSize(r_screensize);
            }
            else
                M_ShowHelp(0);

            return true;
        }

        // Save
        else if (key == KEY_F2 && (!functionkey || functionkey == KEY_F2) && (viewactive || automapactive)
            && !keydown && viewplayer->health > 0)
        {
            keydown = key;

            if (functionkey == KEY_F2)
            {
                functionkey = 0;
                currentMenu->lastOn = itemOn;
                M_ClearMenus();
                S_StartSound(NULL, sfx_swtchx);
            }
            else
            {
                functionkey = KEY_F2;
                M_StartControlPanel();
                itemOn = currentMenu->lastOn;
                S_StartSound(NULL, sfx_swtchn);
                M_SaveGame(0);
            }

            return true;
        }

        // Load
        else if (key == KEY_F3 && (!functionkey || functionkey == KEY_F3) && savegames && !keydown)
        {
            keydown = key;

            if (functionkey == KEY_F3)
            {
                functionkey = 0;
                currentMenu->lastOn = itemOn;
                M_ClearMenus();
                S_StartSound(NULL, sfx_swtchx);
            }
            else
            {
                functionkey = KEY_F3;
                M_StartControlPanel();
                itemOn = currentMenu->lastOn;
                S_StartSound(NULL, sfx_swtchn);
                M_LoadGame(0);
            }

            return true;
        }

        else if (key == KEY_F4 && (!functionkey || functionkey == KEY_F4) && !keydown)
        {
            keydown = key;

            // Sound Volume
            if (functionkey == KEY_F4)
            {
                functionkey = 0;
                currentMenu->lastOn = itemOn;
                M_ClearMenus();
                S_StartSound(NULL, sfx_swtchx);
            }
            else
            {
                functionkey = KEY_F4;
                M_StartControlPanel();
                currentMenu = &SoundDef;
                itemOn = currentMenu->lastOn;
                S_StartSound(NULL, sfx_swtchn);
            }

            return true;
        }

        // Quicksave
        else if (key == KEY_F6 && (!functionkey || functionkey == KEY_F6) && (viewactive || automapactive)
            && !keydown && viewplayer->health > 0)
        {
            keydown = key;

            if (quickSaveSlot >= 0)
                functionkey = KEY_F6;

            M_QuickSave();
            return true;
        }

        // End game
        else if (key == KEY_F7 && !functionkey && (viewactive || automapactive) && !keydown)
        {
            keydown = key;
            functionkey = KEY_F7;
            M_StartControlPanel();
            S_StartSound(NULL, sfx_swtchn);
            M_EndGame(0);

            return true;
        }

        // Toggle messages
        else if (key == KEY_F8 && !functionkey && (viewactive || automapactive) && !keydown)
        {
            keydown = key;
            functionkey = KEY_F8;
            M_ChangeMessages(0);
            functionkey = 0;
            S_StartSound(NULL, sfx_swtchn);

            return false;
        }

        // Quickload
        else if (key == KEY_F9 && !functionkey && (viewactive || automapactive) && savegames && !keydown)
        {
            keydown = key;
            functionkey = KEY_F9;
            M_QuickLoad();

            return true;
        }

        // Quit DOOM Retro
        else if (key == KEY_F10 && !keydown)
        {
            keydown = key;
            functionkey = KEY_F10;
            M_StartControlPanel();
            S_StartSound(NULL, sfx_swtchn);
            M_QuitDOOM(0);

            return true;
        }
    }

    // Toggle graphic detail
    if (key == KEY_F5 && !functionkey && (r_screensize < r_screensize_max || !automapactive) && !keydown)
    {
        keydown = key;
        functionkey = KEY_F5;
        M_ChangeDetail(0);
        functionkey = 0;
        S_StartSound(NULL, sfx_swtchn);

        return false;
    }

    // gamma toggle
    if (key == KEY_F11)
    {
        M_ChangeGamma(SDL_GetModState() & KMOD_SHIFT);
        return false;
    }

    // screenshot
    if (key == keyboardscreenshot && (keyboardscreenshot == KEY_PRINTSCREEN || gamestate == GS_LEVEL) && !splashscreen)
    {
        G_ScreenShot();
        return false;
    }

    // Pop-up menu?
    if (!menuactive)
    {
        if (key == keyboardmenu && !keydown && !splashscreen && !consoleactive)
        {
            keydown = key;

            if (paused)
            {
                paused = false;
                S_ResumeMusic();
                S_StartSound(NULL, sfx_swtchx);
                I_SetPalette(&PLAYPAL[st_palette * 768]);
            }
            else
            {
                M_StartControlPanel();
                S_StartSound(NULL, sfx_swtchn);
            }
        }

        return false;
    }

    if (!paused)
    {
        if (key == KEY_DOWNARROW && keywait < I_GetTime() && !inhelpscreens)
        {
            // Move down to next item
            if (currentMenu == &LoadDef)
            {
                int old = itemOn;

                do
                {
                    if (itemOn + 1 > currentMenu->numitems - 1)
                        itemOn = 0;
                    else
                        itemOn++;
                } while (M_StringCompare(savegamestrings[itemOn], s_EMPTYSTRING));

                if (itemOn != old)
                    S_StartSound(NULL,  sfx_pstop);

                SaveDef.lastOn = itemOn;
                savegame = itemOn + 1;
                M_SaveCVARs();
                C_IntCVAROutput(stringize(savegame), savegame);
            }
            else
            {
                do
                {
                    if (itemOn + 1 > currentMenu->numitems - 1)
                        itemOn = 0;
                    else
                        itemOn++;

                    if (currentMenu == &MainDef)
                    {
                        if (itemOn == 2 && !savegames)
                            itemOn++;

                        if (itemOn == 3 && (gamestate != GS_LEVEL || viewplayer->health <= 0))
                            itemOn++;
                    }
                    else if (currentMenu == &OptionsDef && !itemOn && gamestate != GS_LEVEL)
                        itemOn++;

                    if (currentMenu->menuitems[itemOn].status != -1)
                        S_StartSound(NULL, sfx_pstop);
                } while (currentMenu->menuitems[itemOn].status == -1);
            }

            currentMenu->change = true;

            if (currentMenu == &EpiDef && gamemode != shareware && !EpiCustom)
            {
                episode = itemOn + 1;
                M_SaveCVARs();
                C_IntCVAROutput(stringize(episode), episode);
            }
            else if (currentMenu == &ExpDef)
            {
                expansion = itemOn + 1;
                M_SaveCVARs();
                C_IntCVAROutput(stringize(expansion), expansion);

                if (gamestate != GS_LEVEL)
                    gamemission = (expansion == 2 && nerve ? pack_nerve : doom2);
            }
            else if (currentMenu == &NewDef)
            {
                skilllevel = itemOn + 1;
                M_SaveCVARs();
                C_IntCVAROutput(stringize(skilllevel), skilllevel);
            }
            else if (currentMenu == &SaveDef)
            {
                LoadDef.lastOn = itemOn;
                savegame = itemOn + 1;
                M_SaveCVARs();
                C_IntCVAROutput(stringize(savegame), savegame);
            }

            keywait = I_GetTime() + 2;
            M_SetWindowCaption();

            return false;
        }
        else if (key == KEY_UPARROW && keywait < I_GetTime() && !inhelpscreens)
        {
            // Move back up to previous item
            if (currentMenu == &LoadDef)
            {
                int old = itemOn;

                do
                {
                    if (!itemOn)
                        itemOn = currentMenu->numitems - 1;
                    else
                        itemOn--;
                } while (M_StringCompare(savegamestrings[itemOn], s_EMPTYSTRING));

                if (itemOn != old)
                    S_StartSound(NULL, sfx_pstop);

                SaveDef.lastOn = itemOn;
                savegame = itemOn + 1;
                M_SaveCVARs();
                C_IntCVAROutput(stringize(savegame), savegame);
            }
            else
            {
                do
                {
                    if (!itemOn)
                        itemOn = currentMenu->numitems - 1;
                    else
                        itemOn--;

                    if (currentMenu == &MainDef)
                    {
                        if (itemOn == 3 && (gamestate != GS_LEVEL || viewplayer->health <= 0))
                            itemOn--;

                        if (itemOn == 2 && !savegames)
                            itemOn--;
                    }
                    else if (currentMenu == &OptionsDef && !itemOn && gamestate != GS_LEVEL)
                        itemOn = currentMenu->numitems - 1;

                    if (currentMenu->menuitems[itemOn].status != -1)
                        S_StartSound(NULL, sfx_pstop);
                } while (currentMenu->menuitems[itemOn].status == -1);
            }

            currentMenu->change = true;

            if (currentMenu == &EpiDef && gamemode != shareware && !EpiCustom)
            {
                episode = itemOn + 1;
                M_SaveCVARs();
                C_IntCVAROutput(stringize(episode), episode);
            }
            else if (currentMenu == &ExpDef)
            {
                expansion = itemOn + 1;
                M_SaveCVARs();
                C_IntCVAROutput(stringize(expansion), expansion);

                if (gamestate != GS_LEVEL)
                    gamemission = (expansion == 2 && nerve ? pack_nerve : doom2);
            }
            else if (currentMenu == &NewDef)
            {
                skilllevel = itemOn + 1;
                M_SaveCVARs();
                C_IntCVAROutput(stringize(skilllevel), skilllevel);
            }
            else if (currentMenu == &SaveDef)
            {
                LoadDef.lastOn = itemOn;
                savegame = itemOn + 1;
                M_SaveCVARs();
                C_IntCVAROutput(stringize(savegame), savegame);
            }

            keywait = I_GetTime() + 2;
            M_SetWindowCaption();

            return false;
        }

        else if ((key == KEY_LEFTARROW || (key == KEY_MINUS && !(currentMenu == &OptionsDef && itemOn == 1))) && !inhelpscreens)
        {
            // Slide slider left
            if (currentMenu->menuitems[itemOn].routine && currentMenu->menuitems[itemOn].status == 2)
                currentMenu->menuitems[itemOn].routine(0);
            else if (currentMenu == &OptionsDef && (itemOn == 1 || itemOn == 2) && !keydown)
            {
                keydown = key;
                currentMenu->menuitems[itemOn].routine(itemOn);
                S_StartSound(NULL, sfx_pistol);
            }

            return false;
        }

        else if ((key == KEY_RIGHTARROW || (key == KEY_EQUALS && !(currentMenu == &OptionsDef && itemOn == 1))) && !inhelpscreens)
        {
            // Slide slider right
            if (currentMenu->menuitems[itemOn].routine && currentMenu->menuitems[itemOn].status == 2)
                currentMenu->menuitems[itemOn].routine(1);
            else if (currentMenu == &OptionsDef && (itemOn == 1 || itemOn == 2) && !keydown)
            {
                keydown = key;
                currentMenu->menuitems[itemOn].routine(itemOn);
                S_StartSound(NULL, sfx_pistol);
            }

            return false;
        }

        else if (key == KEY_ENTER && keywait < I_GetTime() && !keydown && !fadecount)
        {
            // Activate menu item
            keydown = key;

            if (inhelpscreens)
            {
                functionkey = 0;
                M_ClearMenus();
                S_StartSound(NULL, sfx_swtchx);
                D_FadeScreen(false);
                R_SetViewSize(r_screensize);

                return true;
            }

            if (currentMenu->menuitems[itemOn].routine && currentMenu->menuitems[itemOn].status)
            {
                if (gamemode != shareware || currentMenu != &EpiDef)
                    currentMenu->lastOn = itemOn;

                if (currentMenu->menuitems[itemOn].status == 2)
                    currentMenu->menuitems[itemOn].routine(1);
                else
                {
                    if (gamestate != GS_LEVEL && ((currentMenu == &MainDef && itemOn == 3) || (currentMenu == &OptionsDef && !itemOn)))
                        return true;

                    if (currentMenu != &LoadDef)
                    {
                        if (currentMenu != &NewDef || itemOn == 4)
                            S_StartSound(NULL, sfx_pistol);

                        if (currentMenu != &NewDef && currentMenu != &SaveDef && (currentMenu != &OptionsDef || itemOn == 2))
                            D_FadeScreen(false);
                    }

                    currentMenu->menuitems[itemOn].routine(itemOn);
                }
            }

            if (currentMenu == &EpiDef && !EpiCustom)
                C_IntCVAROutput(stringize(episode), episode);
            else if (currentMenu == &ExpDef)
                C_IntCVAROutput(stringize(expansion), expansion);
            else if (currentMenu == &NewDef)
                C_IntCVAROutput(stringize(skilllevel), skilllevel);

            M_SetWindowCaption();
            skipaction = (currentMenu == &LoadDef || currentMenu == &SaveDef || currentMenu == &NewDef);
            keywait = I_GetTime() + 5;

            return skipaction;
        }

        else if ((key == keyboardmenu || key == KEY_BACKSPACE) && !keydown)
        {
            // Deactivate menu or go back to previous menu
            keydown = key;

            if (gamemode != shareware || currentMenu != &EpiDef)
                currentMenu->lastOn = itemOn;

            if (currentMenu->prevMenu && !functionkey)
            {
                currentMenu = currentMenu->prevMenu;
                itemOn = currentMenu->lastOn;
                S_StartSound(NULL, sfx_swtchn);
            }
            else
            {
                functionkey = 0;
                M_ClearMenus();
                S_StartSound(NULL, sfx_swtchx);
                gamecontrollerbuttons = 0;
                ev->data1 = 0;
                firstevent = true;
            }

            D_FadeScreen(false);

            if (inhelpscreens)
                R_SetViewSize(r_screensize);

            M_SetWindowCaption();
            return true;
        }

        else if (key == KEY_DELETE && !keydown && (currentMenu == &LoadDef || currentMenu == &SaveDef))
        {
            // Delete a savegame
            keydown = key;

            if (LoadGameMenu[itemOn].status)
            {
                M_DeleteSavegame();
                return true;
            }
            else
                S_StartSound(NULL, sfx_oof);

            return false;
        }

        // Keyboard shortcut?
        else if (key && !(SDL_GetModState() & (KMOD_ALT | KMOD_CTRL)))
        {
            for (int i = itemOn + 1; i < currentMenu->numitems; i++)
            {
                if (((currentMenu == &LoadDef || currentMenu == &SaveDef) && key == i + '1')
                    || (currentMenu->menuitems[i].text && toupper(*currentMenu->menuitems[i].text[0]) == toupper(key)))
                {
                    if (currentMenu == &MainDef)
                    {
                        if ((i == 2 && !savegames) || (i == 3 && (gamestate != GS_LEVEL || viewplayer->health <= 0)))
                            return true;
                    }
                    else if (currentMenu == &OptionsDef && !i && gamestate != GS_LEVEL)
                        return true;
                    else if (currentMenu == &LoadDef && M_StringCompare(savegamestrings[i], s_EMPTYSTRING))
                        return true;

                    if (itemOn != i)
                        S_StartSound(NULL, sfx_pstop);

                    itemOn = i;
                    currentMenu->change = true;

                    if (currentMenu == &EpiDef && gamemode != shareware && !EpiCustom)
                    {
                        episode = itemOn + 1;
                        M_SaveCVARs();
                        C_IntCVAROutput(stringize(episode), episode);
                    }
                    else if (currentMenu == &ExpDef)
                    {
                        expansion = itemOn + 1;
                        M_SaveCVARs();
                        C_IntCVAROutput(stringize(expansion), expansion);

                        if (gamestate != GS_LEVEL)
                            gamemission = (expansion == 2 && nerve ? pack_nerve : doom2);
                    }
                    else if (currentMenu == &NewDef)
                    {
                        skilllevel = itemOn + 1;
                        M_SaveCVARs();
                        C_IntCVAROutput(stringize(skilllevel), skilllevel);
                    }
                    else if (currentMenu == &SaveDef)
                    {
                        LoadDef.lastOn = itemOn;
                        savegame = itemOn + 1;
                        M_SaveCVARs();
                        C_IntCVAROutput(stringize(savegame), savegame);
                    }
                    else if (currentMenu == &LoadDef)
                    {
                        SaveDef.lastOn = itemOn;
                        savegame = itemOn + 1;
                        M_SaveCVARs();
                        C_IntCVAROutput(stringize(savegame), savegame);
                    }

                    M_SetWindowCaption();
                    return false;
                }
            }

            for (int i = 0; i <= itemOn; i++)
            {
                if (((currentMenu == &LoadDef || currentMenu == &SaveDef) && key == i + '1')
                    || (currentMenu->menuitems[i].text && toupper(*currentMenu->menuitems[i].text[0]) == toupper(key)))
                {
                    if (currentMenu == &MainDef)
                    {
                        if ((i == 2 && !savegames) || (i == 3 && (gamestate != GS_LEVEL || viewplayer->health <= 0)))
                            return true;
                    }
                    else if (currentMenu == &OptionsDef && !i && gamestate != GS_LEVEL)
                        return true;
                    else if (currentMenu == &LoadDef && M_StringCompare(savegamestrings[i], s_EMPTYSTRING))
                        return true;

                    if (itemOn != i)
                        S_StartSound(NULL, sfx_pstop);

                    itemOn = i;
                    currentMenu->change = true;

                    if (currentMenu == &EpiDef && gamemode != shareware && !EpiCustom)
                    {
                        episode = itemOn + 1;
                        M_SaveCVARs();
                        C_IntCVAROutput(stringize(episode), episode);
                    }
                    else if (currentMenu == &ExpDef)
                    {
                        expansion = itemOn + 1;
                        M_SaveCVARs();
                        C_IntCVAROutput(stringize(expansion), expansion);

                        if (gamestate != GS_LEVEL)
                            gamemission = (expansion == 2 && nerve ? pack_nerve : doom2);
                    }
                    else if (currentMenu == &NewDef)
                    {
                        skilllevel = itemOn + 1;
                        M_SaveCVARs();
                        C_IntCVAROutput(stringize(skilllevel), skilllevel);
                    }
                    else if (currentMenu == &SaveDef)
                    {
                        LoadDef.lastOn = itemOn;
                        savegame = itemOn + 1;
                        M_SaveCVARs();
                        C_IntCVAROutput(stringize(savegame), savegame);
                    }
                    else if (currentMenu == &LoadDef)
                    {
                        SaveDef.lastOn = itemOn;
                        savegame = itemOn + 1;
                        M_SaveCVARs();
                        C_IntCVAROutput(stringize(savegame), savegame);
                    }

                    M_SetWindowCaption();
                    return false;
                }
            }
        }
    }

    return false;
}

//
// M_StartControlPanel
//
void M_StartControlPanel(void)
{
    // intro might call this repeatedly
    if (menuactive)
        return;

    menuactive = true;
    currentMenu = &MainDef;
    itemOn = currentMenu->lastOn;

    if (joy_rumble_barrels || joy_rumble_damage || joy_rumble_weapons)
    {
        restoredrumblestrength = idlechainsawrumblestrength;
        idlechainsawrumblestrength = 0;
        I_StopGameControllerRumble();
    }

    viewplayer->fixedcolormap = 0;
    I_SetPalette(PLAYPAL);
    I_UpdateBlitFunc(false);
    S_StopSounds();

    if (vid_motionblur)
        I_SetMotionBlur(0);

    if (gamestate == GS_LEVEL)
    {
        playerangle = viewplayer->mo->angle;
        spinspeed = 0;

        if (automapactive)
        {
            AM_SetAutomapSize(r_screensize_max);

            if (!am_rotatemode)
                viewplayer->mo->angle = ANG90;
        }
        else
            R_SetViewSize(r_screensize_max);
    }

    S_LowerMusicVolume();
    D_FadeScreen(false);
}

//
// M_DrawNightmare
//
static void M_DrawNightmare(void)
{
    for (int y = 0; y < 20; y++)
        for (int x = 0; x < 124; x++)
            V_DrawPixel(NewDef.x + x, NewDef.y + OFFSET + 16 * nightmare + y, (int)nmare[y * 124 + x], true);
}

//
// M_Drawer
// Called after the view has been rendered,
// but before it has been blitted.
//
void M_Drawer(void)
{
    static short    x, y;

    // Center string and print it.
    if (messagetoprint)
    {
        char    string[255];
        int     start = 0;

        M_DarkBackground();

        y = (VANILLAHEIGHT - M_StringHeight(messageString)) / 2;

        while (messageString[start] != '\0')
        {
            const int   len = (int)strlen(messageString + start);
            boolean     foundnewline = false;

            for (int i = 0; i < len; i++)
                if (messageString[start + i] == '\n')
                {
                    M_StringCopy(string, messageString + start, sizeof(string));

                    if (i < sizeof(string))
                        string[i] = '\0';

                    foundnewline = true;
                    start += i + 1;

                    break;
                }

            if (!foundnewline)
            {
                M_StringCopy(string, messageString + start, sizeof(string));
                start += (int)strlen(string);
            }

            if (*string)
            {
                M_WriteText((VANILLAWIDTH - M_StringWidth(string)) / 2, y, string, true);
                y += (STCFN034 ? SHORT(hu_font[0]->height) + 1 : 8) + 1;
            }
            else
                y += 3;
        }

        return;
    }

    if (!menuactive)
    {
        inhelpscreens = false;
        return;
    }

    if (currentMenu->routine)
        currentMenu->routine();         // call draw routine

    // DRAW MENU
    x = currentMenu->x;
    y = currentMenu->y;

    if (currentMenu != &ReadDef)
    {
        // DRAW SKULL
        char    *skullname[] = { "M_SKULL1", "M_SKULL2" };
        patch_t *skullpatch = W_CacheLumpName(skullname[whichSkull]);

        if (currentMenu == &LoadDef || currentMenu == &SaveDef)
        {
            if (currentMenu == &LoadDef)
            {
                int old = itemOn;

                while (M_StringCompare(savegamestrings[itemOn], s_EMPTYSTRING))
                    itemOn = (!itemOn ? currentMenu->numitems - 1 : itemOn - 1);

                if (itemOn != old)
                {
                    SaveDef.lastOn = itemOn;
                    savegame = itemOn + 1;
                    M_SaveCVARs();
                    C_IntCVAROutput(stringize(savegame), savegame);
                }
            }

            if (M_SKULL1)
                M_DrawPatchWithShadow(x - 43, y + itemOn * LINEHEIGHT - 8 + OFFSET + (chex ? 1 : 0), skullpatch);
            else
                M_DrawPatchWithShadow(x - 37, y + itemOn * LINEHEIGHT - 7 + OFFSET, skullpatch);
        }
        else
        {
            int yy = y + itemOn * (LINEHEIGHT - 1) - 5 + OFFSET + (chex ? 1 : 0);
            int max = currentMenu->numitems;

            if (currentMenu == &OptionsDef && !itemOn && gamestate != GS_LEVEL)
                itemOn++;

            if (M_SKULL1)
                M_DrawPatchWithShadow(x - 30, yy, skullpatch);
            else
                M_DrawPatchWithShadow(x - 26, yy + 2, skullpatch);

            for (int i = 0; i < max; i++)
            {
                if (currentMenu->menuitems[i].routine)
                {
                    char    *name = currentMenu->menuitems[i].name;
                    char    **text = currentMenu->menuitems[i].text;

                    if (M_StringCompare(name, "M_EPI5") && sigil)
                    {
                        patch_t *patch = W_CacheLumpName(name);

                        M_DrawPatchWithShadow(x, y + OFFSET, patch);
                    }
                    else if (M_StringCompare(name, "M_NMARE"))
                    {
                        if (M_NMARE)
                        {
                            patch_t *patch = W_CacheLumpName(name);

                            M_DrawPatchWithShadow(x, y + OFFSET, patch);
                        }
                        else
                            M_DrawNightmare();
                    }
                    else if (M_StringCompare(name, "M_MSENS") && !M_MSENS)
                        M_DrawString(x, y + OFFSET, (usinggamecontroller ? s_M_GAMECONTROLLERSENSITIVITY : s_M_MOUSESENSITIVITY));
                    else if (W_CheckNumForName(name) < 0 && **text)   // Custom Episode
                        M_DrawString(x, y + OFFSET, *text);
                    else if (W_CheckMultipleLumps(name) > 1 || lumpinfo[W_GetNumForName(name)]->wadfile->type == PWAD)
                    {
                        patch_t *patch = W_CacheLumpName(name);

                        M_DrawPatchWithShadow(x, y + OFFSET, patch);
                    }
                    else if (**text)
                        M_DrawString(x, y + OFFSET, *text);
                }

                y += LINEHEIGHT - 1;
            }
        }
    }
}

//
// M_ClearMenus
//
void M_ClearMenus(void)
{
    if (!menuactive)
        return;

    menuactive = false;
    blurtic = -1;

    if (joy_rumble_barrels || joy_rumble_damage || joy_rumble_weapons)
    {
        idlechainsawrumblestrength = restoredrumblestrength;
        I_GameControllerRumble(idlechainsawrumblestrength);
    }

    if (gamestate == GS_LEVEL)
    {
        I_SetPalette(&PLAYPAL[st_palette * 768]);

        viewplayer->mo->angle = playerangle;

        if (!inhelpscreens)
            R_SetViewSize(r_screensize);

        AM_SetAutomapSize(r_screensize);
    }

    S_SetMusicVolume(musicVolume * MIX_MAX_VOLUME / 31);
}

//
// M_SetupNextMenu
//
static void M_SetupNextMenu(menu_t *menudef)
{
    currentMenu = menudef;
    itemOn = currentMenu->lastOn;
    whichSkull = 0;
}

//
// M_Ticker
//
void M_Ticker(void)
{
    if ((!saveStringEnter || !whichSkull) && windowfocused && --skullAnimCounter <= 0)
    {
        whichSkull ^= 1;
        skullAnimCounter = 8;
    }
}

//
// M_Init
//
void M_Init(void)
{
    M_BigSeed((unsigned int)time(NULL));

    currentMenu = &MainDef;
    menuactive = false;
    itemOn = currentMenu->lastOn;
    skullAnimCounter = 10;
    messagetoprint = false;
    messageString = NULL;
    messageLastMenuActive = false;
    quickSaveSlot = -1;
    spindirection = ((M_Random() & 1) ? 1 : -1);

    for (int i = 0; i < 256; i++)
        blues[i] = nearestcolors[blues[i]];

    if (autostart)
    {
        episode = startepisode;
        skilllevel = startskill + 1;
    }

    if (gamemode != shareware)
        EpiDef.lastOn = episode - 1;

    ExpDef.lastOn = expansion - 1;
    NewDef.lastOn = skilllevel - 1;
    SaveDef.lastOn = LoadDef.lastOn = savegame - 1;

    if (!*savegamestrings[SaveDef.lastOn])
        while (SaveDef.lastOn && !*savegamestrings[SaveDef.lastOn])
            SaveDef.lastOn--;

    OptionsDef.lastOn = msgs;
    M_ReadSaveStrings();

    if (chex)
    {
        MainDef.x += 20;
        NewDef.prevMenu = &MainDef;
    }
    else if (gamemode == commercial)
        NewDef.prevMenu = (nerve ? &ExpDef : &MainDef);
    else if (gamemode == registered || W_CheckNumForName("E4M1") < 0)
        EpiDef.numitems = 3;
    else if (gamemode == retail && sigil)
        EpiDef.numitems = 5;
    else
        EpiDef.numitems = 4;

    if (EpiDef.lastOn > EpiDef.numitems)
    {
        EpiDef.lastOn = 0;
        episode = 1;
        M_SaveCVARs();
    }

    if (M_StringCompare(s_EMPTYSTRING, "null data"))
        s_EMPTYSTRING = "-";
}
