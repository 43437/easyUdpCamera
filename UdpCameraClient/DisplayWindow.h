#ifndef __DISPLAY_WINDOW__
#define __DISPLAY_WINDOW__

extern "C"{
#include "SDL2/SDL.h"
}

class DisplayWindow{
  
  SDL_Window *screen;
  SDL_Renderer* sdlRenderer;
  SDL_Texture* sdlTexture;
  int screen_w = 850;
  int screen_h = 480;
  int pixel_w=426,pixel_h=240; 
public:
  DisplayWindow(int pixel_w, int pixel_h);
  int refresh(uint8_t *buffer);
};

int DisplayWindow::refresh(uint8_t *buffer){
  
  SDL_UpdateTexture( sdlTexture, NULL, buffer, pixel_w);    

  SDL_Rect sdlRect;
  
  //FIX: If window is resize  
  sdlRect.x = 0;    
  sdlRect.y = 0;    
  sdlRect.w = screen_w;    
  sdlRect.h = screen_h;    
    
  SDL_RenderClear( sdlRenderer );     
  SDL_RenderCopy( sdlRenderer, sdlTexture, NULL, &sdlRect);    
  SDL_RenderPresent( sdlRenderer );    
  //Delay 40ms  
//   SDL_Delay(40);  
}

DisplayWindow::DisplayWindow(int pixel_w, int pixel_h){
  
  this->pixel_w = pixel_w;
  this->pixel_h = pixel_h;
  
  screen = SDL_CreateWindow("Simplest Video Play SDL2", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,  
        screen_w, screen_h,SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE); 
  
  sdlRenderer = SDL_CreateRenderer(screen, -1, 0); 
  sdlTexture = SDL_CreateTexture(sdlRenderer,SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING,pixel_w,pixel_h);
}
#endif