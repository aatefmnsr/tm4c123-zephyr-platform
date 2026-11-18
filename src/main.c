/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/version.h>
#include <zephyr/shell/shell.h>
#include <zephyr/devicetree.h>

#if IS_ENABLED(CONFIG_APP_ENABLE_GPIO_DEMO)
#include <zephyr/drivers/gpio.h>

/* LED DT Specs */
static const struct gpio_dt_spec led_red = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec led_blue = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec led_green = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

/* Button DT Specs */
static const struct gpio_dt_spec btn_sw1 = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static const struct gpio_dt_spec btn_sw2 = GPIO_DT_SPEC_GET(DT_ALIAS(sw1), gpios);

static struct gpio_callback btn_cb_data;
static volatile uint32_t sw1_press_count;
static volatile uint32_t sw2_press_count;

static void button_pressed_callback(const struct device *dev,
				    struct gpio_callback *cb,
				    uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);

	if (pins & BIT(btn_sw1.pin)) {
		sw1_press_count++;
		gpio_pin_toggle_dt(&led_red);
	}

	if (pins & BIT(btn_sw2.pin)) {
		sw2_press_count++;
		gpio_pin_toggle_dt(&led_blue);
	}
}

static int init_gpios(void)
{
	int ret;

	if (!gpio_is_ready_dt(&led_red) ||
	    !gpio_is_ready_dt(&led_blue) ||
	    !gpio_is_ready_dt(&led_green)) {
		printk("Error: LED GPIO device not ready\n");
		return -ENODEV;
	}

	gpio_pin_configure_dt(&led_red, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&led_blue, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&led_green, GPIO_OUTPUT_INACTIVE);

	if (!gpio_is_ready_dt(&btn_sw1) || !gpio_is_ready_dt(&btn_sw2)) {
		printk("Error: Button GPIO device not ready\n");
		return -ENODEV;
	}

	gpio_pin_configure_dt(&btn_sw1, GPIO_INPUT);
	gpio_pin_configure_dt(&btn_sw2, GPIO_INPUT);

	gpio_pin_interrupt_configure_dt(&btn_sw1, GPIO_INT_EDGE_TO_ACTIVE);
	gpio_pin_interrupt_configure_dt(&btn_sw2, GPIO_INT_EDGE_TO_ACTIVE);

	gpio_init_callback(&btn_cb_data, button_pressed_callback,
			   BIT(btn_sw1.pin) | BIT(btn_sw2.pin));
	ret = gpio_add_callback(btn_sw1.port, &btn_cb_data);
	if (ret < 0) {
		printk("Error: Failed to register button callback (%d)\n", ret);
	}

	return 0;
}

/* Custom Shell Command: tm4c led <red|blue|green|all> <on|off|toggle> */
static int cmd_tm4c_led(const struct shell *sh, size_t argc, char **argv)
{
	if (argc < 3) {
		shell_print(sh, "Usage: tm4c led <red|blue|green|all> <on|off|toggle>");
		return -EINVAL;
	}

	const char *color = argv[1];
	const char *action = argv[2];
	const struct gpio_dt_spec *target_led = NULL;
	bool is_all = false;

	if (strcmp(color, "red") == 0) {
		target_led = &led_red;
	} else if (strcmp(color, "blue") == 0) {
		target_led = &led_blue;
	} else if (strcmp(color, "green") == 0) {
		target_led = &led_green;
	} else if (strcmp(color, "all") == 0) {
		is_all = true;
	} else {
		shell_print(sh, "Unknown color '%s'. Use: red, blue, green, all", color);
		return -EINVAL;
	}

	if (strcmp(action, "on") == 0) {
		if (is_all) {
			gpio_pin_set_dt(&led_red, 1);
			gpio_pin_set_dt(&led_blue, 1);
			gpio_pin_set_dt(&led_green, 1);
		} else {
			gpio_pin_set_dt(target_led, 1);
		}
		shell_print(sh, "LED '%s' set to ON", color);
	} else if (strcmp(action, "off") == 0) {
		if (is_all) {
			gpio_pin_set_dt(&led_red, 0);
			gpio_pin_set_dt(&led_blue, 0);
			gpio_pin_set_dt(&led_green, 0);
		} else {
			gpio_pin_set_dt(target_led, 0);
		}
		shell_print(sh, "LED '%s' set to OFF", color);
	} else if (strcmp(action, "toggle") == 0) {
		if (is_all) {
			gpio_pin_toggle_dt(&led_red);
			gpio_pin_toggle_dt(&led_blue);
			gpio_pin_toggle_dt(&led_green);
		} else {
			gpio_pin_toggle_dt(target_led);
		}
		shell_print(sh, "LED '%s' toggled", color);
	} else {
		shell_print(sh, "Unknown action '%s'. Use: on, off, toggle", action);
		return -EINVAL;
	}

	return 0;
}

/* Custom Shell Command: tm4c button */
static int cmd_tm4c_button(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int sw1_val = gpio_pin_get_dt(&btn_sw1);
	int sw2_val = gpio_pin_get_dt(&btn_sw2);

	shell_print(sh, "=== Pushbutton Telemetry ===");
	shell_print(sh, "  SW1 (PF4) : %s (Presses: %u)",
		    sw1_val > 0 ? "PRESSED" : "RELEASED", sw1_press_count);
	shell_print(sh, "  SW2 (PF0) : %s (Presses: %u)",
		    sw2_val > 0 ? "PRESSED" : "RELEASED", sw2_press_count);
	shell_print(sh, "============================");
	return 0;
}
#endif

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
	shell_print(sh, "  Peripherals  : UART0, RGB LED (PF1-3), SW1/SW2 (PF4,0)");
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
	shell_print(sh, "LED0 (Red)    : Port F Pin 1 (Active High)");
	shell_print(sh, "LED1 (Blue)   : Port F Pin 2 (Active High)");
	shell_print(sh, "LED2 (Green)  : Port F Pin 3 (Active High)");
	shell_print(sh, "SW0 (Btn 1)   : Port F Pin 4 (Pull-Up, Active Low)");
	shell_print(sh, "SW1 (Btn 2)   : Port F Pin 0 (Pull-Up, Active Low)");
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
#if IS_ENABLED(CONFIG_APP_ENABLE_GPIO_DEMO)
	SHELL_CMD_ARG(led, NULL, "Control on-board RGB LED: tm4c led <red|blue|green|all> <on|off|toggle>", cmd_tm4c_led, 3, 0),
	SHELL_CMD(button, NULL, "Read SW1/SW2 pushbutton status & press counts", cmd_tm4c_button),
#endif
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(tm4c, &sub_tm4c, "TM4C123 LaunchPad custom commands", NULL);

int main(void)
{
#if IS_ENABLED(CONFIG_APP_ENABLE_GPIO_DEMO)
	init_gpios();
#endif

#if IS_ENABLED(CONFIG_APP_WELCOME_BANNER)
	printk("\n");
	printk("==============================================\n");
	printk("   TI TM4C123GXL Zephyr Shell Terminal\n");
	printk("   Zephyr OS %s | Board: %s\n", KERNEL_VERSION_STRING, CONFIG_BOARD_TARGET);
	printk("   RGB LEDs & SW1/SW2 Buttons: Initialized\n");
	printk("   Type 'help' or 'tm4c --help' for commands\n");
	printk("==============================================\n\n");
#endif

	/* Main thread can sleep; the shell runs in its own dedicated thread */
	while (1) {
		k_sleep(K_FOREVER);
	}

	return 0;
}
