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
#include <linux/module.h>
#include <linux/string.h>
#include <linux/socket.h>
#include <linux/net.h>
#include <linux/in.h>
#include <net/sock.h>
#include <asm/processor.h>
#include <asm/uaccess.h>
#include "ksocket.h"
#include "sxgdebug.h"

#define KSOCKET_NAME	"ksocket"
#define KSOCKET_VERSION	"0.0.2"
#define KSOCKET_DESCPT	"BSD-style socket APIs for kernel 2.6 developers"
#define KSOCKET_AUTHOR	"msn : song.xian-guang@hotmail.com\n"\
						"blog: http://sxg.cublog.cn"
#define KSOCKET_DATE	"2008-05-15"

MODULE_AUTHOR(KSOCKET_AUTHOR);
MODULE_DESCRIPTION(KSOCKET_NAME"-"KSOCKET_VERSION"\n"KSOCKET_DESCPT);
MODULE_LICENSE("Dual BSD/GPL");

ksocket_t ksocket(int domain, int type, int protocol)
{
	struct socket *sk = NULL;
	int ret = 0;
	
	ret = sock_create(domain, type, protocol, &sk);
	if (ret < 0)
	{
		sxg_debug("sock_create failed\n");
		return NULL;
	}
	
	sxg_debug("sock_create sk= 0x%p\n", sk);
	
	return sk;
}

int kbind(ksocket_t socket, struct sockaddr *address, int address_len)
{
	struct socket *sk;
	int ret = 0;

	sk = (struct socket *)socket;
	ret = sk->ops->bind(sk, address, address_len);
	sxg_debug("kbind ret = %d\n", ret);
	
	return ret;
}

int klisten(ksocket_t socket, int backlog)
{
	struct socket *sk;
	int ret;

	sk = (struct socket *)socket;
	
	if ((unsigned)backlog > SOMAXCONN)
		backlog = SOMAXCONN;
	
	ret = sk->ops->listen(sk, backlog);
	
	return ret;
}

int kconnect(ksocket_t socket, struct sockaddr *address, int address_len)
{
	struct socket *sk;
	int ret;

	sk = (struct socket *)socket;
	ret = sk->ops->connect(sk, address, address_len, 0/*sk->file->f_flags*/);
	
	return ret;
}

ksocket_t kaccept(ksocket_t socket, struct sockaddr *address, int *address_len)
{
	struct socket *sk;
	struct socket *new_sk = NULL;
	int ret;
	
	sk = (struct socket *)socket;

	sxg_debug("family = %d, type = %d, protocol = %d\n",
					sk->sk->sk_family, sk->type, sk->sk->sk_protocol);
	//new_sk = sock_alloc();
	//sock_alloc() is not exported, so i use sock_create() instead
	ret = sock_create(sk->sk->sk_family, sk->type, sk->sk->sk_protocol, &new_sk);
	if (ret < 0)
		return NULL;
	if (!new_sk)
		return NULL;
	
	new_sk->type = sk->type;
	new_sk->ops = sk->ops;
	
	ret = sk->ops->accept(sk, new_sk, O_NONBLOCK /*sk->file->f_flags*/);
	if (ret < 0)
		goto error_kaccept;
	
	if (address)
	{
		ret = new_sk->ops->getname(new_sk, address, address_len, 2);
		if (ret < 0)
			goto error_kaccept;
	}
	
	return new_sk;

error_kaccept:
	sock_release(new_sk);
	return NULL;
}

ssize_t krecv(ksocket_t socket, void *buffer, size_t length, int flags)
{
	struct socket *sk;
	struct msghdr msg;
	struct iovec iov;
	int ret;
#ifndef KSOCKET_ADDR_SAFE
	mm_segment_t old_fs;
#endif

	sk = (struct socket *)socket;

	iov.iov_base = (void *)buffer;
	iov.iov_len = (__kernel_size_t)length;

	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = NULL;
	msg.msg_controllen = 0;

	/*
	 * msg.msg_iov->iov_base is declared as follows:
	 * void __user *iov_base;
	 * which means there is an user space pointer in 'msg'
	 * use set_fs(KERNEL_DS) to make the pointer safe to kernel space
	 */
#ifndef KSOCKET_ADDR_SAFE
	old_fs = get_fs();
	set_fs(KERNEL_DS);
#endif
	ret = sock_recvmsg(sk, &msg, length, flags);
#ifndef KSOCKET_ADDR_SAFE
	set_fs(old_fs);
#endif
	if (ret < 0)
		goto out_krecv;
	//ret = msg.msg_iov.iov_len;//?
	
out_krecv:
	return ret;

}

ssize_t ksend(ksocket_t socket, const void *buffer, size_t length, int flags)
{
	struct socket *sk;
	struct msghdr msg;
	struct iovec iov;
	int len;
#ifndef KSOCKET_ADDR_SAFE
	mm_segment_t old_fs;
#endif

	sk = (struct socket *)socket;

	iov.iov_base = (void *)buffer;
	iov.iov_len = (__kernel_size_t)length;

	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = NULL;
	msg.msg_controllen = 0;

	msg.msg_flags = flags;

#ifndef KSOCKET_ADDR_SAFE
	old_fs = get_fs();
	set_fs(KERNEL_DS);
#endif
	len = sock_sendmsg(sk, &msg, length);//?
#ifndef KSOCKET_ADDR_SAFE
	set_fs(old_fs);
#endif
	
	return len;//len ?
}

int kshutdown(ksocket_t socket, int how)
{
	struct socket *sk;
	int ret = 0;

	sk = (struct socket *)socket;
	if (sk)
		ret = sk->ops->shutdown(sk, how);
	
	return ret;
}

//TODO: ?
int kclose(ksocket_t socket)
{
	struct socket *sk;
	int ret;

	sk = (struct socket *)socket;
	ret = sk->ops->release(sk);

	if (sk)
		sock_release(sk);

	return ret;
}

ssize_t krecvfrom(ksocket_t socket, void * buffer, size_t length,
              int flags, struct sockaddr * address,
              int * address_len)
{
	struct socket *sk;
	struct msghdr msg;
	struct iovec iov;
	int len;
#ifndef KSOCKET_ADDR_SAFE
	mm_segment_t old_fs;
#endif

	sk = (struct socket *)socket;

	iov.iov_base = (void *)buffer;
	iov.iov_len = (__kernel_size_t)length;

	msg.msg_name = address;
	msg.msg_namelen = 128;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	
#ifndef KSOCKET_ADDR_SAFE
	old_fs = get_fs();
	set_fs(KERNEL_DS);
#endif
	len = sock_recvmsg(sk, &msg, length, flags);
#ifndef KSOCKET_ADDR_SAFE
	set_fs(old_fs);
#endif

	if (address)
	{
		*address_len = msg.msg_namelen;
	}
	
	return len;
}

ssize_t ksendto(ksocket_t socket, void *message, size_t length,
              int flags, const struct sockaddr *dest_addr,
              int dest_len)
{
	struct socket *sk;
	struct msghdr msg;
	struct iovec iov;
	int len;
#ifndef KSOCKET_ADDR_SAFE
	mm_segment_t old_fs;
#endif

	sk = (struct socket *)socket;

	iov.iov_base = (void *)message;
	iov.iov_len = (__kernel_size_t)length;

	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = NULL;
	msg.msg_controllen = 0;

	msg.msg_flags = flags;
	if (dest_addr)
	{
		msg.msg_name = (void *)dest_addr;
		msg.msg_namelen = dest_len;
	}

#ifndef KSOCKET_ADDR_SAFE
	old_fs = get_fs();
	set_fs(KERNEL_DS);
#endif
	len = sock_sendmsg(sk, &msg, length);//?
#ifndef KSOCKET_ADDR_SAFE
	set_fs(old_fs);
#endif
	
	return len;//len ?
}

int kgetsockname(ksocket_t socket, struct sockaddr *address, int *address_len)
{
	struct socket *sk;
	int ret;
	
	sk = (struct socket *)socket;
	ret = sk->ops->getname(sk, address, address_len, 0);
	
	return ret;
}

int kgetpeername(ksocket_t socket, struct sockaddr *address, int *address_len)
{
	struct socket *sk;
	int ret;
	
	sk = (struct socket *)socket;
	ret = sk->ops->getname(sk, address, address_len, 1);
	
	return ret;
}

int ksetsockopt(ksocket_t socket, int level, int optname, void *optval, int optlen)
{
	struct socket *sk;
	int ret;
#ifndef KSOCKET_ADDR_SAFE
	mm_segment_t old_fs;
#endif

	sk = (struct socket *)socket;

#ifndef KSOCKET_ADDR_SAFE
	old_fs = get_fs();
	set_fs(KERNEL_DS);
#endif

	if (level == SOL_SOCKET)
		ret = sock_setsockopt(sk, level, optname, optval, optlen);
	else
		ret = sk->ops->setsockopt(sk, level, optname, optval, optlen);

#ifndef KSOCKET_ADDR_SAFE	
	set_fs(old_fs);
#endif

	return ret;
}

int kgetsockopt(ksocket_t socket, int level, int optname, void *optval, int *optlen)
{
/*	struct socket *sk;
	int ret;
	mm_segment_t old_fs;

	sk = (struct socket *)socket;
	
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	if (level == SOL_SOCKET)
		ret = sock_getsockopt(sk, level, optname, optval, optlen);
	else
		ret = sk->ops->getsockopt(sk, level, optname, optval, optlen);
	
	set_fs(old_fs);

	return ret;
*/
	return -ENOSYS;
}


//helper functions
unsigned int inet_addr(char* ip)
{
	int a, b, c, d;
	char addr[4];
	
	sscanf(ip, "%d.%d.%d.%d", &a, &b, &c, &d);
	addr[0] = a;
	addr[1] = b;
	addr[2] = c;
	addr[3] = d;
	
	return *(unsigned int *)addr;
}

char *inet_ntoa(struct in_addr *in)
{
	char* str_ip = NULL;
	u_int32_t int_ip = 0;
	
	str_ip = kmalloc(16 * sizeof(char), GFP_KERNEL);
	if (!str_ip)
		return NULL;
	else
		memset(str_ip, 0, 16);

	int_ip = in->s_addr;
	
	sprintf(str_ip, "%d.%d.%d.%d",  (int_ip      ) & 0xFF,
									(int_ip >> 8 ) & 0xFF,
									(int_ip >> 16) & 0xFF,
									(int_ip >> 24) & 0xFF);
	return str_ip;
}

//module init and cleanup procedure
static int ksocket_init(void)
{
	printk("%s version %s\n%s\n%s\n", 
		KSOCKET_NAME, KSOCKET_VERSION,
		KSOCKET_DESCPT, KSOCKET_AUTHOR);

	return 0;
}

static void ksocket_exit(void)
{
	printk("ksocket exit\n");
}

module_init(ksocket_init);
module_exit(ksocket_exit);

EXPORT_SYMBOL(ksocket);
EXPORT_SYMBOL(kbind);
EXPORT_SYMBOL(klisten);
EXPORT_SYMBOL(kconnect);
EXPORT_SYMBOL(kaccept);
EXPORT_SYMBOL(krecv);
EXPORT_SYMBOL(ksend);
EXPORT_SYMBOL(kshutdown);
EXPORT_SYMBOL(kclose);
EXPORT_SYMBOL(krecvfrom);
EXPORT_SYMBOL(ksendto);
EXPORT_SYMBOL(kgetsockname);
EXPORT_SYMBOL(kgetpeername);
EXPORT_SYMBOL(ksetsockopt);
EXPORT_SYMBOL(kgetsockopt);
EXPORT_SYMBOL(inet_addr);
EXPORT_SYMBOL(inet_ntoa);
