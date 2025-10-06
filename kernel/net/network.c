// kernel/net/network.c
// TouchOS Networking Stack Implementation
// Created by: floof<3

#include "network.h"
#include "../heap.h"
#include "../../drivers/serial.h"
#include <string.h>

// Network interfaces
static netif_t* interfaces[4] = {0};
static int interface_count = 0;

// ARP cache
#define ARP_CACHE_SIZE 128
typedef struct {
    uint32_t ip;
    uint8_t mac[ETH_ADDR_LEN];
    uint64_t timestamp;
} arp_entry_t;

static arp_entry_t arp_cache[ARP_CACHE_SIZE];

// TCP connections
#define MAX_TCP_CONNECTIONS 64
static tcp_conn_t tcp_connections[MAX_TCP_CONNECTIONS];

// ============================================================================
// Initialization
// ============================================================================

void net_init(void) {
    serial_write("Network: Initializing networking stack...\n");

    // Clear interfaces
    for (int i = 0; i < 4; i++) {
        interfaces[i] = NULL;
    }

    // Clear ARP cache
    memset(arp_cache, 0, sizeof(arp_cache));

    // Clear TCP connections
    memset(tcp_connections, 0, sizeof(tcp_connections));
    for (int i = 0; i < MAX_TCP_CONNECTIONS; i++) {
        tcp_connections[i].state = TCP_CLOSED;
    }

    serial_write("Network: Stack initialized\n");
}

// ============================================================================
// Network Interface Management
// ============================================================================

netif_t* netif_create(const char* name) {
    if (interface_count >= 4) {
        return NULL;
    }

    netif_t* netif = kmalloc(sizeof(netif_t));
    if (!netif) return NULL;

    memset(netif, 0, sizeof(netif_t));
    strncpy(netif->name, name, sizeof(netif->name) - 1);

    interfaces[interface_count++] = netif;

    serial_write("Network: Created interface ");
    serial_write(name);
    serial_write("\n");

    return netif;
}

void netif_set_addr(netif_t* netif, uint32_t ip, uint32_t netmask, uint32_t gateway) {
    netif->ip_addr = ip;
    netif->netmask = netmask;
    netif->gateway = gateway;

    serial_write("Network: Configured IP address for ");
    serial_write(netif->name);
    serial_write("\n");
}

void netif_set_mac(netif_t* netif, const uint8_t* mac) {
    memcpy(netif->mac_addr, mac, ETH_ADDR_LEN);
}

void netif_up(netif_t* netif) {
    netif->is_up = true;
    serial_write("Network: Interface ");
    serial_write(netif->name);
    serial_write(" is up\n");
}

void netif_down(netif_t* netif) {
    netif->is_up = false;
    serial_write("Network: Interface ");
    serial_write(netif->name);
    serial_write(" is down\n");
}

// ============================================================================
// Packet Handling
// ============================================================================

void net_rx_packet(netif_t* netif, const void* data, size_t len) {
    if (!netif->is_up || len < sizeof(eth_header_t)) {
        return;
    }

    eth_header_t* eth = (eth_header_t*)data;
    uint16_t ethertype = __builtin_bswap16(eth->ethertype);

    switch (ethertype) {
        case ETH_TYPE_IP:
            // Handle IP packet
            if (len >= sizeof(eth_header_t) + sizeof(ip_header_t)) {
                ip_header_t* ip = (ip_header_t*)eth->payload;

                /*
                What is happening is it will process everything inside this process here.
                version_ihl packs version upper of four bits and header length 4 bits
                you would need constants like this:

                #define IP_PROTO_ICMP 1
                #define IP_PROTO_TCP  6
                #define IP_PROTO_UDP  17             
                
                we can add checksum verification later if you feel like it.
                */

if ((ip->version_ihl >> 4) != 4) {
    // Not IPv4
    return;
}

size_t ip_header_len = (ip->version_ihl & 0x0F) * 4;
if (len < sizeof(eth_header_t) + ip_header_len) {
    // Invalid header length
    return;
}

// Optional: verify IP checksum (if you have a function for it floof i know you read my source code :3)
// if (!ip_checksum_valid(ip)) return;

uint8_t protocol = ip->protocol;
void* payload = ((uint8_t*)ip) + ip_header_len;
size_t payload_len = len - sizeof(eth_header_t) - ip_header_len;

// Dispatch to protocol handler >:D
switch (protocol) {
    case IP_PROTO_ICMP:
        handle_icmp_packet(netif, ip, payload, payload_len);
        break;
    case IP_PROTO_UDP:
        handle_udp_packet(netif, ip, payload, payload_len);
        break;
    case IP_PROTO_TCP:
        handle_tcp_packet(netif, ip, payload, payload_len);
        break;
    default:
        // Unknown or unsupported protocol, which might be an issue later so i just left this like that :P
        break;
}
                
                
                (void)ip;
            }
            break;

        case ETH_TYPE_ARP:
            // Handle ARP packet
            if (len >= sizeof(eth_header_t) + sizeof(arp_packet_t)) {
                arp_packet_t* arp = (arp_packet_t*)eth->payload;
                uint16_t op = __builtin_bswap16(arp->operation);

                if (op == ARP_OP_REPLY) {
                    // Add to ARP cache
                    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
                        if (arp_cache[i].ip == 0 || arp_cache[i].ip == arp->sender_ip) {
                            arp_cache[i].ip = arp->sender_ip;
                            memcpy(arp_cache[i].mac, arp->sender_mac, ETH_ADDR_LEN);
                            break;
                        }
                    }
                }
            }
            break;
    }
}

int net_tx_packet(netif_t* netif, const void* data, size_t len) {
    if (!netif->is_up || !netif->send_packet) {
        return -1;
    }

    return netif->send_packet(netif->device_private, data, len);
}

// ============================================================================
// IP Functions
// ============================================================================

uint16_t ip_checksum(const void* data, size_t len) {
    const uint16_t* buf = data;
    uint32_t sum = 0;

    while (len > 1) {
        sum += *buf++;
        len -= 2;
    }

    if (len == 1) {
        sum += *(uint8_t*)buf;
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return ~sum;
}

int ip_send_packet(netif_t* netif, uint32_t dst_ip, uint8_t protocol,
                   const void* payload, size_t payload_len) {
    // Resolve MAC address via ARP
    uint8_t dst_mac[ETH_ADDR_LEN];
    if (arp_resolve(netif, dst_ip, dst_mac) != 0) {
        arp_send_request(netif, dst_ip);
        return -1;  // Need to retry after ARP resolves
    }

    // Build packet
    size_t total_len = sizeof(eth_header_t) + sizeof(ip_header_t) + payload_len;
    uint8_t* packet = kmalloc(total_len);
    if (!packet) return -1;

    // Ethernet header
    eth_header_t* eth = (eth_header_t*)packet;
    memcpy(eth->dst_mac, dst_mac, ETH_ADDR_LEN);
    memcpy(eth->src_mac, netif->mac_addr, ETH_ADDR_LEN);
    eth->ethertype = __builtin_bswap16(ETH_TYPE_IP);

    // IP header
    ip_header_t* ip = (ip_header_t*)eth->payload;
    ip->version_ihl = 0x45;  // IPv4, 20 byte header
    ip->tos = 0;
    ip->length = __builtin_bswap16(sizeof(ip_header_t) + payload_len);
    ip->id = 0;  // TODO: Use incrementing ID
    ip->flags_offset = 0;
    ip->ttl = 64;
    ip->protocol = protocol;
    ip->src_ip = netif->ip_addr;
    ip->dst_ip = dst_ip;
    ip->checksum = 0;
    ip->checksum = ip_checksum(ip, sizeof(ip_header_t));

    // Copy payload
    memcpy((uint8_t*)ip + sizeof(ip_header_t), payload, payload_len);

    // Send packet
    int result = net_tx_packet(netif, packet, total_len);
    kfree(packet);

    return result;
}

// ============================================================================
// ARP Functions
// ============================================================================

int arp_resolve(netif_t* netif, uint32_t ip, uint8_t* mac_out) {
    (void)netif;

    // Check cache
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].ip == ip) {
            memcpy(mac_out, arp_cache[i].mac, ETH_ADDR_LEN);
            return 0;
        }
    }

    return -1;  // Not found
}

void arp_send_request(netif_t* netif, uint32_t target_ip) {
    uint8_t packet[sizeof(eth_header_t) + sizeof(arp_packet_t)];

    // Ethernet header (broadcast)
    eth_header_t* eth = (eth_header_t*)packet;
    memset(eth->dst_mac, 0xFF, ETH_ADDR_LEN);
    memcpy(eth->src_mac, netif->mac_addr, ETH_ADDR_LEN);
    eth->ethertype = __builtin_bswap16(ETH_TYPE_ARP);

    // ARP packet
    arp_packet_t* arp = (arp_packet_t*)eth->payload;
    arp->hw_type = __builtin_bswap16(1);  // Ethernet
    arp->proto_type = __builtin_bswap16(ETH_TYPE_IP);
    arp->hw_addr_len = ETH_ADDR_LEN;
    arp->proto_addr_len = 4;
    arp->operation = __builtin_bswap16(ARP_OP_REQUEST);
    memcpy(arp->sender_mac, netif->mac_addr, ETH_ADDR_LEN);
    arp->sender_ip = netif->ip_addr;
    memset(arp->target_mac, 0, ETH_ADDR_LEN);
    arp->target_ip = target_ip;

    net_tx_packet(netif, packet, sizeof(packet));
}

// ============================================================================
// TCP Functions
// ============================================================================

tcp_conn_t* tcp_connect(netif_t* netif, uint32_t remote_ip, uint16_t remote_port) {
    // Find free connection slot
    tcp_conn_t* conn = NULL;
    for (int i = 0; i < MAX_TCP_CONNECTIONS; i++) {
        if (tcp_connections[i].state == TCP_CLOSED) {
            conn = &tcp_connections[i];
            break;
        }
    }

    if (!conn) return NULL;

    // Initialize connection
    conn->local_ip = netif->ip_addr;
    conn->remote_ip = remote_ip;
    conn->local_port = 49152 + (random() % 16384);  // TODO: proper random
    conn->remote_port = remote_port;
    conn->seq_num = random();  // TODO: proper random
    conn->state = TCP_SYN_SENT;

    // Allocate receive buffer
    conn->recv_buffer_size = 65536;
    conn->recv_buffer = kmalloc(conn->recv_buffer_size);
    conn->recv_buffer_used = 0;

    // Send SYN packet
    // TODO: Build and send TCP SYN

    return conn;
}

int tcp_send(tcp_conn_t* conn, const void* data, size_t len) {
    if (conn->state != TCP_ESTABLISHED) {
        return -1;
    }

    // TODO: Build and send TCP packet with data
    (void)data;
    (void)len;

    return 0;
}

int tcp_recv(tcp_conn_t* conn, void* buffer, size_t max_len) {
    if (conn->state != TCP_ESTABLISHED) {
        return -1;
    }

    // TODO: Read from receive buffer
    size_t to_copy = conn->recv_buffer_used < max_len ?
                     conn->recv_buffer_used : max_len;

    if (to_copy > 0) {
        memcpy(buffer, conn->recv_buffer, to_copy);
        // Shift remaining data
        memmove(conn->recv_buffer, conn->recv_buffer + to_copy,
                conn->recv_buffer_used - to_copy);
        conn->recv_buffer_used -= to_copy;
    }

    return to_copy;
}

void tcp_close(tcp_conn_t* conn) {
    // TODO: Send FIN packet
    if (conn->recv_buffer) {
        kfree(conn->recv_buffer);
        conn->recv_buffer = NULL;
    }
    conn->state = TCP_CLOSED;
}

// ============================================================================
// UDP Functions
// ============================================================================

int udp_send(netif_t* netif, uint32_t dst_ip, uint16_t dst_port,
             uint16_t src_port, const void* data, size_t len) {
    size_t packet_len = sizeof(udp_header_t) + len;
    uint8_t* packet = kmalloc(packet_len);
    if (!packet) return -1;

    udp_header_t* udp = (udp_header_t*)packet;
    udp->src_port = __builtin_bswap16(src_port);
    udp->dst_port = __builtin_bswap16(dst_port);
    udp->length = __builtin_bswap16(packet_len);
    udp->checksum = 0;  // Optional for IPv4

    memcpy(udp->data, data, len);

    int result = ip_send_packet(netif, dst_ip, IP_PROTOCOL_UDP, packet, packet_len);
    kfree(packet);

    return result;
}

// ============================================================================
// Helper Functions
// ============================================================================

uint32_t ip_from_string(const char* ip_str) {
    uint32_t a, b, c, d;
    sscanf(ip_str, "%u.%u.%u.%u", &a, &b, &c, &d);
    return (a << 24) | (b << 16) | (c << 8) | d;
}

void ip_to_string(uint32_t ip, char* str) {
    sprintf(str, "%u.%u.%u.%u",
            (ip >> 24) & 0xFF,
            (ip >> 16) & 0xFF,
            (ip >> 8) & 0xFF,
            ip & 0xFF);
}

void mac_to_string(const uint8_t* mac, char* str) {
    sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

// Stub implementations for missing functions
int random(void) {
    static uint32_t seed = 12345;
    seed = seed * 1103515245 + 12345;
    return seed;
}

int sscanf(const char* str, const char* format, ...) {
    (void)str; (void)format;
    return 0;  // TODO: Implement
}

int sprintf(char* str, const char* format, ...) {
    (void)str; (void)format;
    return 0;  // TODO: Implement
}

void* memmove(void* dest, const void* src, size_t n) {
    char* d = dest;
    const char* s = src;
    if (d < s) {
        for (size_t i = 0; i < n; i++) d[i] = s[i];
    } else {
        for (size_t i = n; i > 0; i--) d[i-1] = s[i-1];
    }
    return dest;
}

void* memset(void* s, int c, size_t n) {
    unsigned char* p = s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}

void* memcpy(void* dest, const void* src, size_t n) {
    char* d = dest;
    const char* s = src;
    while (n--) *d++ = *s++;
    return dest;
}

size_t strlen(const char* s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

char* strncpy(char* dest, const char* src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i]; i++) dest[i] = src[i];
    for (; i < n; i++) dest[i] = '\0';
    return dest;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}
