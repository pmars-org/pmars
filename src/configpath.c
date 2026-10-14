/* Shared lookup for installed option files and debugger macros. */
#ifndef _WIN32
#define _XOPEN_SOURCE 700
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#endif

#define CONFIG_PATH_SIZE 32768
static char executable_dir[CONFIG_PATH_SIZE];

void pmars_init_config_path(const char *program)
{
    char path[CONFIG_PATH_SIZE];
    char *slash;
    path[0] = '\0';
    executable_dir[0] = '\0';
#ifdef _WIN32
    {
        DWORD length = GetModuleFileNameA(NULL, path, sizeof(path));
        if (!length || length >= sizeof(path)) return;
    }
#elif defined(__linux__)
    {
        ssize_t length = readlink("/proc/self/exe", path, sizeof(path) - 1);
        if (length < 0 || (size_t)length >= sizeof(path) - 1) path[0] = '\0';
        else path[length] = '\0';
    }
#elif defined(__APPLE__)
    {
        uint32_t size = sizeof(path);
        if (_NSGetExecutablePath(path, &size) != 0) path[0] = '\0';
    }
#endif
#ifndef _WIN32
    /* Resolve symlinks (e.g. Homebrew's bin links) to the package itself. */
    if (!path[0] && program && strchr(program, '/')) {
        if (strlen(program) < sizeof(path)) strcpy(path, program);
    }
    if (!path[0] && program) {
        const char *entry = getenv("PATH");
        while (entry) {
            const char *end = strchr(entry, ':');
            size_t length = end ? (size_t)(end - entry) : strlen(entry);
            int count;
            if (length < sizeof(path)) {
                count = snprintf(path, sizeof(path), "%.*s/%s",
                                 (int)(length ? length : 1), length ? entry : ".", program);
                if (count >= 0 && (size_t)count < sizeof(path) && access(path, X_OK) == 0)
                    break;
            }
            path[0] = '\0';
            entry = end ? end + 1 : NULL;
        }
    }
    if (path[0]) {
        char *resolved = realpath(path, NULL);
        if (!resolved) return;
        if (strlen(resolved) >= sizeof(path)) { free(resolved); return; }
        strcpy(path, resolved);
        free(resolved);
    }
#else
    (void)program;
#endif
    slash = strrchr(path, '/');
#ifdef _WIN32
    {
        char *backslash = strrchr(path, '\\');
        if (backslash && (!slash || backslash > slash)) slash = backslash;
    }
#endif
    if (!slash) return;
    /* Preserve the separator for executables directly in a filesystem root. */
    slash[1] = '\0';
    strcpy(executable_dir, path);
}

static FILE *open_in_directory(const char *directory, const char *name)
{
    char path[CONFIG_PATH_SIZE];
    int count;
    if (!directory || !*directory) { errno = ENOENT; return NULL; }
    count = snprintf(path, sizeof(path), "%s/%s", directory, name);
    if (count < 0 || (size_t)count >= sizeof(path)) { errno = ENAMETOOLONG; return NULL; }
    return fopen(path, "r");
}

FILE *pmars_open_config(const char *name)
{
    static const char *suffixes[] = {"../config", "../share/pmars", ""};
    FILE *file = fopen(name, "r");
    size_t i;
    char directory[CONFIG_PATH_SIZE];
    /* Only missing bare names are searched; never substitute for explicit
       paths or hide errors such as a local file being unreadable. */
    if (file || errno != ENOENT || strchr(name, '/') || strchr(name, '\\') || strchr(name, ':'))
        return file;
    file = open_in_directory(getenv("PMARSHOME"), name);
    if (file || errno != ENOENT) return file;
    if (executable_dir[0]) {
        for (i = 0; i < sizeof(suffixes) / sizeof(suffixes[0]); ++i) {
            int count = snprintf(directory, sizeof(directory), "%s%s", executable_dir, suffixes[i]);
            if (count < 0 || (size_t)count >= sizeof(directory)) continue;
            file = open_in_directory(directory, name);
            if (file || errno != ENOENT) return file;
        }
    }
#ifdef PMARS_DATADIR
    file = open_in_directory(PMARS_DATADIR, name);
#endif
    return file;
}
