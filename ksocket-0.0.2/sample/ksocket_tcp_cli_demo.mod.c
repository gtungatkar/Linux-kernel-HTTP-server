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
	{ 0x2e9014ad, "per_cpu__current_task" },
	{ 0x3c2c5af5, "sprintf" },
	{ 0x10cde6fd, "inet_ntoa" },
	{ 0x2f5e1f1c, "krecv" },
	{ 0xb72397d5, "printk" },
	{ 0xb4390f9a, "mcount" },
	{ 0x4a8736a2, "kconnect" },
	{ 0x84856d65, "inet_addr" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x76654ed8, "kclose" },
	{ 0x37a0cba, "kfree" },
	{ 0x7e9ebb05, "kernel_thread" },
	{ 0x57359d74, "ksocket" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=ksocket";


MODULE_INFO(srcversion, "056CB681F8D0DEDAB596FD9");
