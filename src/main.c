/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/version.h>

int main(void)
{
	printk("\n========================================\n");
	printk("  Zephyr RTOS on TM4C123G LaunchPad\n");
	printk("  Board: %s\n", CONFIG_BOARD_TARGET);
	printk("  Zephyr OS: %s\n", KERNEL_VERSION_STRING);
	printk("========================================\n\n");

	uint32_t counter = 0;
	while (1) {
		printk("[Heartbeat %u] System uptime: %lld ms\n", counter++, k_uptime_get());
		k_msleep(1000);
	}

	return 0;
}
