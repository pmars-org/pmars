/* Keep the CRT console entry, standard streams, argv and exit status.
 * SDL_RunApp supplies platform setup, including macOS application setup. */
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
extern int pmars_main(int argc, char **argv);
int main(int argc, char **argv)
{
    return SDL_RunApp(argc, argv, pmars_main, NULL);
}
