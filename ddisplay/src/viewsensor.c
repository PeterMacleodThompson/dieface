/****************** display sensor data ***************************
   for X86 compile with
gcc -g  viewsensor.c SDL2init.c -o viewsensorX86 -lSDL2 -lSDL2_image -lSDL2_ttf
-lrt

for ARM BBB compile with (note -D BBB)
export PATH=$PATH:$HOME/bbb2018/buildroot/output/host/bin ## for compiler
arm-linux-gnueabihf-gcc -g -D BBB -o viewsensorARM viewsensor.c SDL2init.c  -L
/home/peter/bbb2018/buildroot/output/staging/usr/lib/ -lSDL2 -lSDL2_image
-lSDL2_ttf -lrt


   for ARM IGEPv2 compile with [[ OLD ]]
export PATH=$PATH:/usr/local/xtools/arm-unknown-linux-gnueabi/bin/
export PKG_CONFIG_PATH=/home/peter/igep2015/pmtstaging/usr/lib/pkgconfig/
arm-linux-gcc -g -o viewsensorARM viewsensorARM.c SDL2init.c
~/Documents/daemons2016/pmtdiefaced/src/dietable.c
--sysroot=/home/peter/igep2015/pmtstaging/   -lSDL2 -lSDL2_image -lSDL2_ttf
$(pkg-config --libs --cflags sdl2) $(pkg-config --libs --cflags SDL2_image)
$(pkg-config --libs --cflags SDL2_ttf)

*/

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include "../../pmtdiefaced/src/include/dieface.h"
#include "../../pmtfxosd/src/include/fxos8700.h" // for  PMT structures
#include "../../pmtgpsd/src/include/pmtgps.h" // NMEA sentences, PMT structures

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <time.h>

#define NAME1 "/pmtfxos"
#define SIZE1 (sizeof(struct PMTfxos8700))
#define NAME2 "/pmtgps" //  for shared memory /dev/shm/pmtgps
#define SIZE2 (sizeof(struct PMTgps))
#define NAME3 "/pmtdieface"
#define SIZE3 (sizeof(struct PMTdieEvent))

#define VIEWSENSORPATH "./images/viewsensor.png"
#define HAPPYFACE "./images/happyface.png"

#define TRUE 1
#define FALSE 0

/* GLOBAL VARIABLES initialized by SDL2init.c ==> PMTstandard */
extern SDL_Window *globalwindow;     // The window we'll be rendering to
extern SDL_Renderer *globalrenderer; // The window renderer */
extern SDL_Texture *globaltexture;   // texture for display window

/* function declarations */
int initSDL2(void);
void closeSDL2(void);
char *dietable(long long);

static SDL_Texture *texttexture; /*line of text =global in this program */

/* function to render character string into SDL2 texture */
SDL_Texture *rendertext(TTF_Font *font, char *text, SDL_Rect *place) {
  SDL_Surface *textsurface; /* line of text */

  SDL_Color black = {0, 0, 0};

  // Render text to its own surface
  textsurface = TTF_RenderText_Solid(font, text, black);
  if (textsurface == NULL)
    printf("RenderText Error %s\n", SDL_GetError());

  /* then convert text surface to a texture - free it from before first*/
  SDL_DestroyTexture(texttexture);
  texttexture = SDL_CreateTextureFromSurface(globalrenderer, textsurface);
  SDL_FreeSurface(textsurface); /* no memory leaks allowed */

  /* put text size width, height into placement rectangle */
  SDL_QueryTexture(texttexture, NULL, NULL, &place->w, &place->h);

  return (texttexture);
}

void main() {

  int i;
  int fd1, fd2, fd3; /*/dev/pmtgps /dev/pmtfxos /dev/pmtdieEvent */
  char inchar;
  struct PMTgps *gpsnow;       // shared memory structure
  struct PMTfxos8700 *fxosnow; // shared memory structure
  struct PMTdieEvent *dienow;
  struct tm tm;

  time_t t;
  SDL_Surface *image; /* png image to disp lay */
  SDL_Surface *happy; /* png happy face for fingerdown */
  SDL_Texture *happytexture;

  SDL_Event e; /* union of all input events /dev/input/ */

  int err;
  int quit;

  TTF_Font *font;
  SDL_Color black = {0, 0, 0};
  char text[100];

  SDL_Rect postime = {125, 56, 0, 0};   /* textposition on screen */
  SDL_Rect poslong = {50, 125, 0, 0};   /* textposition on screen */
  SDL_Rect poslat = {260, 125, 0, 0};   /* textposition on screen */
  SDL_Rect pospitch = {110, 185, 0, 0}; /* textposition on screen */
  SDL_Rect posroll = {250, 185, 0, 0};  /* textposition on screen */
  SDL_Rect posyaw = {405, 185, 0, 0};
  SDL_Rect posaltitude = {140, 290, 0, 0};
  SDL_Rect posdiefaceUSS = {410, 290, 0, 0};
  SDL_Rect posdiefaceSS = {170, 350, 0, 0};
  SDL_Rect posdieaction = {270, 350, 0, 0};
  SDL_Rect posecompass = {175, 430, 0, 0};
  SDL_Rect poshappy = {250, 250, 50, 50};

  int inMotionF = FALSE; /* my finger is up/down */

  /* Initialize SDL2*/
  if (!initSDL2())
    exit(0);

  /* initialize daemon access fxosd */
  fd1 = shm_open(NAME1, O_RDONLY, 0666);
  if (fd1 < 0) {
    perror("shm_open()");
    return;
  }
  fxosnow = mmap(0, SIZE1, PROT_READ, MAP_SHARED, fd1, 0);

  /* initialize daemon access - pmtgpsd */
  fd2 = shm_open(NAME2, O_RDONLY, 0666);
  if (fd2 < 0) {
    perror("shm_open()");
    return;
  }
  gpsnow = mmap(0, SIZE2, PROT_READ, MAP_SHARED, fd2, 0);

  /* initialize daemon access dieface */
  fd3 = shm_open(NAME3, O_RDONLY, 0666);
  if (fd3 < 0) {
    perror("shm_open()");
    return;
  }
  dienow = mmap(0, SIZE3, PROT_READ, MAP_SHARED, fd3, 0);

  /* load sensor background image from png file */
  image = IMG_Load(VIEWSENSORPATH);
  if (image == NULL)
    printf("unable to load viewsensor.png\n");

  /* load happyface.png from png file & convert to texture */
  happy = IMG_Load(HAPPYFACE);
  if (happy == NULL)
    printf("unable to load happyface.png\n");
  SDL_SetColorKey(happy, SDL_TRUE, SDL_MapRGB(happy->format, 0xFF, 0xFF, 0xFF));
  happytexture = SDL_CreateTextureFromSurface(globalrenderer, happy);
  SDL_FreeSurface(happy); /* no memory leaks allowed */

  /*initialize specific font
          -see /usr/share/fonts/ for list of available fonts */
  font = TTF_OpenFont("/usr/share/fonts/truetype/freefont/FreeSans.ttf", 28);
  if (font == NULL)
    printf("fonterror\n");

  /* the BIG LOOP */
  quit = FALSE;
  while (!quit) {
    /*************** EVENTS *******************/
    /*Handle events on SDL_Event union stack*/
    while (SDL_PollEvent(&e) != 0) {
      if (e.type == SDL_QUIT)
        quit = TRUE;

      if (e.type == SDL_FINGERDOWN) {
        poshappy.x = e.tfinger.x - 50;
        poshappy.y = e.tfinger.y - 50;
        inMotionF = TRUE;
      }
      if (e.type == SDL_FINGERUP)
        inMotionF = FALSE;

      /* handle other events */
    }

    /******************* RENDER ***************************/
    /* present image on display via 4 finger salute below */

    /* 1-clear entire screen to default colour */
    err = SDL_RenderClear(globalrenderer);

    /* 2-copy png image to texture */
    err = SDL_UpdateTexture(globaltexture, NULL, image->pixels, image->pitch);

    /*  3-copy texture(s) to renderer */
    err = SDL_RenderCopy(globalrenderer, globaltexture, NULL, NULL); // image

    t = time(NULL);      /* get local time */
    tm = *localtime(&t); /* convert format */
    sprintf(text, "%d/%d/%d %d:%d:%d", tm.tm_year + 1900, tm.tm_mon + 1,
            tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    texttexture = rendertext(font, text, &postime);
    err = SDL_RenderCopy(globalrenderer, texttexture, NULL, &postime);

    sprintf(text, "%d:%d:%d%c", gpsnow->longitude / 10000,
            gpsnow->longitude % 10000 / 100, gpsnow->longitude % 100,
            gpsnow->longitudeEW);
    texttexture = rendertext(font, text, &poslong);
    err = SDL_RenderCopy(globalrenderer, texttexture, NULL, &poslong);

    sprintf(text, "%d:%d:%d%c", gpsnow->latitude / 10000,
            gpsnow->latitude % 10000 / 100, gpsnow->latitude % 100,
            gpsnow->latitudeNS);
    texttexture = rendertext(font, text, &poslat);
    err = SDL_RenderCopy(globalrenderer, texttexture, NULL, &poslat);

    sprintf(text, "%5.1f", fxosnow->pitch);
    texttexture = rendertext(font, text, &pospitch);
    err = SDL_RenderCopy(globalrenderer, texttexture, NULL, &pospitch);

    sprintf(text, "%5.1f", fxosnow->roll);
    texttexture = rendertext(font, text, &posroll);
    err = SDL_RenderCopy(globalrenderer, texttexture, NULL, &posroll);

    sprintf(text, "%5.1f", fxosnow->yaw);
    texttexture = rendertext(font, text, &posyaw);
    err = SDL_RenderCopy(globalrenderer, texttexture, NULL, &posyaw);

    sprintf(text, "%d", gpsnow->altitude);
    texttexture = rendertext(font, text, &posaltitude);
    err = SDL_RenderCopy(globalrenderer, texttexture, NULL, &posaltitude);

    sprintf(text, "%d", fxosnow->diefaceUSS);
    texttexture = rendertext(font, text, &posdiefaceUSS);
    err = SDL_RenderCopy(globalrenderer, texttexture, NULL, &posdiefaceUSS);

    sprintf(text, "%d", dienow->diefaceSS);
    texttexture = rendertext(font, text, &posdiefaceSS);
    err = SDL_RenderCopy(globalrenderer, texttexture, NULL, &posdiefaceSS);

    sprintf(text, "%lld", dienow->dieaction);
    texttexture = rendertext(font, text, &posdieaction);
    err = SDL_RenderCopy(globalrenderer, texttexture, NULL, &posdieaction);

    sprintf(text, "%7.3f", (gpsnow->declination));
    texttexture = rendertext(font, text, &posecompass);
    err = SDL_RenderCopy(globalrenderer, texttexture, NULL, &posecompass);

    /* render happyface */
    if (inMotionF)
      err = SDL_RenderCopy(globalrenderer, happytexture, NULL, &poshappy);
    if (err < 0)
      printf("Happyface err: %s\n", SDL_GetError());

    /* 4-render whatever prepared to physical device */
    SDL_RenderPresent(globalrenderer);
  }

  /* exit */

  // Free  font
  TTF_CloseFont(font);
  font = NULL;
  closeSDL2();

  munmap(fxosnow, SIZE1);
  munmap(gpsnow, SIZE2);
  munmap(dienow, SIZE3);
  close(fd1);
  close(fd2);
  close(fd3);

  exit(0);
}
