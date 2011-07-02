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
	{ 0x12da5bb2, "__kmalloc" },
	{ 0x5846d6b9, "generic_file_llseek" },
	{ 0x6980fe91, "param_get_int" },
	{ 0xd0d8621b, "strlen" },
	{ 0x20000329, "simple_strtoul" },
	{ 0x105e2727, "__tracepoint_kmalloc" },
	{ 0x7b8d93fd, "remove_proc_entry" },
	{ 0xff964b25, "param_set_int" },
	{ 0x3c2c5af5, "sprintf" },
	{ 0xcadf1b7, "kbind" },
	{ 0xe2d5255a, "strcmp" },
	{ 0xe83fea1, "del_timer_sync" },
	{ 0x891d4342, "proc_mkdir" },
	{ 0x8d3894f2, "_ctype" },
	{ 0x2f5e1f1c, "krecv" },
	{ 0x70d1f8f3, "strncat" },
	{ 0xb72397d5, "printk" },
	{ 0xb4390f9a, "mcount" },
	{ 0x7a47dbc8, "destroy_workqueue" },
	{ 0x33cf43a, "ksend" },
	{ 0x2e676558, "__create_workqueue_key" },
	{ 0xfa733bba, "fput" },
	{ 0xac073164, "flush_workqueue" },
	{ 0x61651be, "strcat" },
	{ 0xfc6256b9, "boot_tvec_bases" },
	{ 0x6eb25b1e, "kmem_cache_alloc" },
	{ 0xbf8184ed, "kaccept" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x4ac7113d, "create_proc_entry" },
	{ 0x76654ed8, "kclose" },
	{ 0x37a0cba, "kfree" },
	{ 0x701d0ebd, "snprintf" },
	{ 0x57359d74, "ksocket" },
	{ 0x68131a56, "klisten" },
	{ 0x31ac5008, "ksetsockopt" },
	{ 0xd6c963c, "copy_from_user" },
	{ 0x91d5c046, "queue_delayed_work" },
	{ 0xe914e41e, "strcpy" },
	{ 0x15307093, "filp_open" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=ksocket";


MODULE_INFO(srcversion, "BDEE2E1C08304AC185BA2F7");
