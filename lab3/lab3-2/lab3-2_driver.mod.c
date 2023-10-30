#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif


static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x92997ed8, "_printk" },
	{ 0x73f3aefa, "gpio_to_desc" },
	{ 0x89d80c4e, "gpiod_unexport" },
	{ 0xfe990052, "gpio_free" },
	{ 0x815b93c3, "device_destroy" },
	{ 0xe730cdd8, "class_destroy" },
	{ 0x76b48bd9, "cdev_del" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0xf91c2900, "gpiod_direction_output_raw" },
	{ 0xdce6b6f0, "gpiod_export" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0x7b6d404c, "cdev_init" },
	{ 0x6499ae55, "cdev_add" },
	{ 0x4cbcc1fd, "__class_create" },
	{ 0x600df7e7, "device_create" },
	{ 0x47229b5c, "gpio_request" },
	{ 0xc60eaec7, "gpiod_get_raw_value" },
	{ 0x6cbbfc54, "__arch_copy_to_user" },
	{ 0x8da6585d, "__stack_chk_fail" },
	{ 0x12a4e128, "__arch_copy_from_user" },
	{ 0xdcb764ad, "memset" },
	{ 0x7682ba4e, "__copy_overflow" },
	{ 0x1aedfe4a, "gpiod_set_raw_value" },
	{ 0xf9a482f9, "msleep" },
	{ 0x87b40f76, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "15EEFA2571044960B434B39");
