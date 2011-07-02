/* 
 * ksocket project test sample - tcp client
 * BSD-style socket APIs for kernel 2.6 developers
 * 
 * @2007-2008, China
 * @song.xian-guang@hotmail.com (MSN Accounts)
 * 
 * This code is licenced under the GPL
 * Feel free to contact me if any questions
 * 
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/socket.h>
#include <linux/net.h>
#include <net/sock.h>
#include <asm/processor.h>
#include <asm/uaccess.h>
#include <linux/in.h>

#include <net/ksocket.h>

int tcp_cli(void *arg)
{
	ksocket_t sockfd_cli;
	struct sockaddr_in addr_srv;
	char buf[1024], *tmp;
	int addr_len;

#ifdef KSOCKET_ADDR_SAFE
	mm_segment_t old_fs;
	old_fs = get_fs();
	set_fs(KERNEL_DS);
#endif

	sprintf(current->comm, "ksktcli");	

	memset(&addr_srv, 0, sizeof(addr_srv));
	addr_srv.sin_family = AF_INET;
	addr_srv.sin_port = htons(22);
	addr_srv.sin_addr.s_addr = inet_addr("127.0.0.1");;
	addr_len = sizeof(struct sockaddr_in);
	
	sockfd_cli = ksocket(AF_INET, SOCK_STREAM, 0);
	printk("sockfd_cli = 0x%p\n", sockfd_cli);
	if (sockfd_cli == NULL)
	{
		printk("socket failed\n");
		return -1;
	}
	if (kconnect(sockfd_cli, (struct sockaddr*)&addr_srv, addr_len) < 0)
	{
		printk("connect failed\n");
		return -1;
	}

	tmp = inet_ntoa(&addr_srv.sin_addr);
	printk("connected to : %s %d\n", tmp, ntohs(addr_srv.sin_port));
	kfree(tmp);
	
	krecv(sockfd_cli, buf, 1024, 0);
	printk("got message : %s\n", buf);

	kclose(sockfd_cli);
#ifdef KSOCKET_ADDR_SAFE
		set_fs(old_fs);
#endif
	
	return 0;
}

static int ksocket_tcp_cli_init(void)
{
	kernel_thread(tcp_cli, NULL, 0);
	
	printk("ksocket tcp cli init ok\n");
	return 0;
}

static void ksocket_tcp_cli_exit(void)
{
	printk("ksocket tcp cli exit\n");
}

module_init(ksocket_tcp_cli_init);
module_exit(ksocket_tcp_cli_exit);

MODULE_LICENSE("Dual BSD/GPL");
