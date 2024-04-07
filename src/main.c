#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_scancode.h>

#define SCREEN_WIDTH 640.0
#define SCREEN_HEIGHT 480.0

#define PADDLE_OFFSET 20.0
#define PADDLE_WIDTH 16.0
#define PADDLE_HEIGHT 120.0

#define BALL_SIZE 10.0

struct Ball {
  float s;
  float vx;
  float vy;
  SDL_FRect rect;
} typedef Ball;

void update(Ball*);
void draw(SDL_Renderer*, SDL_FRect*, SDL_FRect*, SDL_FRect*);

void center_frects(SDL_FRect** rects, int n) {
  for (int i = 0; i < n; i++){
    SDL_FRect* rect = rects[i];
    rect->x -= rect->w / 2;
    rect->y -= rect->h / 2;
  }
}

void center_frect(SDL_FRect* rect) {
  rect->x -= rect->w / 2;
  rect->y -= rect->h / 2;
}

int main(int argc, char** argv){

  if ( SDL_Init(SDL_INIT_EVERYTHING) ) { printf("%s\n", "Failed Initialization\n"); return 1;}

  SDL_Window* window = SDL_CreateWindow("Pong", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
					SCREEN_WIDTH, SCREEN_HEIGHT, 0);
  if (window == NULL) { printf("Failed Window Creation\n"); return 1;}

  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (renderer == NULL) { printf("Failed Renderer Creation\n"); return 1;}

  
  SDL_FRect ball_rect = { SCREEN_WIDTH/2, SCREEN_HEIGHT/2, BALL_SIZE, BALL_SIZE };
  SDL_FRect p1 = { PADDLE_OFFSET + PADDLE_WIDTH/2, SCREEN_HEIGHT/2, PADDLE_WIDTH, PADDLE_HEIGHT };
  SDL_FRect p2={ SCREEN_WIDTH - PADDLE_OFFSET - PADDLE_WIDTH/2, SCREEN_HEIGHT/2, PADDLE_WIDTH, PADDLE_HEIGHT };

  SDL_FRect* rects_to_center[] = { &ball_rect, &p1, &p2 };
  center_frects( rects_to_center, sizeof(rects_to_center)/sizeof(rects_to_center[0]) );

  Ball ball = {1, 1, 0, ball_rect};
  
  //Loop
  for (int t = 0 ; t < 800; t++){
    
    //inputs()
    update(&ball);
    draw(renderer, &ball.rect, &p1, &p2);
    SDL_Delay(1);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window); 
  SDL_Quit();

  return 0;
}

void update(Ball* ball) {
  ball->rect.x += sqrt( (ball->s*ball->s) - (ball->vy*ball->vy) ) ;
  ball->rect.y += sqrt( (ball->s*ball->s) - (ball->vx*ball->vx) ) ;
}

void draw(SDL_Renderer* renderer, SDL_FRect* ball_rect, SDL_FRect* p1, SDL_FRect* p2) {
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
  SDL_RenderClear(renderer);

  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_RenderFillRectF(renderer, ball_rect);
  SDL_RenderFillRectF(renderer, p1);
  SDL_RenderFillRectF(renderer, p2);

  SDL_RenderPresent(renderer);
}

