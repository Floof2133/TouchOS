// userland/pkg-manager/tpkg-cli.c
// TouchOS Package Manager - Command Line Interface
// Created by: floof<3

#include "tpkg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_usage(const char* prog_name) {
    printf("TouchOS Package Manager (tpkg) v1.0\n\n");
    printf("Usage: %s <command> [options]\n\n", prog_name);
    printf("Commands:\n");
    printf("  install <package>      Install a package from repository\n");
    printf("  remove <package>       Remove an installed package\n");
    printf("  upgrade <package>      Upgrade a package to latest version\n");
    printf("  upgrade-all            Upgrade all installed packages\n");
    printf("  update                 Update repository package list\n");
    printf("  search <query>         Search for packages\n");
    printf("  list                   List installed packages\n");
    printf("  info <package>         Show package information\n");
    printf("  verify <file.tpkg>     Verify package integrity\n");
    printf("\nExamples:\n");
    printf("  %s install vim         # Install vim text editor\n", prog_name);
    printf("  %s update              # Update package lists\n", prog_name);
    printf("  %s list                # Show installed packages\n", prog_name);
    printf("\n");
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    const char* command = argv[1];

    // Initialize package manager
    tpkg_init();

    // Handle commands
    if (strcmp(command, "install") == 0) {
        if (argc < 3) {
            printf("Error: Package name required\n");
            printf("Usage: %s install <package>\n", argv[0]);
            return 1;
        }

        const char* package = argv[2];
        int result = tpkg_install(package);

        if (result != TPKG_OK) {
            printf("Error: %s\n", tpkg_get_error_string(result));
            return 1;
        }

    } else if (strcmp(command, "remove") == 0) {
        if (argc < 3) {
            printf("Error: Package name required\n");
            return 1;
        }

        const char* package = argv[2];
        int result = tpkg_remove(package);

        if (result != TPKG_OK) {
            printf("Error: %s\n", tpkg_get_error_string(result));
            return 1;
        }

    } else if (strcmp(command, "upgrade") == 0) {
        if (argc < 3) {
            printf("Error: Package name required\n");
            return 1;
        }

        const char* package = argv[2];
        int result = tpkg_upgrade(package);

        if (result != TPKG_OK) {
            printf("Error: %s\n", tpkg_get_error_string(result));
            return 1;
        }

    } else if (strcmp(command, "upgrade-all") == 0) {
        int result = tpkg_upgrade_all();

        if (result != TPKG_OK) {
            printf("Error: %s\n", tpkg_get_error_string(result));
            return 1;
        }

    } else if (strcmp(command, "update") == 0) {
        int result = tpkg_update_repo();

        if (result != TPKG_OK) {
            printf("Error: %s\n", tpkg_get_error_string(result));
            return 1;
        }

    } else if (strcmp(command, "list") == 0) {
        tpkg_installed_t packages[256];
        int count = tpkg_list_installed(packages, 256);

        if (count == 0) {
            printf("No packages installed\n");
        } else {
            printf("Installed packages (%d):\n", count);
            for (int i = 0; i < count; i++) {
                printf("  %s (%s)\n",
                       packages[i].metadata.name,
                       packages[i].metadata.version);
            }
        }

    } else if (strcmp(command, "info") == 0) {
        if (argc < 3) {
            printf("Error: Package name required\n");
            return 1;
        }

        const char* package = argv[2];

        // Try to read from cache or installed location
        char path[512];
        snprintf(path, sizeof(path), "/var/cache/tpkg/%s.tpkg", package);

        tpkg_metadata_t metadata;
        int result = tpkg_read_metadata(path, &metadata);

        if (result != TPKG_OK) {
            printf("Error: Could not read package info: %s\n",
                   tpkg_get_error_string(result));
            return 1;
        }

        tpkg_print_package_info(&metadata);

    } else if (strcmp(command, "verify") == 0) {
        if (argc < 3) {
            printf("Error: Package file required\n");
            return 1;
        }

        const char* file = argv[2];
        int result = tpkg_verify(file);

        if (result == TPKG_OK) {
            printf("Package verification: OK\n");
        } else {
            printf("Package verification: FAILED\n");
            printf("Error: %s\n", tpkg_get_error_string(result));
            return 1;
        }

    } else if (strcmp(command, "search") == 0) {
        if (argc < 3) {
            printf("Error: Search query required\n");
            return 1;
        }

        const char* query = argv[2];
        tpkg_repo_entry_t results[64];
        int count = tpkg_search(query, results, 64);

        if (count == 0) {
            printf("No packages found matching '%s'\n", query);
        } else {
            printf("Found %d package(s):\n", count);
            for (int i = 0; i < count; i++) {
                printf("  %s (%s) - %s\n",
                       results[i].name,
                       results[i].version,
                       results[i].description);
            }
        }

    } else {
        printf("Error: Unknown command '%s'\n", command);
        print_usage(argv[0]);
        return 1;
    }

    return 0;
}
