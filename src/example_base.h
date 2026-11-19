#ifndef DEMO_EXAMPLE_BASE_H
#define DEMO_EXAMPLE_BASE_H

#include <SDL3/SDL.h>

typedef struct Context {
    SDL_Window    *Window;
    SDL_GPUDevice *Device;
    void*     ExampleData;
} Context;

typedef struct Example {
    const char* Name;
    int  (*Init)   (Context *c);
    int  (*Update) (Context *c);
    int  (*Draw)   (Context *c);
    void (*Quit)   (Context *c);
} Example;

#endif // !DEMO_EXAMPLE_BASE_H