#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_scancode.h>

#define SCREEN_WIDTH 640.0
#define SCREEN_HEIGHT 480.0

#define PADDLE_OFFSET 20.0
#define PADDLE_WIDTH 16.0
#define PADDLE_HEIGHT 110.0

#define BALL_SIZE 10.0

const Uint8* keyboard;
SDL_bool gameOver = 0;

enum Direction { UP = 0, DOWN = 1, LEFT = 2, RIGHT = 3 };
void print_dir(enum Direction d) {
  switch(d){
  case UP:
    printf("UP"); break;
  case DOWN:
    printf("DOWN"); break;
  case LEFT:
    printf("LEFT"); break;
  case RIGHT:
    printf("RIGHT"); break;
  }
  printf("\n");
}

//union for either rect or sdl_surface? (for drawing)

struct Ball {
  float s;
  float vx;
  float vy;
  SDL_FRect rect;
} typedef Ball;

void inputs(SDL_FRect*, SDL_FRect*);
void update(Ball*, SDL_FRect*, SDL_FRect*);
void draw(SDL_Renderer*, SDL_FRect*, SDL_FRect*, SDL_FRect*);

void center_rects(SDL_FRect** rects, int n) {
  for (int i = 0; i < n; i++){
    SDL_FRect* rect = rects[i];
    rect->x -= rect->w / 2;
    rect->y -= rect->h / 2;
  }
}

//Returns the position of the side of a rectangle
float rect_side(SDL_FRect* rect, enum Direction d) {
  switch (d){
  case UP:
    return rect->y;
  case DOWN:
    return rect->y + rect->h;
  case LEFT:
    return rect->x;
  case RIGHT:
    return rect->x + rect->w;
  }
}

SDL_FPoint rect_center(SDL_FRect *rect) {
  SDL_FPoint p = {(2*rect->x+rect->w)/2, (2*rect->y+rect->h)/2};
  return p;
}

SDL_FPoint rect_corner(SDL_FRect *rect, enum Direction d1, enum Direction d2){
  //this is dumb lol
  d1*=2;  d2*=2; d1 += (d1==4); d2 += (d2==4);
  int c = d1+d2;
  SDL_FPoint pt;
  
  switch (c) {
  case 5: //5: UL
    pt.x = rect_side(rect, LEFT); pt.y = rect_side(rect, UP);
    return pt;
  case 6: //6: UR
    pt.x = rect_side(rect, RIGHT); pt.y = rect_side(rect, UP);
    return pt;
  case 7: //7: DL
    pt.x = rect_side(rect, LEFT); pt.y = rect_side(rect, DOWN);
    return pt;
  case 8: //8: DR
    pt.x = rect_side(rect, RIGHT); pt.y = rect_side(rect, DOWN);
    return pt;
  }

  printf("rect_corner: bad arguments");
  exit(EXIT_FAILURE);
}

//Return index of the min number in the array
int i_min(float* nums, int n){
  float min = 1000000000;
  int index = -1;
  for (int i = 0; i < n; i++){
    float num = nums[i];
    if (num < min) {min = num; index = i;}
  }
  return index;
}

float norm2D(float a, float b) {
  return sqrt( a*a + b*b );
}
//Checks if "out" is in "in". Returns which side of the "in" was entered (or also where the "out" should be set)
//Returns -1 if not inside 
enum Direction AABB(SDL_FRect *out, SDL_FRect *in, int set_pos) {

  SDL_bool isInside = 0; 
  SDL_FPoint pts[] = {rect_corner(out, UP, LEFT), rect_corner(out, UP, RIGHT),
		      rect_corner(out, DOWN, LEFT), rect_corner(out, DOWN, RIGHT)};
  isInside = SDL_EncloseFPoints(pts, 4, in, NULL);
  if (!isInside) {return -1;}

  //(if isInside)
  printf("IS INSIDE\n"); fflush(stdout);
  float up_inset = rect_side(out, DOWN) - rect_side(in, UP);
  float down_inset = rect_side(in, DOWN) - rect_side(out, UP);
  float left_inset = rect_side(out, RIGHT) - rect_side(in, LEFT);
  float right_inset = rect_side(in, RIGHT) - rect_side(out, LEFT);
  
  float insets[4] = {up_inset, down_inset, left_inset, right_inset};
  for (int i = 0; i<4; i++){ if (insets[i] < 0) {insets[i] = 1000000;} }
  enum Direction index = i_min(insets, 4);
  int offset = 1;
  print_dir(index);
  if (set_pos){
    switch(index){
    case UP: out->y -= (up_inset + offset); break;
    case DOWN: out->y += (down_inset + offset); break;
    case LEFT: out->x -= (left_inset + offset); break;
    case RIGHT: out->x += (right_inset + offset); break;
    }
  }

  return index;
}


SDL_FPoint bounce_dir(SDL_FRect* to_bounce, SDL_FRect* to_bounce_off) {
  SDL_FPoint to_bounce_center = rect_center(to_bounce);
  SDL_FPoint to_bounce_off_center = rect_center(to_bounce_off);
  
  float dx = to_bounce_center.x - to_bounce_off_center.x;
  float dy = to_bounce_center.y - to_bounce_off_center.y; printf("dy: %f\n", dy);

  float fct  = (dy) / (PADDLE_HEIGHT/2);
  SDL_FPoint pt = {1 - fabs(fct), fct};
  if (dx<0) { pt.x = -fabs(pt.x); } //bounce to left
  else {pt.x = fabs(pt.x); } // bounce to right

  if (dy < 0 ) { pt.y = -fabs(pt.y); } //Top half: go up
  else pt.y = fabs(pt.y); // Bottom half: go down
  
  
  return pt;
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

  keyboard = SDL_GetKeyboardState(NULL);
  
  SDL_FRect ball_rect = { SCREEN_WIDTH/2, SCREEN_HEIGHT/2-20, BALL_SIZE, BALL_SIZE };
  SDL_FRect p1 = { PADDLE_OFFSET + PADDLE_WIDTH/2, SCREEN_HEIGHT/2, PADDLE_WIDTH, PADDLE_HEIGHT };
  SDL_FRect p2={ SCREEN_WIDTH - PADDLE_OFFSET - PADDLE_WIDTH/2, SCREEN_HEIGHT/2, PADDLE_WIDTH, PADDLE_HEIGHT };

  SDL_FRect* rects_to_center[] = { &ball_rect, &p1, &p2 };
  center_rects( rects_to_center, sizeof(rects_to_center)/sizeof(rects_to_center[0]) );

  Ball ball = {1, 1, 0, ball_rect};
  
  //Loop
  while (!gameOver){
    inputs(&p1, &p2);
    update(&ball, &p1, &p2);
    draw(renderer, &ball.rect, &p1, &p2);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window); 
  SDL_Quit();

  return 0;
}

void inputs(SDL_FRect* p1, SDL_FRect* p2) {

  if (keyboard[SDL_SCANCODE_UP]) { p2->y -=2; }
  if (keyboard[SDL_SCANCODE_DOWN]) { p2->y += 2; }
  if (keyboard[SDL_SCANCODE_W]) { p1->y -= 2; }
  if (keyboard[SDL_SCANCODE_S]) { p1->y += 2; }
  
  SDL_Event e;
  while (SDL_PollEvent(&e)){ //handle all events in the queue
    switch(e.type){
    case SDL_QUIT:
      gameOver = 1;
      break;
    }
  }
}

void update(Ball* ball, SDL_FRect* p1, SDL_FRect* p2) {
  
  ball->rect.x += ball->vx * ball->s ;
  ball->rect.y += ball->vy * ball->s ;

  SDL_FPoint new_vxy;
  
  enum Direction d1 = AABB(&ball->rect, p1, 1);
  if (d1 != -1) { new_vxy = bounce_dir(&ball->rect, p1);}
  
  enum Direction d2 = AABB(&ball->rect, p2, 1);
  if (d2 != -1) { new_vxy = bounce_dir(&ball->rect, p2);}

  if (d1 != -1 || d2 != -1){
    ball->vx = new_vxy.x;
    ball->vy = new_vxy.y;
    printf("VY = %f; VX = %f\n", ball->vy, ball->vx);
  }
  
  // Bounce on walls
  if ( rect_side(&ball->rect, UP) <= 0 || rect_side(&ball->rect, DOWN) >= SCREEN_HEIGHT) { ball->vy *= -1; }
  
  //Win condtion
  if (rect_side(&ball->rect, LEFT) <= 0) { printf("P2 WON!\n"); gameOver = 1;}
  if (rect_side(&ball->rect, RIGHT) >= SCREEN_WIDTH) { printf("P1 WON!\n"); gameOver = 1;}

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

