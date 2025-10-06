// userland/pkg-manager/tpkg.h
// TouchOS Package Manager - .tpkg file format and API
// Created by: floof<3
//
// .tpkg Format Specification:
// [Header] - 512 bytes
// [Metadata] - Variable length (JSON)
// [File Data] - Compressed tar.gz
//
// Example .tpkg structure:
// - Magic: "TPKG" (4 bytes)
// - Version: 1 (4 bytes)
// - Metadata size: X bytes (8 bytes)
// - Data size: Y bytes (8 bytes)
// - Checksum: SHA256 (32 bytes)
// - Reserved: (456 bytes)
// - Metadata: JSON with package info
// - Data: gzip compressed tarball

#ifndef TPKG_H
#define TPKG_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Package magic number "TPKG"
#define TPKG_MAGIC 0x474B5054  // "TPKG" in little-endian
#define TPKG_VERSION 1
#define TPKG_HEADER_SIZE 512
#define TPKG_MAX_NAME_LEN 64
#define TPKG_MAX_VERSION_LEN 32
#define TPKG_MAX_DESC_LEN 256
#define TPKG_MAX_DEPS 16

// Package header structure (512 bytes)
typedef struct {
    uint32_t magic;              // Magic number "TPKG"
    uint32_t version;            // Format version
    uint64_t metadata_size;      // Size of metadata section
    uint64_t data_size;          // Size of data section
    uint8_t checksum[32];        // SHA256 checksum of entire package
    uint8_t reserved[456];       // Reserved for future use
} __attribute__((packed)) tpkg_header_t;

// Package metadata (stored as JSON in the file)
typedef struct {
    char name[TPKG_MAX_NAME_LEN];
    char version[TPKG_MAX_VERSION_LEN];
    char description[TPKG_MAX_DESC_LEN];
    char author[TPKG_MAX_NAME_LEN];
    char license[TPKG_MAX_NAME_LEN];
    char architecture[16];       // "x86_64", "arm64", etc.

    // Dependencies
    int dep_count;
    char dependencies[TPKG_MAX_DEPS][TPKG_MAX_NAME_LEN];

    // Install info
    char install_path[256];      // Where to extract files
    bool requires_restart;

    // Build info
    uint64_t build_timestamp;
    char build_host[64];
} tpkg_metadata_t;

// Installed package database entry
typedef struct {
    tpkg_metadata_t metadata;
    char install_date[32];
    uint64_t installed_size;     // Size on disk after installation
    bool is_dependency;          // Auto-installed as dependency
} tpkg_installed_t;

// Package repository entry
typedef struct {
    char name[TPKG_MAX_NAME_LEN];
    char version[TPKG_MAX_VERSION_LEN];
    char description[TPKG_MAX_DESC_LEN];
    char download_url[512];
    uint64_t size;
    uint8_t checksum[32];
} tpkg_repo_entry_t;

// Package manager configuration
typedef struct {
    char repo_url[512];          // Repository URL
    char cache_dir[256];         // Downloaded packages cache
    char db_file[256];           // Installed packages database
    char install_root[256];      // Root directory for installations
    bool verify_checksums;       // Verify package integrity
} tpkg_config_t;

// ============================================================================
// Package Manager API
// ============================================================================

// Initialize package manager
void tpkg_init(void);

// Configuration
void tpkg_set_repo_url(const char* url);
void tpkg_set_cache_dir(const char* path);
void tpkg_load_config(const char* config_file);

// Package operations
int tpkg_install(const char* package_name);
int tpkg_remove(const char* package_name);
int tpkg_upgrade(const char* package_name);
int tpkg_upgrade_all(void);

// Repository operations
int tpkg_update_repo(void);  // Fetch latest package list
int tpkg_search(const char* query, tpkg_repo_entry_t* results, int max_results);
int tpkg_list_installed(tpkg_installed_t* packages, int max_packages);

// Package file operations
int tpkg_create(const char* source_dir, const char* output_file, tpkg_metadata_t* metadata);
int tpkg_extract(const char* tpkg_file, const char* dest_dir);
int tpkg_verify(const char* tpkg_file);
int tpkg_upload(const char* tpkg_file, const char* repo_url);

// Metadata operations
int tpkg_read_metadata(const char* tpkg_file, tpkg_metadata_t* metadata);
int tpkg_write_metadata(int fd, tpkg_metadata_t* metadata);

// Dependency resolution
int tpkg_resolve_deps(const char* package_name, char deps[][TPKG_MAX_NAME_LEN], int max_deps);
bool tpkg_is_installed(const char* package_name);

// Utility functions
void tpkg_print_package_info(tpkg_metadata_t* metadata);
const char* tpkg_get_error_string(int error_code);

// Error codes
#define TPKG_OK                0
#define TPKG_ERROR_INVALID    -1
#define TPKG_ERROR_NOT_FOUND  -2
#define TPKG_ERROR_NETWORK    -3
#define TPKG_ERROR_CHECKSUM   -4
#define TPKG_ERROR_DEPS       -5
#define TPKG_ERROR_IO         -6
#define TPKG_ERROR_PERMISSION -7
#define TPKG_ERROR_EXISTS     -8

#endif // TPKG_H
