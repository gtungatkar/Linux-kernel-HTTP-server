/* 
 * ksocket project
 * BSD-style socket APIs for kernel 2.6 developers
 * 
 * @2007-2008, China
 * @song.xian-guang@hotmail.com (MSN Accounts)
 * 
 * This code is licenced under the GPL
 * Feel free to contact me if any questions
 * 
 */

#ifndef _ksocket_h_
#define _ksocket_h_

struct socket;
struct sockaddr;
struct in_addr;
typedef struct socket * ksocket_t;

/* BSD socket APIs prototype declaration */
ksocket_t ksocket(int domain, int type, int protocol);
int kshutdown(ksocket_t socket, int how);
int kclose(ksocket_t socket);

int kbind(ksocket_t socket, struct sockaddr *address, int address_len);
int klisten(ksocket_t socket, int backlog);
int kconnect(ksocket_t socket, struct sockaddr *address, int address_len);
ksocket_t kaccept(ksocket_t socket, struct sockaddr *address, int *address_len);

ssize_t krecv(ksocket_t socket, void *buffer, size_t length, int flags);
ssize_t ksend(ksocket_t socket, const void *buffer, size_t length, int flags);
ssize_t krecvfrom(ksocket_t socket, void * buffer, size_t length, int flags, struct sockaddr * address, int * address_len);
ssize_t ksendto(ksocket_t socket, void *message, size_t length, int flags, const struct sockaddr *dest_addr, int dest_len);

int kgetsockname(ksocket_t socket, struct sockaddr *address, int *address_len);
int kgetpeername(ksocket_t socket, struct sockaddr *address, int *address_len);
int ksetsockopt(ksocket_t socket, int level, int optname, void *optval, int optlen);
int kgetsockopt(ksocket_t socket, int level, int optname, void *optval, int *optlen);

unsigned int inet_addr(char* ip);
char *inet_ntoa(struct in_addr *in); /* DO NOT forget to kfree the return pointer */

#endif /* !_ksocket_h_ */
