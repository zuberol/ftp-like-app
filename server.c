#include        <sys/types.h>   /* basic system data types */
#include        <sys/socket.h>  /* basic socket definitions */
#include        <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include        <arpa/inet.h>   /* inet(3) functions */
#include        <errno.h>
#include        <signal.h>
#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include 	<sys/ioctl.h>
#include 	<unistd.h>
#include 	<net/if.h>
#include	<netdb.h>
#include	<sys/utsname.h>
#include	<linux/un.h>
#include <dirent.h>				/*	POSIX	*/


#define SA    struct sockaddr
#define TCP_LISTEN_PORT 7	// testing
#define UDP_SEND_PORT 21	// multicast service discovery
#define	SENDRATE	5		/* send one service discovery datagram every five seconds */
#define MAXLINE 1024
#define LISTENQ 2

// globals
int g_sendfd;
SA * g_sasend;
socklen_t g_salen;


// declarations
void send_all(int, SA *, socklen_t);



#define M_ALARM
#ifdef M_ALARM
void sig_alarm(int signo){
	printf("pid=%ld   ppid=%ld	|	sends multicast msg\n", (long)getpid(), (long)getppid());
	//send
	send_all(g_sendfd, g_sasend, g_salen);	/* parent -> sends */
	alarm(5);

}

int m_signal(int signum, void handler(int)){
    struct sigaction new_action, old_action;

  /* Set up the structure to specify the new action. */
    new_action.sa_handler = handler;
    sigemptyset (&new_action.sa_mask);
    new_action.sa_flags = SA_RESTART;

    if( sigaction (signum, &new_action, &old_action) < 0 ){
          fprintf(stderr,"sigaction error : %s\n", strerror(errno));
          return 1;
    }

	return 0;
}
#endif

int family_to_level(int family){
	switch (family) {
        case AF_INET:
            return IPPROTO_IP;
        case AF_INET6:
            return IPPROTO_IPV6;

        default:
            return -1;
	}
}

unsigned int _if_nametoindex(const char *ifname){
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

void send_all(int sendfd, SA *sadest, socklen_t salen){
	char		line[MAXLINE];
	struct utsname	myname;

	if (uname(&myname) < 0)
		perror("uname error");
	//snprintf(line, sizeof(line), "%s, PID=%d", myname.nodename, getpid());

	//ls directory files
	DIR *d;
	struct dirent *dir;
	d = opendir(".");
	if (d) {
		int len = 0;
		while ((dir = readdir(d)) != NULL) {
			 if( !strcmp(dir->d_name, ".") || !strcmp(dir->d_name, "..") ) continue;
			len += sprintf(line+len, dir->d_name);
			len += sprintf(line+len, "\n");
		}
		closedir(d);
	}
	//

	if(sendto(sendfd, line, strlen(line), 0, sadest, salen) < 0 )
		fprintf(stderr,"sendto() error : %s\n", strerror(errno));
}

/* Write "n" bytes to a descriptor. */
ssize_t	writen(int fd, const void *vptr, size_t n){
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
/* end writen() */

void Writen(int fd, void *ptr, size_t nbytes){
	if (writen(fd, ptr, nbytes) != nbytes)
		perror("writen error");
}

void str_echo(int sockfd){

	FILE * pFile;
	long lSize;
	char * buffer;
	size_t result;

	char* filename;
	char buff[100];
	recv(g_sendfd, buff, 100, 0);
	sscanf(buff, "%s", filename);



	pFile = fopen ( filename , "r" );
	if (pFile==NULL) {
		fputs ("File error",stderr);
		exit (1);
	}

	// obtain file size:
	fseek (pFile , 0 , SEEK_END);
	lSize = ftell (pFile);
	rewind (pFile);

	// allocate memory to contain the whole file:
	buffer = (char*) malloc (sizeof(char)*lSize);
	if (buffer == NULL) {
		fputs ("Memory error",stderr);
		exit (2);
	}

	// copy the file into the buffer:
	result = fread (buffer,1,lSize,pFile);
	if (result != lSize) {
		fputs ("Reading error",stderr);
		exit (3);
	}

	/* send data now */
	Writen(sockfd, buffer, lSize);

	// terminate
	fclose (pFile);
	free (buffer);

}


int main(int argc, char **argv){

	if (argc != 2){
		fprintf(stderr, "usage: %s  <IP-multicast-address>\n", argv[0]);
		return 1;
	}

	g_sendfd = snd_udp_socket(argv[1], UDP_SEND_PORT, &g_sasend, &g_salen);

    // TCP part
	int					listenfd, connfd;
	pid_t				childpid;
	socklen_t			clilen;
	struct sockaddr_in6	cliaddr, servaddr;


	// take care of fired signals
	signal(SIGCHLD, SIG_IGN);
	m_signal(SIGALRM, sig_alarm);

	// socket

	if ( (listenfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0){
		fprintf(stderr,"socket error : %s\n", strerror(errno));
		return 1;
   }
   int optval = 1;
   if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0){
                fprintf(stderr,"SO_REUSEADDR setsockopt error : %s\n", strerror(errno));
   }

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin6_family = AF_INET6;
	servaddr.sin6_addr   = in6addr_any;
	servaddr.sin6_port   = htons(TCP_LISTEN_PORT);	/* echo server */

   if ( bind( listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0){
           fprintf(stderr,"bind error : %s\n", strerror(errno));
           return 1;
   }

   if ( listen(listenfd, LISTENQ) < 0){
           fprintf(stderr,"listen error : %s\n", strerror(errno));
           return 1;
   }


	alarm(1);

	for ( ; ; ) {
		clilen = sizeof(cliaddr);
		if ( (connfd = accept(listenfd, (SA *) &cliaddr, &clilen)) < 0) {
			if (errno == EINTR)
				continue;		/* back to for() */
			else
				perror("accept error");
				exit(1);
		}

	}

		if ( (childpid = fork()) == 0) {	/* child process */
			alarm(0);
			close(g_sendfd);		/* Do not send multicast msg if child */
			close(listenfd);	/* close listening socket */
			str_echo(connfd);	/* process the request */
			exit(0);
		}
		close(connfd);			/* parent closes connected socket */
	}

	return 0;
}
