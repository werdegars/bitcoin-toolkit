#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include "node.h"
#include "mem.h"
#include "assert.h"

struct Node {
	int sockfd;
};

Node node_connect(const char *host, int port) {
	int t, sockfd = 0;
	struct hostent *server;
	struct sockaddr_in serv_addr;
	Node r;
	
	assert(host);
	assert(port);
	
	// Set up a socket in the AF_INET domain (Internet Protocol v4 addresses)
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	assert(sockfd >= 0);
	
	// Get a pointer to 'hostent' containing info about host.
	server = gethostbyname(host);
	assert(server);
	
	// Initializing serv_addr memory footprint to all integer zeros ('\0')
	memset(&serv_addr, 0, sizeof(serv_addr));
	
	// Setting up our serv_addr structure
	serv_addr.sin_family = AF_INET;       // Internet Protocol v4 addresses
	memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
	serv_addr.sin_port = htons(port);     // Convert port byte order to 'network byte order'
	
	// Connect to server. Error if can't connect.
	t = connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr));
	assert(t >= 0);
	
	NEW(r);

	r->sockfd = sockfd;
	
	return r;
}

int node_socket(Node n) {
	assert(n);
	
	return n->sockfd;
}

void node_disconnect(Node n) {
	assert(n);
	assert(n->sockfd);
	
	close(n->sockfd);
}

