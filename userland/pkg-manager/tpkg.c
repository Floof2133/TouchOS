// userland/pkg-manager/tpkg.c
// TouchOS Package Manager Implementation
// Created by: floof<3

#include "tpkg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

// Global configuration
static tpkg_config_t config = {
    .repo_url = "http://159.13.54.231:8080",
    .cache_dir = "/var/cache/tpkg",
    .db_file = "/var/lib/tpkg/installed.db",
    .install_root = "/",
    .verify_checksums = true
};

// ============================================================================
// Initialization
// ============================================================================

void tpkg_init(void) {
    printf("TouchOS Package Manager v1.0\n");
    printf("Repository: %s\n", config.repo_url);

    // Create necessary directories
    mkdir(config.cache_dir, 0755);
    mkdir("/var/lib/tpkg", 0755);

    // Load configuration if exists
    tpkg_load_config("/etc/tpkg.conf");
}

void tpkg_set_repo_url(const char* url) {
    strncpy(config.repo_url, url, sizeof(config.repo_url) - 1);
}

void tpkg_set_cache_dir(const char* path) {
    strncpy(config.cache_dir, path, sizeof(config.cache_dir) - 1);
}

void tpkg_load_config(const char* config_file) {
    FILE* fp = fopen(config_file, "r");
    if (!fp) return;

    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n') continue;

        // Parse key=value pairs
        char* equals = strchr(line, '=');
        if (!equals) continue;

        *equals = '\0';
        char* key = line;
        char* value = equals + 1;

        // Remove trailing newline
        char* newline = strchr(value, '\n');
        if (newline) *newline = '\0';

        if (strcmp(key, "repo_url") == 0) {
            strncpy(config.repo_url, value, sizeof(config.repo_url) - 1);
        } else if (strcmp(key, "cache_dir") == 0) {
            strncpy(config.cache_dir, value, sizeof(config.cache_dir) - 1);
        }
    }

    fclose(fp);
}

// ============================================================================
// Package File Operations
// ============================================================================

int tpkg_read_metadata(const char* tpkg_file, tpkg_metadata_t* metadata) {
    int fd = open(tpkg_file, O_RDONLY);
    if (fd < 0) {
        return TPKG_ERROR_NOT_FOUND;
    }

    tpkg_header_t header;
    if (read(fd, &header, sizeof(header)) != sizeof(header)) {
        close(fd);
        return TPKG_ERROR_INVALID;
    }

    // Verify magic number
    if (header.magic != TPKG_MAGIC) {
        close(fd);
        return TPKG_ERROR_INVALID;
    }

    // Read metadata (JSON format)
    char* json_data = malloc(header.metadata_size + 1);
    if (!json_data) {
        close(fd);
        return TPKG_ERROR_IO;
    }

    if (read(fd, json_data, header.metadata_size) != (ssize_t)header.metadata_size) {
        free(json_data);
        close(fd);
        return TPKG_ERROR_IO;
    }
    json_data[header.metadata_size] = '\0';

    // Parse JSON (simple parser for now)
    // TODO: Use proper JSON library
    sscanf(json_data, "{\"name\":\"%[^\"]\",\"version\":\"%[^\"]\",\"description\":\"%[^\"]\"",
           metadata->name, metadata->version, metadata->description);

    free(json_data);
    close(fd);
    return TPKG_OK;
}

int tpkg_write_metadata(int fd, tpkg_metadata_t* metadata) {
    // Create JSON metadata
    char json[4096];
    int json_len = snprintf(json, sizeof(json),
        "{"
        "\"name\":\"%s\","
        "\"version\":\"%s\","
        "\"description\":\"%s\","
        "\"author\":\"%s\","
        "\"license\":\"%s\","
        "\"architecture\":\"%s\","
        "\"install_path\":\"%s\","
        "\"requires_restart\":%s,"
        "\"build_timestamp\":%lu,"
        "\"dependencies\":[",
        metadata->name,
        metadata->version,
        metadata->description,
        metadata->author,
        metadata->license,
        metadata->architecture,
        metadata->install_path,
        metadata->requires_restart ? "true" : "false",
        metadata->build_timestamp
    );

    // Add dependencies
    for (int i = 0; i < metadata->dep_count; i++) {
        json_len += snprintf(json + json_len, sizeof(json) - json_len,
                           "%s\"%s\"", i > 0 ? "," : "", metadata->dependencies[i]);
    }

    json_len += snprintf(json + json_len, sizeof(json) - json_len, "]}");

    if (write(fd, json, json_len) != json_len) {
        return TPKG_ERROR_IO;
    }

    return TPKG_OK;
}

int tpkg_verify(const char* tpkg_file) {
    int fd = open(tpkg_file, O_RDONLY);
    if (fd < 0) return TPKG_ERROR_NOT_FOUND;

    tpkg_header_t header;
    if (read(fd, &header, sizeof(header)) != sizeof(header)) {
        close(fd);
        return TPKG_ERROR_INVALID;
    }

    if (header.magic != TPKG_MAGIC) {
        close(fd);
        return TPKG_ERROR_INVALID;
    }

    // TODO: Verify checksum
    if (config.verify_checksums) {
        // Calculate SHA256 and compare with header.checksum
    }

    close(fd);
    return TPKG_OK;
}

// ============================================================================
// Package Installation
// ============================================================================

int tpkg_install(const char* package_name) {
    printf("Installing package: %s\n", package_name);

    // Check if already installed
    if (tpkg_is_installed(package_name)) {
        printf("Package %s is already installed\n", package_name);
        return TPKG_ERROR_EXISTS;
    }

    // Build download URL
    char download_url[1024];
    snprintf(download_url, sizeof(download_url),
             "%s/api/packages/%s/download", config.repo_url, package_name);

    // Download package to cache
    char cache_path[512];
    snprintf(cache_path, sizeof(cache_path),
             "%s/%s.tpkg", config.cache_dir, package_name);

    printf("Downloading from %s...\n", download_url);

    // TODO: Use HTTP client to download
    // For now, use curl as external command
    char cmd[2048];
    snprintf(cmd, sizeof(cmd), "curl -o %s %s", cache_path, download_url);
    if (system(cmd) != 0) {
        return TPKG_ERROR_NETWORK;
    }

    // Verify package
    if (tpkg_verify(cache_path) != TPKG_OK) {
        printf("Package verification failed!\n");
        unlink(cache_path);
        return TPKG_ERROR_CHECKSUM;
    }

    // Read metadata
    tpkg_metadata_t metadata;
    if (tpkg_read_metadata(cache_path, &metadata) != TPKG_OK) {
        return TPKG_ERROR_INVALID;
    }

    // Resolve and install dependencies
    char deps[TPKG_MAX_DEPS][TPKG_MAX_NAME_LEN];
    int dep_count = tpkg_resolve_deps(package_name, deps, TPKG_MAX_DEPS);

    for (int i = 0; i < dep_count; i++) {
        if (!tpkg_is_installed(deps[i])) {
            printf("Installing dependency: %s\n", deps[i]);
            if (tpkg_install(deps[i]) != TPKG_OK) {
                return TPKG_ERROR_DEPS;
            }
        }
    }

    // Extract package
    printf("Extracting package...\n");
    if (tpkg_extract(cache_path, config.install_root) != TPKG_OK) {
        return TPKG_ERROR_IO;
    }

    // Add to installed database
    FILE* db = fopen(config.db_file, "a");
    if (db) {
        fprintf(db, "%s|%s|installed\n", metadata.name, metadata.version);
        fclose(db);
    }

    printf("Package %s installed successfully!\n", package_name);

    if (metadata.requires_restart) {
        printf("Note: System restart required for changes to take effect.\n");
    }

    return TPKG_OK;
}

int tpkg_remove(const char* package_name) {
    printf("Removing package: %s\n", package_name);

    if (!tpkg_is_installed(package_name)) {
        printf("Package %s is not installed\n", package_name);
        return TPKG_ERROR_NOT_FOUND;
    }

    // TODO: Check if other packages depend on this
    // TODO: Remove files listed in package manifest
    // TODO: Update database

    printf("Package %s removed successfully!\n", package_name);
    return TPKG_OK;
}

// ============================================================================
// Repository Operations
// ============================================================================

int tpkg_update_repo(void) {
    printf("Updating package repository...\n");

    char url[1024];
    snprintf(url, sizeof(url), "%s/api/packages", config.repo_url);

    // Download package list
    char list_file[512];
    snprintf(list_file, sizeof(list_file), "%s/pkglist.json", config.cache_dir);

    char cmd[2048];
    snprintf(cmd, sizeof(cmd), "curl -o %s %s", list_file, url);
    if (system(cmd) != 0) {
        return TPKG_ERROR_NETWORK;
    }

    printf("Repository updated successfully!\n");
    return TPKG_OK;
}

int tpkg_search(const char* query, tpkg_repo_entry_t* results, int max_results) {
    // TODO: Parse cached package list and search
    (void)query;
    (void)results;
    (void)max_results;
    return 0;
}

int tpkg_list_installed(tpkg_installed_t* packages, int max_packages) {
    FILE* db = fopen(config.db_file, "r");
    if (!db) return 0;

    int count = 0;
    char line[512];

    while (fgets(line, sizeof(line), db) && count < max_packages) {
        char* sep = strchr(line, '|');
        if (!sep) continue;

        *sep = '\0';
        strncpy(packages[count].metadata.name, line, TPKG_MAX_NAME_LEN - 1);
        strncpy(packages[count].metadata.version, sep + 1, TPKG_MAX_VERSION_LEN - 1);
        count++;
    }

    fclose(db);
    return count;
}

// ============================================================================
// Package Creation & Upload
// ============================================================================

int tpkg_create(const char* source_dir, const char* output_file, tpkg_metadata_t* metadata) {
    printf("Creating package: %s\n", metadata->name);

    // Create tar.gz of source directory
    char tarball[512];
    snprintf(tarball, sizeof(tarball), "%s.tar.gz", output_file);

    char cmd[2048];
    snprintf(cmd, sizeof(cmd), "tar czf %s -C %s .", tarball, source_dir);
    if (system(cmd) != 0) {
        return TPKG_ERROR_IO;
    }

    // Open tarball to get size
    struct stat st;
    if (stat(tarball, &st) != 0) {
        return TPKG_ERROR_IO;
    }

    // Create .tpkg file
    int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        return TPKG_ERROR_IO;
    }

    // Write header
    tpkg_header_t header = {0};
    header.magic = TPKG_MAGIC;
    header.version = TPKG_VERSION;
    header.data_size = st.st_size;

    // Calculate metadata size (will be updated after writing metadata)
    lseek(fd, sizeof(header), SEEK_SET);
    off_t metadata_start = lseek(fd, 0, SEEK_CUR);

    if (tpkg_write_metadata(fd, metadata) != TPKG_OK) {
        close(fd);
        unlink(output_file);
        return TPKG_ERROR_IO;
    }

    off_t metadata_end = lseek(fd, 0, SEEK_CUR);
    header.metadata_size = metadata_end - metadata_start;

    // Copy tarball data
    int tar_fd = open(tarball, O_RDONLY);
    if (tar_fd < 0) {
        close(fd);
        return TPKG_ERROR_IO;
    }

    char buffer[4096];
    ssize_t bytes;
    while ((bytes = read(tar_fd, buffer, sizeof(buffer))) > 0) {
        write(fd, buffer, bytes);
    }
    close(tar_fd);

    // Update header
    lseek(fd, 0, SEEK_SET);
    write(fd, &header, sizeof(header));

    close(fd);
    unlink(tarball);

    printf("Package created: %s\n", output_file);
    return TPKG_OK;
}

int tpkg_extract(const char* tpkg_file, const char* dest_dir) {
    tpkg_header_t header;
    int fd = open(tpkg_file, O_RDONLY);
    if (fd < 0) return TPKG_ERROR_NOT_FOUND;

    read(fd, &header, sizeof(header));

    // Skip to data section
    lseek(fd, sizeof(header) + header.metadata_size, SEEK_SET);

    // Extract to temporary file
    char temp_tar[512];
    snprintf(temp_tar, sizeof(temp_tar), "/tmp/tpkg_extract_%d.tar.gz", getpid());

    int out_fd = open(temp_tar, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out_fd < 0) {
        close(fd);
        return TPKG_ERROR_IO;
    }

    char buffer[4096];
    uint64_t remaining = header.data_size;
    while (remaining > 0) {
        size_t to_read = remaining < sizeof(buffer) ? remaining : sizeof(buffer);
        ssize_t bytes = read(fd, buffer, to_read);
        if (bytes <= 0) break;
        write(out_fd, buffer, bytes);
        remaining -= bytes;
    }

    close(out_fd);
    close(fd);

    // Extract tarball
    char cmd[2048];
    snprintf(cmd, sizeof(cmd), "tar xzf %s -C %s", temp_tar, dest_dir);
    int result = system(cmd);
    unlink(temp_tar);

    return result == 0 ? TPKG_OK : TPKG_ERROR_IO;
}

int tpkg_upload(const char* tpkg_file, const char* repo_url) {
    printf("Uploading package to repository...\n");

    // Verify package first
    if (tpkg_verify(tpkg_file) != TPKG_OK) {
        printf("Package verification failed!\n");
        return TPKG_ERROR_CHECKSUM;
    }

    // Upload using HTTP POST
    char url[1024];
    snprintf(url, sizeof(url), "%s/api/packages/upload", repo_url);

    char cmd[2048];
    snprintf(cmd, sizeof(cmd),
             "curl -X POST -F \"package=@%s\" %s",
             tpkg_file, url);

    if (system(cmd) != 0) {
        return TPKG_ERROR_NETWORK;
    }

    printf("Package uploaded successfully!\n");
    return TPKG_OK;
}

// ============================================================================
// Utility Functions
// ============================================================================

int tpkg_resolve_deps(const char* package_name, char deps[][TPKG_MAX_NAME_LEN], int max_deps) {
    // TODO: Implement dependency resolution
    (void)package_name;
    (void)deps;
    (void)max_deps;
    return 0;
}

bool tpkg_is_installed(const char* package_name) {
    FILE* db = fopen(config.db_file, "r");
    if (!db) return false;

    char line[512];
    while (fgets(line, sizeof(line), db)) {
        char* sep = strchr(line, '|');
        if (!sep) continue;
        *sep = '\0';

        if (strcmp(line, package_name) == 0) {
            fclose(db);
            return true;
        }
    }

    fclose(db);
    return false;
}

void tpkg_print_package_info(tpkg_metadata_t* metadata) {
    printf("\nPackage Information:\n");
    printf("  Name:         %s\n", metadata->name);
    printf("  Version:      %s\n", metadata->version);
    printf("  Description:  %s\n", metadata->description);
    printf("  Author:       %s\n", metadata->author);
    printf("  License:      %s\n", metadata->license);
    printf("  Architecture: %s\n", metadata->architecture);
    printf("  Install Path: %s\n", metadata->install_path);

    if (metadata->dep_count > 0) {
        printf("  Dependencies:\n");
        for (int i = 0; i < metadata->dep_count; i++) {
            printf("    - %s\n", metadata->dependencies[i]);
        }
    }
    printf("\n");
}

const char* tpkg_get_error_string(int error_code) {
    switch (error_code) {
        case TPKG_OK: return "Success";
        case TPKG_ERROR_INVALID: return "Invalid package format";
        case TPKG_ERROR_NOT_FOUND: return "Package not found";
        case TPKG_ERROR_NETWORK: return "Network error";
        case TPKG_ERROR_CHECKSUM: return "Checksum verification failed";
        case TPKG_ERROR_DEPS: return "Dependency resolution failed";
        case TPKG_ERROR_IO: return "I/O error";
        case TPKG_ERROR_PERMISSION: return "Permission denied";
        case TPKG_ERROR_EXISTS: return "Package already exists";
        default: return "Unknown error";
    }
}

int tpkg_upgrade(const char* package_name) {
    printf("Upgrading package: %s\n", package_name);

    if (!tpkg_is_installed(package_name)) {
        return tpkg_install(package_name);
    }

    // TODO: Check for newer version
    // TODO: Download and install new version

    return TPKG_OK;
}

int tpkg_upgrade_all(void) {
    printf("Upgrading all packages...\n");

    tpkg_installed_t packages[256];
    int count = tpkg_list_installed(packages, 256);

    for (int i = 0; i < count; i++) {
        tpkg_upgrade(packages[i].metadata.name);
    }

    return TPKG_OK;
}
