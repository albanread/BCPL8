#include <stdio.h>

#define BCPL_VERSION_MAJOR 1
#define BCPL_VERSION_MINOR 0
#define BCPL_VERSION_PATCH 567

static int version_major = BCPL_VERSION_MAJOR;
static int version_minor = BCPL_VERSION_MINOR;
static int version_patch = BCPL_VERSION_PATCH;

void print_version() {
    printf("NewBCPL Compiler Version %d.%d.%d\n", version_major, version_minor, version_patch);
}

// For build automation: increment version
void increment_patch_version() { version_patch++; }
void increment_minor_version() { version_minor++; version_patch = 0; }
void increment_major_version() { version_major++; version_minor = 0; version_patch = 0; }

// For build.sh: expose a way to update version numbers
void write_version_file(const char* path) {
    FILE* f = fopen(path, "w");
    if (!f) return;
    fprintf(f, "#define BCPL_VERSION_MAJOR %d\n", version_major);
    fprintf(f, "#define BCPL_VERSION_MINOR %d\n", version_minor);
    fprintf(f, "#define BCPL_VERSION_PATCH %d\n", version_patch);
    fclose(f);
}