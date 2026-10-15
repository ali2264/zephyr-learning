/*
 * 01-blinky: toggle the board's led0 (LD2 on the Nucleo-F401RE)
 * and print the state to the console.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

#define SLEEP_TIME_MS 250

/* led0 is a devicetree alias defined by the board's .dts file */
#define LED0_NODE DT_ALIAS(led0)
/* led1 is a devicetree alias defined by this app's overlay */
#define LED1_NODE DT_ALIAS(led1)

#if !DT_NODE_EXISTS(LED1_NODE)
#error "led1 alias not found: check boards/nucleo_f401re.overlay"
#endif

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec ext_led = GPIO_DT_SPEC_GET(LED1_NODE, gpios);


int main(void)
{
	int ret;
	bool led_on = true;

	if (!gpio_is_ready_dt(&led) || !gpio_is_ready_dt(&ext_led)) {
		printk("LED device not ready\n");
		return 0;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printk("Failed to configure LED: %d\n", ret);
		return 0;
	}

	/* start opposite to led0 so the two alternate */
	ret = gpio_pin_configure_dt(&ext_led, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		printk("Failed to configure external LED: %d\n", ret);
		return 0;
	}

	while (1) {
		ret = gpio_pin_toggle_dt(&led);
		if (ret < 0) {
			printk("Failed to toggle LED: %d\n", ret);
			return 0;
		}

		ret = gpio_pin_toggle_dt(&ext_led);
		if (ret < 0) {
			printk("Failed to toggle external LED: %d\n", ret);
			return 0;
		}

		led_on = !led_on;
		printk("LD2 %s, external %s\n", led_on ? "ON" : "OFF", led_on ? "OFF" : "ON");
		k_msleep(SLEEP_TIME_MS);
	}

	return 0;
}
