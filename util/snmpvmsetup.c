#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <errno.h>

#define ETHH_LEN sizeof(struct ether_header)

#if __linux__

#include <sys/socket.h>
#include <linux/if_packet.h>

void send_raw(char *ifname, char *buf, int len) {
    int sockfd;
    struct ifreq ifr;
    struct ether_header *eh = (struct ether_header *)buf;
    struct sockaddr_ll saddr;

    if ((sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
	perror("socket");
	exit(1);
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name)-1);
    if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0) {
	perror(ifname);
	exit(1);
    }
    memcpy(eh->ether_shost, &ifr.ifr_hwaddr.sa_data, ETHER_ADDR_LEN);

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name)-1);
    if (ioctl(sockfd, SIOCGIFINDEX, &ifr) < 0) {
	perror(ifname);
	exit(1);
    }

    memset(&saddr, 0, sizeof(saddr));
    saddr.sll_family = AF_PACKET;
    saddr.sll_ifindex = ifr.ifr_ifindex;

    if (sendto(sockfd, buf, len, 0,
		(struct sockaddr *)&saddr, sizeof(saddr)) != len)
	printf("Send failed\n");
}

#else /* !__linux__ */

#include <unistd.h>
#include <fcntl.h>
#include <net/bpf.h>

void send_raw(char *ifname, char *buf, int len) {
    int i, bpffd;
    char device[] = "/dev/bpf000";
    struct ifreq ifr;

    for (i = 0; i < 1000; i++) {
	snprintf(device, sizeof(device), "/dev/bpf%d", i);
	bpffd = open(device, O_RDWR);
	if (bpffd >= 0 || errno != EBUSY)
	    break;
    }
    if (bpffd < 0) {
	perror("/dev/bpf*");
	exit(1);
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name)-1);
    if (ioctl(bpffd, BIOCSETIF, &ifr) < 0) {
	perror(ifname);
	exit(1);
    }

    if (write(bpffd, buf, len) != len)
	printf("Send failed\n");
}

#endif /* __linux__ */

char hsu_mac[ETHER_ADDR_LEN] = {0x48,0x4e,0x59,0x45,0x53,0x55};
char rarp_hdr[] = {
	0x00,0x01,			// HW = Ethernet
	0x08,0x00,			// Proto = IPv4
	0x06,				// HW len = 6
	0x04,				// Proto len = 4
	0x00,0x04			// op = reply
};
#define RARP_VMUL  8
#define RARP_MASK 10
#define RARP_GW   14
#define RARP_MAC  18
#define RARP_IP   24
#define RARP_LEN  28

// snmpvmsetup eth0 48:4f:4e:45:59:5f 10.0.0.2 255.255.255.0 10.0.0.1

int main(int argc, char *argv[]) {
    char buf[1024], dummy;
    struct ether_header *eh = (struct ether_header *)buf;
    char *rarp = &buf[ETHH_LEN];
    unsigned short vmul = 0;

    if (argc < 6 || argc > 7) {
	fprintf(stderr, "Usage: %s Iface MAC IP Mask GW [Vmul]\n", argv[0]);
	exit(1);
    }

    memset(buf, 0, sizeof(buf));
    memcpy(eh->ether_dhost, hsu_mac, ETHER_ADDR_LEN);
    eh->ether_type = htons(ETHERTYPE_REVARP);
    memcpy(rarp, rarp_hdr, sizeof(rarp_hdr));

    if (6 != sscanf(argv[2], "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx%c",
	&rarp[RARP_MAC+0], &rarp[RARP_MAC+1], &rarp[RARP_MAC+2],
	&rarp[RARP_MAC+3], &rarp[RARP_MAC+4], &rarp[RARP_MAC+5], &dummy)) {
	fprintf(stderr, "%s: Invalid MAC\n", argv[2]);
	exit(1);
    }

    if (!inet_aton(argv[3], (struct in_addr *)&rarp[RARP_IP])) {
	fprintf(stderr, "%s: Invalid IP\n", argv[3]);
	exit(1);
    }

    if (!inet_aton(argv[4], (struct in_addr *)&rarp[RARP_MASK])) {
	fprintf(stderr, "%s: Invalid Mask\n", argv[4]);
	exit(1);
    }

    if (!inet_aton(argv[5], (struct in_addr *)&rarp[RARP_GW])) {
	fprintf(stderr, "%s: Invalid GW\n", argv[5]);
	exit(1);
    }

    if (argc > 6 && 1 != sscanf(argv[6], "%hi%c", &vmul, &dummy)) {
	fprintf(stderr, "%s: Invalid Vmul\n", argv[6]);
	exit(1);
    }
    rarp[RARP_VMUL+0] = vmul >> 8;
    rarp[RARP_VMUL+1] = vmul & 0xff;

    send_raw(argv[1], buf, ETHH_LEN + RARP_LEN);

    return 0;
}

