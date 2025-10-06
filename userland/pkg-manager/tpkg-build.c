// userland/pkg-manager/tpkg-build.c
// TouchOS Package Builder - Create .tpkg packages from source
// Created by: floof<3

#include "tpkg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>

void print_usage(const char* prog_name) {
    printf("TouchOS Package Builder (tpkg-build) v1.0\n\n");
    printf("Usage: %s [options]\n\n", prog_name);
    printf("Options:\n");
    printf("  -n <name>          Package name (required)\n");
    printf("  -v <version>       Package version (required)\n");
    printf("  -d <description>   Package description\n");
    printf("  -a <author>        Package author\n");
    printf("  -l <license>       Package license (e.g., MIT, GPL)\n");
    printf("  -s <source_dir>    Source directory to package (default: current)\n");
    printf("  -o <output_file>   Output .tpkg file (default: <name>-<version>.tpkg)\n");
    printf("  -i <install_path>  Installation path (default: /usr/local)\n");
    printf("  -D <dependency>    Add dependency (can be used multiple times)\n");
    printf("  -r                 Requires system restart after install\n");
    printf("  -u <repo_url>      Upload to repository after building\n");
    printf("\nExamples:\n");
    printf("  %s -n myapp -v 1.0 -d \"My Application\" -s ./myapp\n", prog_name);
    printf("  %s -n vim -v 8.2 -l MIT -D libc -D ncurses\n", prog_name);
    printf("  %s -n kernel-module -v 1.0 -r -i /lib/modules\n", prog_name);
    printf("\n");
}

int main(int argc, char** argv) {
    // Initialize metadata
    tpkg_metadata_t metadata = {0};
    strcpy(metadata.architecture, "x86_64");
    strcpy(metadata.install_path, "/usr/local");
    metadata.build_timestamp = time(NULL);
    strncpy(metadata.build_host, "TouchOS", sizeof(metadata.build_host) - 1);

    const char* source_dir = ".";
    char output_file[256] = {0};
    const char* repo_url = NULL;
    bool has_name = false;
    bool has_version = false;

    // Parse command line arguments
    int opt;
    while ((opt = getopt(argc, argv, "n:v:d:a:l:s:o:i:D:ru:h")) != -1) {
        switch (opt) {
            case 'n':
                strncpy(metadata.name, optarg, TPKG_MAX_NAME_LEN - 1);
                has_name = true;
                break;

            case 'v':
                strncpy(metadata.version, optarg, TPKG_MAX_VERSION_LEN - 1);
                has_version = true;
                break;

            case 'd':
                strncpy(metadata.description, optarg, TPKG_MAX_DESC_LEN - 1);
                break;

            case 'a':
                strncpy(metadata.author, optarg, TPKG_MAX_NAME_LEN - 1);
                break;

            case 'l':
                strncpy(metadata.license, optarg, TPKG_MAX_NAME_LEN - 1);
                break;

            case 's':
                source_dir = optarg;
                break;

            case 'o':
                strncpy(output_file, optarg, sizeof(output_file) - 1);
                break;

            case 'i':
                strncpy(metadata.install_path, optarg, sizeof(metadata.install_path) - 1);
                break;

            case 'D':
                if (metadata.dep_count < TPKG_MAX_DEPS) {
                    strncpy(metadata.dependencies[metadata.dep_count],
                           optarg, TPKG_MAX_NAME_LEN - 1);
                    metadata.dep_count++;
                } else {
                    printf("Warning: Maximum dependencies exceeded\n");
                }
                break;

            case 'r':
                metadata.requires_restart = true;
                break;

            case 'u':
                repo_url = optarg;
                break;

            case 'h':
                print_usage(argv[0]);
                return 0;

            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    // Validate required fields
    if (!has_name || !has_version) {
        printf("Error: Package name and version are required\n\n");
        print_usage(argv[0]);
        return 1;
    }

    // Generate output filename if not specified
    if (output_file[0] == '\0') {
        snprintf(output_file, sizeof(output_file),
                "%s-%s.tpkg", metadata.name, metadata.version);
    }

    // Display package information
    printf("\n=== Building Package ===\n");
    tpkg_print_package_info(&metadata);
    printf("Source directory: %s\n", source_dir);
    printf("Output file: %s\n\n", output_file);

    // Create package
    printf("Creating package...\n");
    int result = tpkg_create(source_dir, output_file, &metadata);

    if (result != TPKG_OK) {
        printf("Error: Failed to create package: %s\n",
               tpkg_get_error_string(result));
        return 1;
    }

    printf("Package created successfully: %s\n", output_file);

    // Verify the created package
    printf("Verifying package...\n");
    result = tpkg_verify(output_file);

    if (result != TPKG_OK) {
        printf("Warning: Package verification failed: %s\n",
               tpkg_get_error_string(result));
    } else {
        printf("Package verification: OK\n");
    }

    // Upload to repository if requested
    if (repo_url) {
        printf("\nUploading to repository: %s\n", repo_url);
        result = tpkg_upload(output_file, repo_url);

        if (result != TPKG_OK) {
            printf("Error: Upload failed: %s\n",
                   tpkg_get_error_string(result));
            return 1;
        }

        printf("Package uploaded successfully!\n");
    }

    printf("\n=== Build Complete ===\n");
    return 0;
}
