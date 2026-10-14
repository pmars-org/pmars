/* Compiled only into pmars-sdl3-events, never into the distributed program. */
static void test_require(int ok, const char *what)
{
    if (!ok) { fprintf(stderr, "FAIL SDL3 events: %s (%s)\n", what, SDL_GetError()); exit(90); }
}
static void test_key(SDL_Keycode key, SDL_Keymod mod)
{
    SDL_Event e;
    SDL_zero(e);
    e.type = SDL_EVENT_KEY_DOWN;
    e.key.key = key;
    e.key.mod = mod;
    test_require(SDL_PushEvent(&e), "queue key");
}
static void test_text(const char *text)
{
    SDL_Event e;
    SDL_zero(e);
    e.type = SDL_EVENT_TEXT_INPUT;
    e.text.text = text;
    test_require(SDL_PushEvent(&e), "queue text");
}
static void test_input(const char *expected)
{
    char buf[MAXALLCHAR];
    test_require(sdlgr_gets(buf, sizeof(buf), "test> ") != NULL, "gets returned");
    if (strcmp(buf, expected)) {
        fprintf(stderr, "Expected [%s], got [%s]\n", expected, buf);
        test_require(0, "input contents");
    }
}
static void sdl3_event_tests(void)
{
    SDL_Event e;
    SDL_KeyboardEvent key;
    char buf[MAXALLCHAR];
    int i, coloured = 0, oldspeed;
    const char *exit_test = SDL_getenv("PMARS_SDL3_EXIT_TEST");
    inputRedirection = 0;
    SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);
    if (exit_test) {
        SDL_zero(e); SDL_zero(key);
        if (!strcmp(exit_test, "close")) {
            e.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED; default_handler(&e);
        } else if (!strcmp(exit_test, "ctrl-c")) {
            key.key = SDLK_C; key.mod = SDL_KMOD_CTRL; special_keyhandler(&key);
        } else if (!strcmp(exit_test, "escape")) {
            test_key(SDLK_ESCAPE, 0); cycle = 0; sdlgr_display_cycle();
        } else if (!strcmp(exit_test, "endwait")) {
            test_key(SDLK_SPACE, 0); sdlgr_display_close(WAIT); exit(0);
        }
        test_require(0, "exit handler returned");
    }
    /* SDL_KEY_DOWN must not duplicate the following printable text event. */
    test_key(SDLK_L, 0); test_text("lis 0,3"); test_key(SDLK_RETURN, 0);
    test_input("lis 0,3\n");
    test_text("?@<>_+AZ"); test_key(SDLK_BACKSPACE, 0);
    test_text("\xc3\xa4"); test_key(SDLK_KP_ENTER, 0);
    test_input("?@<>_+A\n");
    test_key(SDLK_UP, SDL_KMOD_SHIFT); test_key(SDLK_RETURN, 0);
    test_input("?@<>_+A\n");
    /* AltGr must not become a Ctrl+Alt macro. */
    test_key(SDLK_Q, SDL_KMOD_CTRL | SDL_KMOD_RALT); test_text("@");
    test_key(SDLK_RETURN, 0); test_input("@\n");
    test_key(SDLK_KP_1, SDL_KMOD_NUM); test_text("1");
    test_key(SDLK_RETURN, 0); test_input("1\n");
    test_key(SDLK_F13, 0); test_input(" m f13\n");
    test_key(SDLK_LEFT, SDL_KMOD_CTRL); test_input(" m ctrl-left\n");
    SDL_zero(key); key.key = SDLK_F15;
    test_require(macro_key(&key, buf, sizeof(buf)) && !strcmp(buf, " m f15\n"), "F15 macro");
    sdlgr_update(2);
    test_require(NTextPanels == 2 && curPanel == 2, "split panel");
    test_key(SDLK_TAB, 0); test_text("reg"); test_key(SDLK_RETURN, 0);
    test_input("reg\n");
    test_require(curPanel == 1, "tab focus");
    sdlgr_update(2); sdlgr_update(0);
    test_require(NTextPanels == 1 && curPanel == 1, "close panel");
    SDL_zero(e); e.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    e.button.button = 1; e.button.x = ArenaX + CellSize; e.button.y = ArenaY;
    test_require(SDL_PushEvent(&e), "queue mouse");
    test_input(" m mousel\n"); test_require(curAddr == 1, "mouse core address");
    /* Exercise pixel writes, bitmap fonts, sprite colours, scroll and resize. */
    for (i = 0; i < 128; ++i) display_box(i, TL|TR|BL|BR, WarColourIx[i % warriors]);
    for (i = 0; i < 40; ++i) sdlgr_puts("SDL3 scroll test: 0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ\n");
    relayout(800, 600);
    test_require(TheSurf->w == 800 && TheSurf->h == 600, "resize buffer");
    for (i = 0; i < 128; ++i) display_box(i, TL|TR|BL|BR, WarColourIx[i % warriors]);
    test_require(*(Uint32 *)((Uint8 *)TheSurf->pixels + ArenaY * TheSurf->pitch + ArenaX * 4)
                 == Colours[WarColourIx[0]], "core sprite pixel before redraw");
    redraw();
    test_require(*(Uint32 *)((Uint8 *)TheSurf->pixels + ArenaY * TheSurf->pitch + ArenaX * 4)
                 == Colours[WarColourIx[0]], "core sprite pixel after redraw");
    sdlgr_puts("SDL3 graphics / debugger event tests passed\n");
    sdlgr_refresh(curPanel);
    for (i = 0; i < TheSurf->h * TheSurf->pitch; ++i)
        if (((unsigned char *)TheSurf->pixels)[i]) ++coloured;
    test_require(coloured > 1000, "nonempty framebuffer");
    test_require(SDL_SaveBMP(TheSurf, "sdl3-events.bmp"), "save graphics evidence");
    /* Real event handler, and real native resize/expose event queue. */
    SDL_zero(e); e.type = SDL_EVENT_WINDOW_EXPOSED; default_handler(&e);
    for (i = 0; i < 20; ++i) {
        while (SDL_PollEvent(&e)) default_handler(&e);
        SDL_Delay(5);
    }
    if (strcmp(SDL_GetCurrentVideoDriver(), "dummy")) {
        SDL_zero(key); key.key = SDLK_RETURN; key.mod = SDL_KMOD_ALT;
        test_require(special_keyhandler(&key), "enter fullscreen");
        SDL_SyncWindow(TheWindow);
        while (SDL_PollEvent(&e)) default_handler(&e);
        test_require(SDL_GetWindowFlags(TheWindow) & SDL_WINDOW_FULLSCREEN, "fullscreen flag");
        test_require(special_keyhandler(&key), "leave fullscreen");
        SDL_SyncWindow(TheWindow);
        while (SDL_PollEvent(&e)) default_handler(&e);
        test_require(!(SDL_GetWindowFlags(TheWindow) & SDL_WINDOW_FULLSCREEN), "windowed flag");
    }
    SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);
    oldspeed = displaySpeed;
    test_text(">"); cycle = 0; sdlgr_display_cycle();
    test_require(displaySpeed == max(0, oldspeed-1), "battle speed hotkey");
    test_text("4"); sdlgr_display_cycle();
    test_require(displayLevel == 4, "battle level hotkey");
    printf("PASS: SDL3 text, editing, history, AltGr, keypad, F13/F15, macros, tab, mouse, resize, scroll, pixels, expose, fullscreen (native only), battle hotkeys; driver=%s\n", SDL_GetCurrentVideoDriver());
    exit(0);
}

