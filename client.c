//#include	"unp.h"
#include        <sys/types.h>   /* basic system data types */
#include        <sys/socket.h>  /* basic socket definitions */
#include        <sys/time.h>    /* timeval{} for select() */
#include        <time.h>                /* timespec{} for pselect() */
#include        <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include        <arpa/inet.h>   /* inet(3) functions */
#include        <errno.h>
#include        <fcntl.h>               /* for nonblocking */
#include        <netdb.h>
#include        <signal.h>
#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include        <unistd.h>
#include 	<sys/ioctl.h>
#include 	<net/if.h>
#include	<netdb.h>

#include	<sys/utsname.h>
#include	<linux/un.h>



#define MAXLINE 1024
#define SA      struct sockaddr
#define UDP_SEND_PORT 21	// multicast service discovery
#define TCP_SERVER_PORT 7	// testing


static int	read_cnt;
static char	*read_ptr;
static char	read_buf[MAXLINE];

static ssize_t
my_read(int fd, char *ptr)
{

	if (read_cnt <= 0) {
again:
		if ( (read_cnt = read(fd, read_buf, sizeof(read_buf))) < 0) {
			if (errno == EINTR)
				goto again;
			return(-1);
		} else if (read_cnt == 0)
			return(0);
		read_ptr = read_buf;
	}

	read_cnt--;
	*ptr = *read_ptr++;
	return(1);
}

ssize_t
readline(int fd, void *vptr, size_t maxlen)
{
	ssize_t	n, rc;
	char	c, *ptr;

	ptr = vptr;
	for (n = 1; n < maxlen; n++) {
		if ( (rc = my_read(fd, &c)) == 1) {
			*ptr++ = c;
			//if (c == '\n')
			//	break;	/* newline is stored, like fgets() */
		} else if (rc == 0) {
			*ptr = 0;
			return(n - 1);	/* EOF, n - 1 bytes were read */
		} else
			return(-1);		/* error, errno set by read() */
	}

	*ptr = 0;	/* null terminate like fgets() */
	return(n);
}

/* end readline */

ssize_t
Readline(int fd, void *ptr, size_t maxlen)
{
	ssize_t		n;

	if ( (n = readline(fd, ptr, maxlen)) < 0)
		perror("readline error");
	return(n);
}




void
Fputs(const char *ptr, FILE *stream)
{
	if (fputs(ptr, stream) == EOF)
		perror("fputs error");
}


char *
Fgets(char *ptr, int n, FILE *stream)
{
	char	*rptr;

	if ( (rptr = fgets(ptr, n, stream)) == NULL && ferror(stream))
		perror("fgets error");

	return (rptr);
}


ssize_t						/* Write "n" bytes to a descriptor. */
writen(int fd, const void *vptr, size_t n)
{
	size_t		nleft;
	ssize_t		nwritten;
	const char	*ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
			if (nwritten < 0 && errno == EINTR)
				nwritten = 0;		/* and call write() again */
			else
				return(-1);			/* error */
		}

		nleft -= nwritten;
		ptr   += nwritten;
	}

	return(n);
}
/* end writen */

void
Writen(int fd, void *ptr, size_t nbytes)
{

	if (writen(fd, ptr, nbytes) != nbytes)
		perror("writen error");
}

void
str_cli(FILE *fp, int sockfd)
{
	char	sendline[MAXLINE], recvline[MAXLINE];

	printf("Enter text:");

	while (Fgets(sendline, MAXLINE, fp) != NULL) {

		Writen(sockfd, sendline, sizeof(sendline));

		if (Readline(sockfd, recvline, MAXLINE) == 0){
			perror("str_cli: server terminated prematurely");
			exit(0);
		}
		Fputs(recvline, stdout);
		printf("Enter text:");
	}
}

unsigned int
_if_nametoindex(const char *ifname)
{
	int s;
	struct ifreq ifr;
	unsigned int ni;

	s = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
	if (s != -1) {

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));

	if (ioctl(s, SIOCGIFINDEX, &ifr) != -1) {
			close(s);
			return (ifr.ifr_ifindex);
	}
		close(s);
		return -1;
	}
}



int snd_udp_socket(const char *serv, int port, SA **saptr, socklen_t *lenp){
	int sockfd;
	struct sockaddr_in6 *pservaddrv6;
	struct sockaddr_in *pservaddrv4;

	*saptr = malloc( sizeof(struct sockaddr_in6));

	pservaddrv6 = (struct sockaddr_in6*)*saptr;

	bzero(pservaddrv6, sizeof(struct sockaddr_in6));

	if (inet_pton(AF_INET6, serv, &pservaddrv6->sin6_addr) <= 0){

		free(*saptr);
		*saptr = malloc( sizeof(struct sockaddr_in));
		pservaddrv4 = (struct sockaddr_in*)*saptr;
		bzero(pservaddrv4, sizeof(struct sockaddr_in));

		if (inet_pton(AF_INET, serv, &pservaddrv4->sin_addr) <= 0){
			fprintf(stderr,"AF_INET inet_pton error for %s : %s \n", serv, strerror(errno));
			return -1;
		}else{
			pservaddrv4->sin_family = AF_INET;
			pservaddrv4->sin_port   = htons(port);
			*lenp =  sizeof(struct sockaddr_in);
			if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
				fprintf(stderr,"AF_INET socket error : %s\n", strerror(errno));
				return -1;
			}
		}

	}else{
		pservaddrv6 = (struct sockaddr_in6*)*saptr;
		pservaddrv6->sin6_family = AF_INET6;
		pservaddrv6->sin6_port   = htons(port);	/* daytime server */
		*lenp =  sizeof(struct sockaddr_in6);

		if ( (sockfd = socket(AF_INET6, SOCK_DGRAM, 0)) < 0){
			fprintf(stderr,"AF_INET6 socket error : %s\n", strerror(errno));
			return -1;
		}

	}

	return(sockfd);
}
/* end send_udp_socket */


int family_to_level(int family)
{
	switch (family) {
	case AF_INET:
		return IPPROTO_IP;

	case AF_INET6:
		return IPPROTO_IPV6;

	default:
		return -1;
	}
}

int mcast_join(int sockfd, const SA *grp, socklen_t grplen, const char *ifname, u_int ifindex){
	struct group_req req;
	if (ifindex > 0) {
		req.gr_interface = ifindex;
	} else if (ifname != NULL) {
		if ( (req.gr_interface = if_nametoindex(ifname)) == 0) {
			errno = ENXIO;	/* if name not found */
			return(-1);
		}
	} else
		req.gr_interface = 0;
	if (grplen > sizeof(req.gr_group)) {
		errno = EINVAL;
		return -1;
	}
	memcpy(&req.gr_group, grp, grplen);

	return (setsockopt(sockfd, family_to_level(grp->sa_family),MCAST_JOIN_GROUP, &req, sizeof(req)));
}


void recv_all(int recvfd, socklen_t salen, SA ** saptr)
{
	int					n;
	char				line[MAXLINE+1];
	socklen_t			len;
	struct sockaddr		*safrom;
	char str[128];
	struct sockaddr_in6*	 cliaddr;
	struct sockaddr_in*	 cliaddrv4;
	char			addr_str[INET6_ADDRSTRLEN+1];

	safrom = malloc(salen);

	//for ( ; ; ) {
		len = salen;
		if( (n = recvfrom(recvfd, line, MAXLINE, 0, safrom, &len)) < 0 )
		  perror("recvfrom() error");

		line[n] = 0;	/* null terminate */

		* saptr = safrom; //

		if( safrom->sa_family == AF_INET6 ){
		      cliaddr = (struct sockaddr_in6*) safrom;
		      inet_ntop(AF_INET6, (struct sockaddr  *) &cliaddr->sin6_addr,  addr_str, sizeof(addr_str));
		}
		else{
		      cliaddrv4 = (struct sockaddr_in*) safrom;
		      inet_ntop(AF_INET, (struct sockaddr  *) &cliaddrv4->sin_addr,  addr_str, sizeof(addr_str));
		}

		printf("Datagram from %s : %s (%d bytes)\n", addr_str, line, n);
		fflush(stdout);

	//}
}


void activate_service_discovery(char ** a){
	// udp part

	int sendfd, recvfd;
	const int on = 1;
	socklen_t salen;
	struct sockaddr	*sasend, *sarecv;
	struct sockaddr_in6 *ipv6addr;
	struct sockaddr_in *ipv4addr;


	// change nazwe fuction
	recvfd = snd_udp_socket(a[1], UDP_SEND_PORT, &sasend, &salen);


	if (setsockopt(recvfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0){
		fprintf(stderr,"setsockopt error : %s\n", strerror(errno));
		exit(1);
	}

	sarecv = malloc(salen);
	memcpy(sarecv, sasend, salen);

	if(sarecv->sa_family == AF_INET6){
	  ipv6addr = (struct sockaddr_in6 *) sarecv;
	  ipv6addr->sin6_addr =  in6addr_any;
	}

	if(sarecv->sa_family == AF_INET){
	  ipv4addr = (struct sockaddr_in *) sarecv;
	  ipv4addr->sin_addr.s_addr =  htonl(INADDR_ANY);
	}

	if( bind(recvfd, sarecv, salen) < 0 ){
	    fprintf(stderr,"bind error : %s\n", strerror(errno));
	    exit(1);
	}

	if( mcast_join(recvfd, sasend, salen, NULL, 0) < 0 ){
		fprintf(stderr,"mcast_join() error : %s\n", strerror(errno));
		exit(1);
	}

	struct sockaddr		* safrom;
	recv_all(recvfd, salen, &safrom);	/* child -> receives */

	/*	Print link-local addr, eg. fe80...	*/
	// char str[128];
	// struct sockaddr_in6*	 cliaddr;
	// struct sockaddr_in*	 cliaddrv4;
	// char			addr_str[INET6_ADDRSTRLEN+1];

	// if( safrom->sa_family == AF_INET6 ){
	// 		cliaddr = (struct sockaddr_in6*) safrom;
	// 		inet_ntop(AF_INET6, (struct sockaddr  *) &cliaddr->sin6_addr,  addr_str, sizeof(addr_str));
	// }
	// else{
	// 		cliaddrv4 = (struct sockaddr_in*) safrom;
	// 		inet_ntop(AF_INET, (struct sockaddr  *) &cliaddrv4->sin_addr,  addr_str, sizeof(addr_str));
	// }

	// printf("Addr %s\n", addr_str);
	// fflush(stdout);
}

void download_data(char ** arg){
	int					sockfd, n;
	struct sockaddr_in6	servaddr;
	char				recvline[MAXLINE + 1];
	int err;
	if ( (sockfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0){
		fprintf(stderr,"socket error : %s\n", strerror(errno));
		exit(1);
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin6_family = AF_INET6;
	servaddr.sin6_port   = htons(TCP_SERVER_PORT);	/* daytime server */
	if ( (err=inet_pton(AF_INET6, arg[2], &servaddr.sin6_addr)) <= 0){
		fprintf(stderr,"inet_pton error for %s : %s \n", arg[2], strerror(errno));
		exit(1);
	}
	if (connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) < 0){
		fprintf(stderr,"connect error : %s \n", strerror(errno));
		exit(1);
	}

	char filename[20];
	printf("Enter filename to download: ");
	scanf("%s", filename);

	Writen(sockfd, filename, strlen(filename));

	// buffer for name, scanf
	// download_data <- success/error msg

	while ( (n = read(sockfd, recvline, MAXLINE)) > 0) {
		recvline[n] = 0;	/* null terminate */
		if (fputs(recvline, stdout) == EOF){
			fprintf(stderr,"fputs error : %s\n", strerror(errno));
			exit(1);
		}
	}

	if (n < 0)
		fprintf(stderr,"read error : %s\n", strerror(errno));

	fflush(stdout);
	fprintf(stderr,"\nData downloaded\n");
}

void down_change_name(char ** arg){
	int					sockfd, n;
	struct sockaddr_in6	servaddr;
	char				recvline[MAXLINE + 1];
	int err;
	if ( (sockfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0){
		fprintf(stderr,"socket error : %s\n", strerror(errno));
		exit(1);
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin6_family = AF_INET6;
	servaddr.sin6_port   = htons(TCP_SERVER_PORT);	/* daytime server */
	if ( (err=inet_pton(AF_INET6, arg[2], &servaddr.sin6_addr)) <= 0){
		fprintf(stderr,"inet_pton error for %s : %s \n", arg[2], strerror(errno));
		exit(1);
	}
	if (connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) < 0){
		fprintf(stderr,"connect error : %s \n", strerror(errno));
		exit(1);
	}

	char filename[20];
	char newname[20];
	printf("Enter filename to download: ");
	scanf("%s", filename);
	printf("Enter new filename: ");
	scanf("%s", newname);


	Writen(sockfd, filename, strlen(filename));


	// buffer for name, scanf
	// download_data <- success/error msg
	FILE *file = fopen(newname, "w");



	while ( (n = read(sockfd, recvline, MAXLINE)) > 0) {
		recvline[n] = 0;	/* null terminate */
		fputs(recvline,file);
		if (fputs(recvline, stdout) == EOF){
			fprintf(stderr,"fputs error : %s\n", strerror(errno));
			exit(1);
		}
	}

	if (n < 0)
		fprintf(stderr,"read error : %s\n", strerror(errno));



	fflush(stdout);
	fclose(file);

	fprintf(stderr,"\nData downloaded into %s\n",newname);

}

void remove_file(char ** arg){
	int					sockfd, n;
	struct sockaddr_in6	servaddr;
	char				recvline[MAXLINE + 1];
	int err;
	if ( (sockfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0){
		fprintf(stderr,"socket error : %s\n", strerror(errno));
		exit(1);
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin6_family = AF_INET6;
	servaddr.sin6_port   = htons(TCP_SERVER_PORT);	/* daytime server */
	if ( (err=inet_pton(AF_INET6, arg[2], &servaddr.sin6_addr)) <= 0){
		fprintf(stderr,"inet_pton error for %s : %s \n", arg[2], strerror(errno));
		exit(1);
	}
	if (connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) < 0){
		fprintf(stderr,"connect error : %s \n", strerror(errno));
		exit(1);
	}
	char buf[100];
	char filename[20];
	printf("Enter filename to delete: ");
	scanf("%s\n",filename);
	strcpy(buf, filename);
	send(sockfd, buf, 100, 0);
	recv(sockfd, buf, 100, 0);

	fflush(stdout);

	fprintf(stderr,"\n%s deleted from server \n",filename);

}

int
main(int argc, char **argv){
	if (argc != 3){
		fprintf(stderr, "usage: %s  <IP multicast group> <server ipv6>\n", argv[0]);
		return 1;
	}

  int choice;
  while(1){
      printf("Enter a choice:\n1 - print file contents\n2 - copy to local file\n3 - delete\n4 - copy\n5 - quit\n");
      scanf("%d", &choice);

      switch(choice){
		  case(1):
				activate_service_discovery(argv);
				download_data(argv);

		  break;

		  case(2):
				activate_service_discovery(argv);
				down_change_name(argv);

			case(3):
				activate_service_discovery(argv);
				remove_file(argv);

			break;

			case(4):
				activate_service_discovery(argv);
			break;
				activate_service_discovery(argv);
			case(5):
				exit(0);

			break;

  }
exit(0);

}

}
