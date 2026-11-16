/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/version.h>
#include <zephyr/shell/shell.h>
#include <zephyr/devicetree.h>

/* Custom Shell Command: tm4c info */
static int cmd_tm4c_info(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "========================================");
	shell_print(sh, "  Target Board : %s", CONFIG_BOARD_TARGET);
	shell_print(sh, "  SOC          : TI TM4C123GH6PM (ARM Cortex-M4F)");
	shell_print(sh, "  Clock        : 80 MHz");
	shell_print(sh, "  Flash / RAM  : 256 KB Flash / 32 KB SRAM");
	shell_print(sh, "  Zephyr RTOS  : %s", KERNEL_VERSION_STRING);
	shell_print(sh, "========================================");
	return 0;
}

/* Custom Shell Command: tm4c uptime */
static int cmd_tm4c_uptime(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int64_t uptime_ms = k_uptime_get();
	uint32_t secs = (uint32_t)(uptime_ms / 1000);
	uint32_t mins = secs / 60;
	uint32_t hours = mins / 60;

	shell_print(sh, "System Uptime: %u h %u m %u s (%lld ms)",
		    hours, mins % 60, secs % 60, uptime_ms);
	return 0;
}

#if IS_ENABLED(CONFIG_APP_ENABLE_DTS_INSPECTION_CMD)
/* Custom Shell Command: tm4c dts (Devicetree Introspection) */
static int cmd_tm4c_dts(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "=== Devicetree Compile-Time Introspection ===");
	shell_print(sh, "Target Board  : %s", CONFIG_BOARD_TARGET);
	shell_print(sh, "UART0 Status  : %s (Baud: %d)",
		    DT_NODE_HAS_STATUS(DT_NODELABEL(uart0), okay) ? "okay" : "disabled",
		    DT_PROP(DT_NODELABEL(uart0), current_speed));
	shell_print(sh, "SRAM Base/Size: 0x%08x / %u KB",
		    (uint32_t)DT_REG_ADDR(DT_NODELABEL(sram0)),
		    (uint32_t)(DT_REG_SIZE(DT_NODELABEL(sram0)) / 1024));
	shell_print(sh, "Flash Base/Sz : 0x%08x / %u KB",
		    (uint32_t)DT_REG_ADDR(DT_NODELABEL(flash0)),
		    (uint32_t)(DT_REG_SIZE(DT_NODELABEL(flash0)) / 1024));
	shell_print(sh, "=============================================");
	return 0;
}
#endif

SHELL_STATIC_SUBCMD_SET_CREATE(sub_tm4c,
	SHELL_CMD(info, NULL, "Print board & hardware specifications", cmd_tm4c_info),
	SHELL_CMD(uptime, NULL, "Print formatted system uptime", cmd_tm4c_uptime),
#if IS_ENABLED(CONFIG_APP_ENABLE_DTS_INSPECTION_CMD)
	SHELL_CMD(dts, NULL, "Print Devicetree compile-time properties", cmd_tm4c_dts),
#endif
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(tm4c, &sub_tm4c, "TM4C123 LaunchPad custom commands", NULL);

int main(void)
{
#if IS_ENABLED(CONFIG_APP_WELCOME_BANNER)
	printk("\n");
	printk("==============================================\n");
	printk("   TI TM4C123GXL Zephyr Shell Terminal\n");
	printk("   Zephyr OS %s | Board: %s\n", KERNEL_VERSION_STRING, CONFIG_BOARD_TARGET);
	printk("   Type 'help' or 'tm4c --help' for commands\n");
	printk("==============================================\n\n");
#endif

	/* Main thread can sleep; the shell runs in its own dedicated thread */
	while (1) {
		k_sleep(K_FOREVER);
	}

	return 0;
}
