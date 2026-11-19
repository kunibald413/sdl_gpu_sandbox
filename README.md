## Setup

**SDL3:** download the MinGW dev package from the
[SDL releases](https://github.com/libsdl-org/SDL/releases), then copy
`local.cmake.example` to `local.cmake` and set `SDL3_DIR`.

**shadercross:** grab a prebuilt from the
[nightly builds](https://nightly.link/libsdl-org/SDL_shadercross/workflows/main/main) 
or from 
[github actions artifacts](https://github.com/libsdl-org/SDL_shadercross/actions)
and put it on your `PATH`.

## Build and run

```bash
./make.sh  # build
./run.sh   # build + run
```


## Credits

- [SDL](https://github.com/libsdl-org/SDL)
- [SDL_gpu_examples](https://github.com/TheSpydog/SDL_gpu_examples)
- [SDL_shadercross](https://github.com/libsdl-org/SDL_shadercross)