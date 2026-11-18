/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * TI Tiva C Series GPIO Controller Driver
 */

#define DT_DRV_COMPAT ti_tiva_c_gpio

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/irq.h>

/* TivaWare DriverLib */
#undef ITM_BASE
#undef DWT_BASE
#undef NVIC_BASE
#undef TPIU_BASE
#include <driverlib/sysctl.h>
#include <driverlib/gpio.h>
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <inc/hw_gpio.h>

struct tiva_c_gpio_config {
	struct gpio_driver_config common;
	uint32_t base;
	uint8_t ngpios;
	void (*irq_config_func)(const struct device *dev);
};

struct tiva_c_gpio_data {
	struct gpio_driver_data common;
	sys_slist_t callbacks;
};

static uint32_t tiva_c_gpio_periph(uint32_t base)
{
	switch (base) {
	case GPIO_PORTA_BASE: return SYSCTL_PERIPH_GPIOA;
	case GPIO_PORTB_BASE: return SYSCTL_PERIPH_GPIOB;
	case GPIO_PORTC_BASE: return SYSCTL_PERIPH_GPIOC;
	case GPIO_PORTD_BASE: return SYSCTL_PERIPH_GPIOD;
	case GPIO_PORTE_BASE: return SYSCTL_PERIPH_GPIOE;
	case GPIO_PORTF_BASE: return SYSCTL_PERIPH_GPIOF;
	default: return 0;
	}
}

static inline void tiva_c_gpio_unlock_pin(uint32_t base, uint8_t pin)
{
	if ((base == GPIO_PORTF_BASE && pin == 0) ||
	    (base == GPIO_PORTD_BASE && pin == 7)) {
		HWREG(base + GPIO_O_LOCK) = GPIO_LOCK_KEY;
		HWREG(base + GPIO_O_CR) |= BIT(pin);
		HWREG(base + GPIO_O_LOCK) = 0;
	}
}

static int tiva_c_gpio_pin_configure(const struct device *dev,
				     gpio_pin_t pin,
				     gpio_flags_t flags)
{
	const struct tiva_c_gpio_config *cfg = dev->config;
	uint32_t base = cfg->base;
	uint8_t pin_mask = BIT(pin);

	if (pin >= cfg->ngpios) {
		return -EINVAL;
	}

	tiva_c_gpio_unlock_pin(base, pin);

	if (flags & GPIO_OUTPUT) {
		GPIOPinTypeGPIOOutput(base, pin_mask);
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			GPIOPinWrite(base, pin_mask, pin_mask);
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			GPIOPinWrite(base, pin_mask, 0);
		}
	} else if (flags & GPIO_INPUT) {
		uint32_t pad_type = GPIO_PIN_TYPE_STD;

		if (flags & GPIO_PULL_UP) {
			pad_type = GPIO_PIN_TYPE_STD_WPU;
		} else if (flags & GPIO_PULL_DOWN) {
			pad_type = GPIO_PIN_TYPE_STD_WPD;
		}

		GPIOPinTypeGPIOInput(base, pin_mask);
		GPIOPadConfigSet(base, pin_mask, GPIO_STRENGTH_2MA, pad_type);
	} else if (flags == GPIO_DISCONNECTED) {
		GPIOPinTypeGPIOInput(base, pin_mask);
		HWREG(base + GPIO_O_DEN) &= ~pin_mask;
	}

	return 0;
}

static int tiva_c_gpio_port_get_raw(const struct device *dev, uint32_t *value)
{
	const struct tiva_c_gpio_config *cfg = dev->config;

	*value = GPIOPinRead(cfg->base, 0xFF);
	return 0;
}

static int tiva_c_gpio_port_set_masked_raw(const struct device *dev,
					   uint32_t mask,
					   uint32_t value)
{
	const struct tiva_c_gpio_config *cfg = dev->config;

	GPIOPinWrite(cfg->base, mask, value);
	return 0;
}

static int tiva_c_gpio_port_set_bits_raw(const struct device *dev, uint32_t mask)
{
	const struct tiva_c_gpio_config *cfg = dev->config;

	GPIOPinWrite(cfg->base, mask, mask);
	return 0;
}

static int tiva_c_gpio_port_clear_bits_raw(const struct device *dev, uint32_t mask)
{
	const struct tiva_c_gpio_config *cfg = dev->config;

	GPIOPinWrite(cfg->base, mask, 0);
	return 0;
}

static int tiva_c_gpio_port_toggle_bits(const struct device *dev, uint32_t mask)
{
	const struct tiva_c_gpio_config *cfg = dev->config;
	uint32_t val = GPIOPinRead(cfg->base, mask);

	GPIOPinWrite(cfg->base, mask, val ^ mask);
	return 0;
}

static void tiva_c_gpio_isr(const struct device *dev)
{
	const struct tiva_c_gpio_config *cfg = dev->config;
	struct tiva_c_gpio_data *data = dev->data;
	uint32_t base = cfg->base;
	uint32_t int_status = GPIOIntStatus(base, true);

	GPIOIntClear(base, int_status);
	gpio_fire_callbacks(&data->callbacks, dev, int_status);
}

static int tiva_c_gpio_pin_interrupt_configure(const struct device *dev,
					       gpio_pin_t pin,
					       enum gpio_int_mode mode,
					       enum gpio_int_trig trig)
{
	const struct tiva_c_gpio_config *cfg = dev->config;
	uint32_t base = cfg->base;
	uint8_t pin_mask = BIT(pin);

	if (pin >= cfg->ngpios) {
		return -EINVAL;
	}

	if (mode == GPIO_INT_MODE_DISABLED) {
		GPIOIntDisable(base, pin_mask);
		return 0;
	}

	uint32_t int_type;
	if (mode == GPIO_INT_MODE_EDGE) {
		if (trig == GPIO_INT_TRIG_BOTH) {
			int_type = GPIO_BOTH_EDGES;
		} else if (trig == GPIO_INT_TRIG_HIGH) {
			int_type = GPIO_RISING_EDGE;
		} else {
			int_type = GPIO_FALLING_EDGE;
		}
	} else {
		if (trig == GPIO_INT_TRIG_HIGH) {
			int_type = GPIO_HIGH_LEVEL;
		} else {
			int_type = GPIO_LOW_LEVEL;
		}
	}

	GPIOIntTypeSet(base, pin_mask, int_type);
	GPIOIntClear(base, pin_mask);
	GPIOIntEnable(base, pin_mask);
	return 0;
}

static int tiva_c_gpio_manage_callback(const struct device *dev,
				       struct gpio_callback *callback,
				       bool set)
{
	struct tiva_c_gpio_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static DEVICE_API(gpio, tiva_c_gpio_driver_api) = {
	.pin_configure = tiva_c_gpio_pin_configure,
	.port_get_raw = tiva_c_gpio_port_get_raw,
	.port_set_masked_raw = tiva_c_gpio_port_set_masked_raw,
	.port_set_bits_raw = tiva_c_gpio_port_set_bits_raw,
	.port_clear_bits_raw = tiva_c_gpio_port_clear_bits_raw,
	.port_toggle_bits = tiva_c_gpio_port_toggle_bits,
	.pin_interrupt_configure = tiva_c_gpio_pin_interrupt_configure,
	.manage_callback = tiva_c_gpio_manage_callback,
};

static int tiva_c_gpio_init(const struct device *dev)
{
	const struct tiva_c_gpio_config *cfg = dev->config;
	uint32_t periph = tiva_c_gpio_periph(cfg->base);

	if (periph != 0) {
		SysCtlPeripheralEnable(periph);
		while (!SysCtlPeripheralReady(periph)) {
		}
	}

	if (cfg->irq_config_func) {
		cfg->irq_config_func(dev);
	}

	return 0;
}

#define TIVA_C_GPIO_INIT(n)							\
	static void tiva_c_gpio_irq_config_##n(const struct device *dev);	\
										\
	static const struct tiva_c_gpio_config tiva_c_gpio_cfg_##n = {		\
		.common = {							\
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),	\
		},								\
		.base = DT_INST_REG_ADDR(n),					\
		.ngpios = DT_INST_PROP_OR(n, ngpios, 8),			\
		.irq_config_func = tiva_c_gpio_irq_config_##n,			\
	};									\
										\
	static struct tiva_c_gpio_data tiva_c_gpio_data_##n;			\
										\
	DEVICE_DT_INST_DEFINE(n,						\
			      tiva_c_gpio_init,					\
			      NULL,						\
			      &tiva_c_gpio_data_##n,				\
			      &tiva_c_gpio_cfg_##n,				\
			      POST_KERNEL,					\
			      CONFIG_GPIO_INIT_PRIORITY,			\
			      &tiva_c_gpio_driver_api);				\
										\
	static void tiva_c_gpio_irq_config_##n(const struct device *dev)	\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n),					\
			    DT_INST_IRQ(n, priority),				\
			    tiva_c_gpio_isr,					\
			    DEVICE_DT_INST_GET(n),				\
			    0);							\
		irq_enable(DT_INST_IRQN(n));					\
	}

DT_INST_FOREACH_STATUS_OKAY(TIVA_C_GPIO_INIT)
