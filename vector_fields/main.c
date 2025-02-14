#include "SDL_events.h"
#include "SDL_keycode.h"
#include "SDL_mouse.h"
#include "SDL_render.h"
#include <SDL2/SDL.h>
#include <_stdio.h>
#include <math.h>
#include <stdint.h>

typedef struct {
    float x;
    float y;
} Vector2;

#define VEC_MAGNITUDE(vec) (sqrt(vex.x * vec.x + vec.y * vec.y))

#define WIDTH 600
#define HEIGHT 600

#define ROWS 30
#define COLUMNS 29

enum { FIELD_LINES, GRADIENT, BOTH } render_type;

typedef struct {
    Vector2 position;
    Vector2 direction;
    float potential;
} FieldLine;

typedef struct {
    Vector2 position;
    float charge;
} Particle;

typedef struct {
    Particle *array;
    uint8_t size;
} ParticleArray;

void rotate(Vector2 *vec, double angle) {
    Vector2 temp = *vec;
    vec->x = (temp.x * cos(angle) - temp.y * sin(angle));
    vec->y = (temp.x * sin(angle) + temp.y * cos(angle));
}

void render_gradient(SDL_Renderer *rend);
void set_gradient(int x, int y);

uint32_t number_to_color(double number, double max_magnitude);

void render_field_vec(SDL_Renderer *rend);

void draw_arrow(SDL_Renderer *rend, Vector2 pos, Vector2 dir, float potential);
void set_field_line(FieldLine *fl);
FieldLine *generate_field_lines();

int running = 1;
int track_mouse = 0;

int mouseX, mouseY;

uint32_t pixels[WIDTH * HEIGHT] = {0};

FieldLine *fl_arr;
ParticleArray arr;

SDL_Texture *texture;

int main(void) {

    fl_arr = generate_field_lines();
    arr = (ParticleArray){.array = calloc(255, sizeof(Particle)), .size = 0};

    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window *win = SDL_CreateWindow("Vector Fields", 20, 20, HEIGHT, WIDTH, 0);
    SDL_Renderer *rend = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);
    texture = SDL_CreateTexture(
        rend, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

    // Vector2 pos = {200, 200};
    // Vector2 dir = {25, 43.301};

    // FieldLine fl = {.position = pos};

    // Vector2 dir1 = {21.66, 12.5};

    SDL_Event e;
    while (running) {
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_QUIT:
                running = 0;
            case SDL_MOUSEBUTTONDOWN:
                if (e.button.button == SDL_BUTTON_LEFT) {
                    arr.array[arr.size++] =
                        (Particle){.position = (Vector2){.x = mouseX, .y = mouseY}, .charge = 1};
                } else {
                    arr.array[arr.size++] =
                        (Particle){.position = (Vector2){.x = mouseX, .y = mouseY}, .charge = -1};
                }
            case SDL_KEYDOWN:
                if (e.key.keysym.sym == SDLK_q) {
                    free(arr.array);
                    arr.array = calloc(256, sizeof(Particle));
                    arr.size = 0;
                } else if (e.key.keysym.sym == SDLK_m) {
                    track_mouse = !track_mouse;
                } else if (e.key.keysym.sym == SDLK_1) {
                    render_type = FIELD_LINES;
                } else if (e.key.keysym.sym == SDLK_2) {
                    render_type = GRADIENT;
                } else if (e.key.keysym.sym == SDLK_3) {
                    render_type = BOTH;
                }
            }
        }

        SDL_GetMouseState(&mouseX, &mouseY);
        // set_field_line(&fl);

        SDL_RenderClear(rend);
        if (render_type == GRADIENT || render_type == BOTH)
            render_gradient(rend);
        if (render_type == FIELD_LINES || render_type == BOTH)
            render_field_vec(rend);
        SDL_SetRenderDrawColor(rend, 0, 0, 0, 0);
        SDL_RenderPresent(rend);
    }

    SDL_Quit();

    return 0;
}

const double k = 750;
const double q = 1;

void set_gradient(int x, int y) {

    float distance_square = 0, field = 0;

    if (track_mouse) {
        distance_square = (x - mouseX) * (x - mouseX) + (y - mouseY) * (y - mouseY);
        field = k * q / distance_square;
    }

    for (int i = 0; i < arr.size; i++) {
        distance_square = (x - arr.array[i].position.x) * (x - arr.array[i].position.x) +
                          (y - arr.array[i].position.y) * (y - arr.array[i].position.y);
        field += k * arr.array[i].charge / distance_square;
    }

    pixels[y * WIDTH + x] = number_to_color(field, 0.01);
}

void render_gradient(SDL_Renderer *rend) {
    for (int i = 0; i < WIDTH; i++) {
        for (int j = 0; j < HEIGHT; j++) {
            set_gradient(i, j);
        }
    }
    int pitch; // Bytes per row
    void *texturePixels;
    if (SDL_LockTexture(texture, NULL, &texturePixels, &pitch) <
        0) { // Lock the texture for direct access
        fprintf(stderr, "Could not lock texture! SDL_Error: %s\n", SDL_GetError());
    }
    memcpy(texturePixels, pixels, WIDTH * HEIGHT * sizeof(uint32_t)); // Copy the pixel data
    SDL_UnlockTexture(texture);

    SDL_Rect textureRect = {0, 0, WIDTH, HEIGHT}; // Destination rectangle (entire window)
    SDL_RenderCopy(rend, texture, NULL, &textureRect);
}

void render_field_vec(SDL_Renderer *rend) {

    for (int i = 0; i < COLUMNS; i++) {
        for (int j = 1; j < ROWS; j++) {
            if (fl_arr[i * 29 + j].position.x == mouseX && fl_arr[i * 29 + j].position.y == mouseY)
                continue;
            set_field_line(&fl_arr[i * 29 + j]);
            draw_arrow(rend,
                fl_arr[i * 29 + j].position,
                fl_arr[i * 29 + j].direction,
                fl_arr[i * 29 + j].potential);
        }
    }
    for (int i = 0; i < arr.size; i++) {
        if (arr.array[i].charge > 0)
            SDL_SetRenderDrawColor(rend, 204, 51, 51, 255);
        else
            SDL_SetRenderDrawColor(rend, 255, 0, 255, 255);
        SDL_RenderFillRect(rend,
            &(SDL_Rect){
                .x = arr.array[i].position.x, .y = arr.array[i].position.y, .w = 5, .h = 5});
    }
}

FieldLine *generate_field_lines() {
    FieldLine *fl_arr = calloc(29 * 29, sizeof(FieldLine));
    for (int i = 0; i < COLUMNS; i++) {
        for (int j = 1; j < ROWS; j++) {
            fl_arr[i * 29 + j] = (FieldLine){.position = {.x = ((i + 1) * 20), .y = (j * 20)}};
        }
    }

    return fl_arr;
}

float clamp(float value, float min, float max) {
    if (value < min)
        return min;
    if (value > max)
        return max;
    return value;
}

void set_field_line(FieldLine *fl) {
    float distance_square, field;
    fl->direction = (Vector2){0, 0};
    fl->potential = 0;

    if (track_mouse) {
        distance_square = (fl->position.x - mouseX) * (fl->position.x - mouseX) +
                          (fl->position.y - mouseY) * (fl->position.y - mouseY);
        field = k * q / distance_square;
        fl->direction = (Vector2){
            .x = (fl->position.x - mouseX) * field, .y = (fl->position.y - mouseY) * field};
        fl->potential = field * sqrt(distance_square);
    }

    for (int i = 0; i < arr.size; i++) {
        distance_square =
            (fl->position.x - arr.array[i].position.x) *
                (fl->position.x - arr.array[i].position.x) +
            (fl->position.y - arr.array[i].position.y) * (fl->position.y - arr.array[i].position.y);
        field = k * arr.array[i].charge / distance_square;
        fl->direction.x += (fl->position.x - arr.array[i].position.x) * field;
        fl->direction.y += (fl->position.y - arr.array[i].position.y) * field;
        fl->potential += field * sqrt(distance_square);
    }

    // printf("%ff\n", fl->potential);

    // fl->direction.x = clamp(fl->direction.x, -10, 10);
    // fl->direction.y = clamp(fl->direction.y, -10, 10);
}

void draw_arrow(SDL_Renderer *rend, Vector2 pos, Vector2 dir, float potential) {
    SDL_SetRenderDrawColor(rend, 255, 255, 255, clamp(fabsf(potential) * 50, 0, 255));
    SDL_RenderDrawLineF(rend, pos.x, pos.y, pos.x + dir.x, pos.y + dir.y);
    Vector2 arrow1 = {(-dir.x / 5.0), (-dir.y / 5.0)};
    rotate(&arrow1, -M_PI / 6);
    SDL_RenderDrawLineF(
        rend, pos.x + dir.x, pos.y + dir.y, pos.x + dir.x + arrow1.x, pos.y + dir.y + arrow1.y);

    Vector2 arrow2 = {(-dir.x / 5.0), (-dir.y / 5.0)};
    rotate(&arrow2, M_PI / 6);
    SDL_RenderDrawLineF(
        rend, pos.x + dir.x, pos.y + dir.y, pos.x + dir.x + arrow2.x, pos.y + dir.y + arrow2.y);
}

uint32_t number_to_color(double num, double sensitivity) {
    double r, g, b;

    // Apply sensitivity: divide num by sensitivity.
    // Higher sensitivity means colors change faster for the same input change.
    num = num / sensitivity;

    // Clamp the modified input number to the range [-1, 1] for a defined color spectrum.
    num = fmax(-1.0, fmin(1.0, num));

    if (num <= -0.5) {
        // Range: -1 to -0.5 (Violet to Indigo)
        double range = -0.5 - (-1.0);
        double pos = (num - (-1.0)) / range;
        r = 1.0 - pos * (1.0 - 0.5);
        g = 0.0 + pos * 0.5;
        b = 1.0;
    } else if (num <= 0) {
        // Range: -0.5 to 0 (Indigo to Blue/Green)
        double range = 0.0 - (-0.5);
        double pos = (num - (-0.5)) / range;
        r = 0.5 - pos * 0.5;
        g = 0.5 + pos * 0.5;
        b = 1.0 - pos * 1.0;
    } else if (num <= 0.5) {
        // Range: 0 to 0.5 (Green to Yellow)
        double range = 0.5 - 0.0;
        double pos = (num - 0.0) / range;
        r = 0.0 + pos * 1.0;
        g = 1.0;
        b = 0.0;
    } else {
        // Range: 0.5 to 1 (Yellow to Red)
        double range = 1.0 - 0.5;
        double pos = (num - 0.5) / range;
        r = 1.0;
        g = 1.0 - pos * 1.0;
        b = 0.0;
    }

    // Ensure RGB values are in the valid range [0, 255]
    int ir = (int)(fmax(0.0, fmin(1.0, r)) * 255.0);
    int ig = (int)(fmax(0.0, fmin(1.0, g)) * 255.0);
    int ib = (int)(fmax(0.0, fmin(1.0, b)) * 255.0);

    // Pack RGB into a uint32_t - format 0x00RRGGBB
    uint32_t hex_color = (ir << 16) | (ig << 8) | ib;
    return hex_color;
}
