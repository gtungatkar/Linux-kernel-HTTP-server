#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
 .arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xa7672d5a, "module_layout" },
	{ 0x20623cac, "kmalloc_caches" },
	{ 0x5e3f6df6, "sock_setsockopt" },
	{ 0x6e7e4bf9, "sock_release" },
	{ 0x105e2727, "__tracepoint_kmalloc" },
	{ 0xeffc3f4f, "sock_recvmsg" },
	{ 0x3c2c5af5, "sprintf" },
	{ 0xb72397d5, "printk" },
	{ 0x42224298, "sscanf" },
	{ 0xdc5664c2, "sock_sendmsg" },
	{ 0xb4390f9a, "mcount" },
	{ 0x6eb25b1e, "kmem_cache_alloc" },
	{ 0x50ac791f, "sock_create" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "940B822C8799613D0003066");
