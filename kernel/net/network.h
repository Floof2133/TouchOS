// kernel/net/network.h
// TouchOS Networking Stack
// Basic TCP/IP implementation for package downloads
// Created by: floof<3

#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// Ethernet
// ============================================================================

#define ETH_ADDR_LEN 6
#define ETH_FRAME_LEN 1518
#define ETH_TYPE_IP 0x0800
#define ETH_TYPE_ARP 0x0806

typedef struct {
    uint8_t dst_mac[ETH_ADDR_LEN];
    uint8_t src_mac[ETH_ADDR_LEN];
    uint16_t ethertype;
    uint8_t payload[];
} __attribute__((packed)) eth_header_t;

// ============================================================================
// IPv4
// ============================================================================

typedef struct {
    uint8_t version_ihl;    // Version (4 bits) + IHL (4 bits)
    uint8_t tos;            // Type of Service
    uint16_t length;        // Total Length
    uint16_t id;            // Identification
    uint16_t flags_offset;  // Flags (3 bits) + Fragment Offset (13 bits)
    uint8_t ttl;            // Time to Live
    uint8_t protocol;       // Protocol
    uint16_t checksum;      // Header Checksum
    uint32_t src_ip;        // Source IP
    uint32_t dst_ip;        // Destination IP
    uint8_t options[];      // Options (variable)
} __attribute__((packed)) ip_header_t;

#define IP_PROTOCOL_ICMP 1
#define IP_PROTOCOL_TCP  6
#define IP_PROTOCOL_UDP  17

// ============================================================================
// TCP
// ============================================================================

typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint8_t data_offset;    // Data offset (4 bits) + Reserved (4 bits)
    uint8_t flags;
    uint16_t window_size;
    uint16_t checksum;
    uint16_t urgent_ptr;
    uint8_t options[];
} __attribute__((packed)) tcp_header_t;

#define TCP_FLAG_FIN 0x01
#define TCP_FLAG_SYN 0x02
#define TCP_FLAG_RST 0x04
#define TCP_FLAG_PSH 0x08
#define TCP_FLAG_ACK 0x10
#define TCP_FLAG_URG 0x20

// TCP connection states
typedef enum {
    TCP_CLOSED,
    TCP_LISTEN,
    TCP_SYN_SENT,
    TCP_SYN_RECEIVED,
    TCP_ESTABLISHED,
    TCP_FIN_WAIT_1,
    TCP_FIN_WAIT_2,
    TCP_CLOSE_WAIT,
    TCP_CLOSING,
    TCP_LAST_ACK,
    TCP_TIME_WAIT
} tcp_state_t;

// TCP connection structure
typedef struct {
    uint32_t local_ip;
    uint32_t remote_ip;
    uint16_t local_port;
    uint16_t remote_port;
    tcp_state_t state;
    uint32_t seq_num;
    uint32_t ack_num;
    uint16_t window_size;
    uint8_t* recv_buffer;
    size_t recv_buffer_size;
    size_t recv_buffer_used;
} tcp_conn_t;

// ============================================================================
// UDP
// ============================================================================

typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t checksum;
    uint8_t data[];
} __attribute__((packed)) udp_header_t;

// ============================================================================
// ARP
// ============================================================================

typedef struct {
    uint16_t hw_type;
    uint16_t proto_type;
    uint8_t hw_addr_len;
    uint8_t proto_addr_len;
    uint16_t operation;
    uint8_t sender_mac[ETH_ADDR_LEN];
    uint32_t sender_ip;
    uint8_t target_mac[ETH_ADDR_LEN];
    uint32_t target_ip;
} __attribute__((packed)) arp_packet_t;

#define ARP_OP_REQUEST 1
#define ARP_OP_REPLY   2

// ============================================================================
// Network Interface
// ============================================================================

typedef struct {
    char name[16];
    uint8_t mac_addr[ETH_ADDR_LEN];
    uint32_t ip_addr;
    uint32_t netmask;
    uint32_t gateway;
    bool is_up;

    // Function pointers for device operations
    int (*send_packet)(void* dev, const void* data, size_t len);
    void* device_private;
} netif_t;

// ============================================================================
// Network Stack API
// ============================================================================

// Initialize networking stack
void net_init(void);

// Network interface management
netif_t* netif_create(const char* name);
void netif_set_addr(netif_t* netif, uint32_t ip, uint32_t netmask, uint32_t gateway);
void netif_set_mac(netif_t* netif, const uint8_t* mac);
void netif_up(netif_t* netif);
void netif_down(netif_t* netif);

// Packet handling
void net_rx_packet(netif_t* netif, const void* data, size_t len);
int net_tx_packet(netif_t* netif, const void* data, size_t len);

// IP functions
uint16_t ip_checksum(const void* data, size_t len);
int ip_send_packet(netif_t* netif, uint32_t dst_ip, uint8_t protocol,
                   const void* payload, size_t payload_len);

// ARP functions
int arp_resolve(netif_t* netif, uint32_t ip, uint8_t* mac_out);
void arp_send_request(netif_t* netif, uint32_t target_ip);

// TCP functions
tcp_conn_t* tcp_connect(netif_t* netif, uint32_t remote_ip, uint16_t remote_port);
int tcp_send(tcp_conn_t* conn, const void* data, size_t len);
int tcp_recv(tcp_conn_t* conn, void* buffer, size_t max_len);
void tcp_close(tcp_conn_t* conn);

// UDP functions
int udp_send(netif_t* netif, uint32_t dst_ip, uint16_t dst_port,
             uint16_t src_port, const void* data, size_t len);

// Helper functions
uint32_t ip_from_string(const char* ip_str);
void ip_to_string(uint32_t ip, char* str);
void mac_to_string(const uint8_t* mac, char* str);

#endif // NETWORK_H
