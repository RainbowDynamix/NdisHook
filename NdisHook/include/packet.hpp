#ifndef packet_hpp
#define packet_hpp

#include <ntifs.h>
#include <ntddk.h>
#pragma pack(push, 1)

struct ethernet_header {
    UCHAR  dst_mac[6];
    UCHAR  src_mac[6];
    USHORT ether_type; // big-endian
};

struct ipv4_header {
    UCHAR  version_ihl;    // high nibble = version, low nibble = IHL (32-bit words)
    UCHAR  dscp_ecn;
    USHORT total_length;   // big-endian, header + payload
    USHORT identification;
    USHORT flags_fragment;
    UCHAR  ttl;
    UCHAR  protocol;
    USHORT header_checksum;
    ULONG  src_addr;
    ULONG  dst_addr;
    // options follow if IHL > 5
};

struct tcp_header {
    USHORT src_port;             // big-endian
    USHORT dst_port;             // big-endian
    ULONG  seq_num;
    ULONG  ack_num;
    UCHAR  data_offset_reserved; // high nibble = data offset (32-bit words)
    UCHAR  flags;
    USHORT window;
    USHORT checksum;
    USHORT urgent_ptr;
    // options follow if data offset > 5
};

#pragma pack(pop)

constexpr USHORT ETHERTYPE_IPV4 = 0x0800;
constexpr UCHAR  IPPROTO_TCP_V4 = 6;

inline USHORT be16_to_host(USHORT v) { return (USHORT)((v >> 8) | (v << 8)); }

inline UCHAR ipv4_version(const ipv4_header* ip)      { return (UCHAR)(ip->version_ihl >> 4); }
inline UCHAR ipv4_ihl_words(const ipv4_header* ip)    { return (UCHAR)(ip->version_ihl & 0x0F); }
inline ULONG ipv4_header_bytes(const ipv4_header* ip) { return (ULONG)ipv4_ihl_words(ip) * 4u; }

inline UCHAR tcp_data_offset_words(const tcp_header* t) { return (UCHAR)((t->data_offset_reserved >> 4) & 0x0F); }
inline ULONG tcp_header_bytes(const tcp_header* t)      { return (ULONG)tcp_data_offset_words(t) * 4u; }

struct tcp_packet {
    const ethernet_header* eth;
    const ipv4_header*     ip;
    const tcp_header*      tcp;
    const UCHAR*           payload;
    ULONG                  payload_len;
};

// Parses buf as Ethernet/IPv4/TCP. Returns true on success; on success, the
// fields in *out alias into buf (no copies). IP total_length is trusted over
// buf_len so trailing L2 padding isn't included in payload_len.
inline bool parse_tcp_packet(const UCHAR* buf, ULONG buf_len, tcp_packet* out) {
    if (!buf || !out)
        return false;
    RtlZeroMemory(out, sizeof(*out));

    if (buf_len < sizeof(ethernet_header))
        return false;
    auto eth = reinterpret_cast<const ethernet_header*>(buf);
    if (be16_to_host(eth->ether_type) != ETHERTYPE_IPV4)
        return false;

    ULONG off = sizeof(ethernet_header);
    if (buf_len < off + sizeof(ipv4_header))
        return false;
    auto ip = reinterpret_cast<const ipv4_header*>(buf + off);
    if (ipv4_version(ip) != 4 || ip->protocol != IPPROTO_TCP_V4)
        return false;

    ULONG ip_hdr = ipv4_header_bytes(ip);
    USHORT ip_total = be16_to_host(ip->total_length);
    if (ip_hdr < sizeof(ipv4_header) || ip_total < ip_hdr)
        return false;
    if (buf_len < off + ip_total)
        return false;

    ULONG ip_payload_len = (ULONG)ip_total - ip_hdr;
    off += ip_hdr;
    if (ip_payload_len < sizeof(tcp_header))
        return false;
    auto tcp = reinterpret_cast<const tcp_header*>(buf + off);

    ULONG tcp_hdr = tcp_header_bytes(tcp);
    if (tcp_hdr < sizeof(tcp_header) || ip_payload_len < tcp_hdr)
        return false;

    out->eth         = eth;
    out->ip          = ip;
    out->tcp         = tcp;
    out->payload     = buf + off + tcp_hdr;
    out->payload_len = ip_payload_len - tcp_hdr;
    return true;
}

inline bool tcp_payload_starts_with(const tcp_packet& pkt, const UCHAR* sig, ULONG sig_len) {
    if (!sig || pkt.payload_len < sig_len)
        return false;
    return RtlCompareMemory(pkt.payload, sig, sig_len) == sig_len;
}

#endif // !packet_hpp
