/* linux/arch/arm/mach-s5pv210/mach-aries.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/serial_core.h>
#include <linux/gpio.h>
#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/i2c/qt602240_ts.h>
#include <linux/regulator/max8998.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/usb/ch9.h>
#include <linux/pwm_backlight.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <linux/reboot.h>

#ifdef CONFIG_KERNEL_DEBUG_SEC
#include <linux/kernel_sec_common.h>
#endif

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/setup.h>
#include <asm/mach-types.h>

#include <mach/map.h>
#include <mach/regs-clock.h>
#include <mach/regs-mem.h>
#include <mach/regs-gpio.h>
#include <mach/gpio-bank.h>
#include <mach/adcts.h>
#include <mach/ts.h>
#include <mach/adc.h>

#ifdef CONFIG_VIDEO_CE147
#include <media/ce147_platform.h>
#endif // CONFIG_VIDEO_CE147

#ifdef CONFIG_VIDEO_S5KA3DFX
#include <media/s5ka3dfx_platform.h>
#endif // CONFIG_VIDEO_S5KA3DFX

#ifdef CONFIG_VIDEO_S5K5CCGX
#include <media/common_camera_platform.h>
#endif // CONFIG_VIDEO_S5K5CCGX

#include <plat/regs-serial.h>
#include <plat/s5pv210.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <plat/fb.h>
#include <plat/gpio-cfg.h>
#include <plat/iic.h>

#include <plat/fimc.h>
#include <plat/regs-fimc.h>
#include <plat/csis.h>
#include <plat/mfc.h>
#include <plat/sdhci.h>
#include <plat/ide.h>
#include <plat/regs-otg.h>
#include <plat/clock.h>
#include <plat/gpio-core.h>

#include <mach/gpio-aries.h>
#include <mach/gpio-aries-setting.h>
#ifdef CONFIG_ANDROID_PMEM
#include <linux/android_pmem.h>
#include <plat/media.h>
#endif

#if defined(CONFIG_PM)
#include <plat/pm.h>
#endif
#include <mach/max8998_function.h>
#include <mach/sec_jack.h>
#include <mach/param.h>

#if defined (CONFIG_SAMSUNG_PHONE_SVNET) || defined (CONFIG_SAMSUNG_PHONE_SVNET_MODULE)
#include <linux/modemctl.h>
#include <linux/onedram.h>
#include <linux/irq.h>
#endif

#define RECOVERY_SYSFS

struct class *sec_class;
EXPORT_SYMBOL(sec_class);

struct device *switch_dev;
EXPORT_SYMBOL(switch_dev);

struct device *gps_dev = NULL;
EXPORT_SYMBOL(gps_dev);

void (*sec_set_param_value)(int idx, void *value);
EXPORT_SYMBOL(sec_set_param_value);

void (*sec_get_param_value)(int idx, void *value);
EXPORT_SYMBOL(sec_get_param_value);

//recovery interface, Thomas Ryu
#ifdef RECOVERY_SYSFS
static int __init recovery_class_init(void);
#endif

static void jupiter_switch_init(void)
{
	sec_class = class_create(THIS_MODULE, "sec");

	if (IS_ERR(sec_class))
		pr_err("Failed to create class(sec)!\n");

	switch_dev = device_create(sec_class, NULL, 0, NULL, "switch");

	if (IS_ERR(switch_dev))
		pr_err("Failed to create device(switch)!\n");

};

/* Following are default values for UCON, ULCON and UFCON UART registers */
#define S5PV210_UCON_DEFAULT	(S3C2410_UCON_TXILEVEL |	\
				 S3C2410_UCON_RXILEVEL |	\
				 S3C2410_UCON_TXIRQMODE |	\
				 S3C2410_UCON_RXIRQMODE |	\
				 S3C2410_UCON_RXFIFO_TOI |	\
				 S3C2443_UCON_RXERR_IRQEN)

#define S5PV210_ULCON_DEFAULT	S3C2410_LCON_CS8

#define S5PV210_UFCON_DEFAULT	(S3C2410_UFCON_FIFOMODE |	\
				 S5PV210_UFCON_TXTRIG4 |	\
				 S5PV210_UFCON_RXTRIG4)

extern void s5pv210_reserve_bootmem(void);
extern void s3c_sdhci_set_platdata(void);

static struct s3c2410_uartcfg smdkv210_uartcfgs[] __initdata = {
	[0] = {
		.hwport		= 0,
		.flags		= 0,
		.ucon		= S5PV210_UCON_DEFAULT,
		.ulcon		= S5PV210_ULCON_DEFAULT,
		.ufcon		= S5PV210_UFCON_DEFAULT,
	},
	[1] = {
		.hwport		= 1,
		.flags		= 0,
		.ucon		= S5PV210_UCON_DEFAULT,
		.ulcon		= S5PV210_ULCON_DEFAULT,
		.ufcon		= S5PV210_UFCON_DEFAULT,
	},
	[2] = {
		.hwport		= 2,
		.flags		= 0,
		.ucon		= S5PV210_UCON_DEFAULT,
		.ulcon		= S5PV210_ULCON_DEFAULT,
		.ufcon		= S5PV210_UFCON_DEFAULT,
	},
	[3] = {
		.hwport		= 3,
		.flags		= 0,
		.ucon		= S5PV210_UCON_DEFAULT,
		.ulcon		= S5PV210_ULCON_DEFAULT,
		.ufcon		= S5PV210_UFCON_DEFAULT,
	},
};


#if defined(CONFIG_TOUCHSCREEN_QT602240)
static struct platform_device s3c_device_qtts = {
        .name = "qt602240-ts",
        .id = -1,
};
#endif


/* PMIC */
static struct regulator_consumer_supply dcdc1_consumers[] = {
        {
                .supply         = "vddarm",
        },
};

static struct regulator_init_data max8998_dcdc1_data = {
        .constraints    = {
                .name           = "VCC_ARM",
                .min_uV         =  750000,
                .max_uV         = 1500000,
                .always_on      = 1,
                .valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
        },
        .num_consumer_supplies  = ARRAY_SIZE(dcdc1_consumers),
        .consumer_supplies      = dcdc1_consumers,
};

static struct regulator_consumer_supply dcdc2_consumers[] = {
        {
                .supply         = "vddint",
        },
};

static struct regulator_init_data max8998_dcdc2_data = {
        .constraints    = {
                .name           = "VCC_INTERNAL",
                .min_uV         =  750000,
                .max_uV         = 1500000,
                .always_on      = 1,
//              .apply_uV       = 1,
                .valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
        },
        .num_consumer_supplies  = ARRAY_SIZE(dcdc2_consumers),
        .consumer_supplies      = dcdc2_consumers,
};
static struct regulator_init_data max8998_ldo4_data = {
        .constraints    = {
                .name           = "VCC_DAC",
                .min_uV         = 3300000,
                .max_uV         = 3300000,
                .always_on      = 1,
                .apply_uV       = 1,
                .valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
        },
};

static struct regulator_init_data max8998_ldo5_data = {
        .constraints    = {
        .name           = "VCC_SDHC",
        .min_uV         = 2800000,
        .max_uV         = 2800000,
        .always_on	    = 0,
        .apply_uV       = 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE,
        },
};

static struct regulator_init_data max8998_ldo6_data = {
        .constraints    = {
        .name           = "VBT_WL_2.6V",
        .min_uV         = 2800000,
        .max_uV         = 2800000,
		.always_on	    = 0,
        .apply_uV       = 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE,
        },
};


static struct regulator_init_data max8998_ldo7_data = {
        .constraints    = {
                .name           = "VCC_LCD",
                .min_uV         = 1800000,
                .max_uV         = 1800000,
                .always_on      = 1,
                //.apply_uV     = 1,
                .valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
        },
};

static struct regulator_init_data max8998_ldo17_data = {
        .constraints    = {
                .name           = "PM_LVDS_VDD",
                .min_uV         = 1600000,
                .max_uV         = 3600000,
                .always_on      = 1,
                //.apply_uV     = 1,
                .valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
        },
};

static struct regulator_init_data max8998_ldo12_data = {
        .constraints    = {
                .name           = "V_MOTOR_3.0V",
                .min_uV         = 3200000,  //3000000
                .max_uV         = 3200000,  //3000000
		        .always_on	    = 0,
                .apply_uV       = 1,
		        .valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE,
        },
};

static struct regulator_init_data max8998_ldo16_data = {
        .constraints    = {
        .name           = "MIPI_1.8V",
        .min_uV         = 1800000,
        .max_uV         = 1800000,
        .always_on	= 1,
        .apply_uV       = 1,
        .valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE,
        },
};

static struct max8998_subdev_data universal_regulators[] = {
	{ MAX8998_DCDC1, &max8998_dcdc1_data },
	{ MAX8998_DCDC2, &max8998_dcdc2_data },
//	{ MAX8998_DCDC4, &max8998_dcdc4_data },
	{ MAX8998_LDO4, &max8998_ldo4_data },       /* VADC_3.3     */
	{ MAX8998_LDO5, &max8998_ldo5_data },       /* FORTE SHDC   */
//	{ MAX8998_LDO11, &max8998_ldo11_data },     /* CAM_AF_2.8V  */
	{ MAX8998_LDO12, &max8998_ldo12_data },     /* MOTOR        */
//	{ MAX8998_LDO13, &max8998_ldo13_data },     /* CAM_A_2.8V   */
//	{ MAX8998_LDO14, &max8998_ldo14_data },     /* CAM_IO_2.8V  */
//	{ MAX8998_LDO15, &max8998_ldo15_data },     /* CAM_1.2V     */
	{ MAX8998_LDO7, &max8998_ldo7_data },       /* VLCD_1.8V    */
	{ MAX8998_LDO17, &max8998_ldo17_data },     /* VCC_3.0V_LCD */
	{ MAX8998_LDO16, &max8998_ldo16_data },
};

static struct max8998_platform_data max8998_platform_data = {
        .num_regulators = ARRAY_SIZE(universal_regulators),
        .regulators     = universal_regulators,
};

#if 0
/* I2C2 */
static struct i2c_board_info i2c_devs2[] __initdata = {
        {
                /* The address is 0xCC used since SRAD = 0 */
                I2C_BOARD_INFO("max8998", (0xCC >> 1)),
                .platform_data = &max8998_platform_data,
        },
};
#endif

#if defined (CONFIG_SAMSUNG_PHONE_TTY) || defined (CONFIG_SAMSUNG_PHONE_TTY_MODULE)
struct platform_device sec_device_dpram = {
        .name = "dpram-device",
        .id   = -1,
};
#endif

struct platform_device s3c_device_8998consumer = {
        .name = "max8998-consumer",
        .id   = 0,
        .dev  = { .platform_data = &max8998_platform_data },
};

#ifdef CONFIG_FB_S3C_S6D16A0X
#define DISPLAY_CS      GPIO_DISPLAY_CS
#define DISPLAY_CLK     GPIO_DISPLAY_CLK
#define DISPLAY_SI      GPIO_DISPLAY_SI
#define DISPLAY_SO      GPIO_DUMMY


static void s6d16a0x_cfg_gpio(struct platform_device *pdev)
{
    int i;

    printk(KERN_ERR "s6d16a0x_cfg_gpio");

    for (i = 0; i < 8; i++) {
        s3c_gpio_cfgpin(S5PV210_GPF0(i), S3C_GPIO_SFN(2));
        s3c_gpio_setpull(S5PV210_GPF0(i), S3C_GPIO_PULL_NONE);
    }

    for (i = 0; i < 8; i++) {
        s3c_gpio_cfgpin(S5PV210_GPF1(i), S3C_GPIO_SFN(2));
        s3c_gpio_setpull(S5PV210_GPF1(i), S3C_GPIO_PULL_NONE);
    }

    for (i = 0; i < 8; i++) {
        s3c_gpio_cfgpin(S5PV210_GPF2(i), S3C_GPIO_SFN(2));
        s3c_gpio_setpull(S5PV210_GPF2(i), S3C_GPIO_PULL_NONE);
    }

    for (i = 0; i < 4; i++) {
        s3c_gpio_cfgpin(S5PV210_GPF3(i), S3C_GPIO_SFN(2));
        s3c_gpio_setpull(S5PV210_GPF3(i), S3C_GPIO_PULL_NONE);
    }

    /* mDNIe SEL: why we shall write 0x2 ? */
    #ifndef CONFIG_FB_S3C_MDNIE
    writel(0x2, S5P_MDNIE_SEL);
    #endif

    #if 0   // TODO:FORTE_FROYO
    /* drive strength to max */
    writel(0xffffffff, S5P_VA_GPIO + 0x12c);
    writel(0xffffffff, S5P_VA_GPIO + 0x14c);
    writel(0xffffffff, S5P_VA_GPIO + 0x16c);
    writel(0x000000ff, S5P_VA_GPIO + 0x18c);
    #endif

    /* DISPLAY_CS */
    s3c_gpio_cfgpin(DISPLAY_CS, S3C_GPIO_OUTPUT);
    /* DISPLAY_CLK */
    s3c_gpio_cfgpin(DISPLAY_CLK, S3C_GPIO_OUTPUT);
    /* DISPLAY_SI */
    s3c_gpio_cfgpin(DISPLAY_SI, S3C_GPIO_OUTPUT);
    /* LCD_SO */
    //s3c_gpio_cfgpin(DISPLAY_SO, S3C_GPIO_INPUT);

    /* DISPLAY_CS */
    s3c_gpio_setpull(DISPLAY_CS, S3C_GPIO_PULL_NONE);
    /* DISPLAY_CLK */
    s3c_gpio_setpull(DISPLAY_CLK, S3C_GPIO_PULL_NONE);
    /* DISPLAY_SI */
    s3c_gpio_setpull(DISPLAY_SI, S3C_GPIO_PULL_NONE);
    /* LCD_SO */
    //s3c_gpio_setpull(DISPLAY_SO, S3C_GPIO_PULL_UP);
}

static int s6d16a0x_backlight_on(struct platform_device *pdev)
{
    printk(KERN_ERR "s6d16a0x_backlight_on");
    return 0;
}

static int s6d16a0x_reset_lcd(struct platform_device *pdev)
{
    printk(KERN_ERR "s6d16a0x_reset_lcd");
    s3c_gpio_cfgpin(GPIO_MLCD_RST, S3C_GPIO_SFN(1));

    gpio_set_value(GPIO_MLCD_RST, 0);
    msleep(1);
    gpio_set_value(GPIO_MLCD_RST, 1);
    msleep(10);

    return 0;
}


static struct s3c_platform_fb s6d16a0x_data __initdata = {
    .hw_ver         = 0x62,     // FORTE_FROYO
    .clk_name       = "sclk_fimd",
    .nr_wins        = 5,
    .default_win    = CONFIG_FB_S3C_DEFAULT_WINDOW,
    .swap           = FB_SWAP_HWORD | FB_SWAP_WORD,

    .cfg_gpio       = s6d16a0x_cfg_gpio,
    .backlight_on   = s6d16a0x_backlight_on,
    .reset_lcd      = s6d16a0x_reset_lcd,
};

#define LCD_BUS_NUM     3

static struct spi_board_info spi_board_info[] __initdata = {
    {
        .modalias           = "s6d16a0x",
        .platform_data      = NULL,
        .max_speed_hz       = 1200000,
        .bus_num            = LCD_BUS_NUM,
        .chip_select        = 0,
        .mode               = SPI_MODE_3,
        .controller_data    = (void *)DISPLAY_CS,
    },
};

static struct spi_gpio_platform_data s6d16a0x_spi_gpio_data = {
    .sck            = DISPLAY_CLK,
    .mosi           = DISPLAY_SI,
    .miso           = -1, //DISPLAY_SO
    .num_chipselect = 1,    // 2
};

static struct platform_device s3c_device_spi_gpio = {
    .name   = "spi_gpio",
    .id     = LCD_BUS_NUM,
    .dev    = {
        .parent         = &s3c_device_fb.dev,
        .platform_data  = &s6d16a0x_spi_gpio_data,
    },
};

#elif   defined(CONFIG_FB_S3C_TL2796)
static void tl2796_cfg_gpio(struct platform_device *pdev)
{
	int i;

	/* DISPLAY_HSYNC */
	s3c_gpio_cfgpin(GPIO_DISPLAY_HSYNC, GPIO_DISPLAY_HSYNC_AF);
	s3c_gpio_setpull(GPIO_DISPLAY_HSYNC, S3C_GPIO_PULL_NONE);

	/* DISPLAY_VSYNC */
	s3c_gpio_cfgpin(GPIO_DISPLAY_VSYNC, GPIO_DISPLAY_VSYNC_AF);
	s3c_gpio_setpull(GPIO_DISPLAY_VSYNC, S3C_GPIO_PULL_NONE);

	/* DISPLAY_DE */
	s3c_gpio_cfgpin(GPIO_DISPLAY_DE, GPIO_DISPLAY_DE_AF);
	s3c_gpio_setpull(GPIO_DISPLAY_DE, S3C_GPIO_PULL_NONE);

	/* DISPLAY_PCLK */
	s3c_gpio_cfgpin(GPIO_DISPLAY_PCLK, GPIO_DISPLAY_PCLK_AF);
	s3c_gpio_setpull(GPIO_DISPLAY_PCLK, S3C_GPIO_PULL_NONE);

	/*
		WARNING:
			This code works on situation that LCD data pin is
			set serially by hardware
	 */
	for (i = 0; i < 24; i++)	{
		s3c_gpio_cfgpin(GPIO_LCD_D0 + i, GPIO_LCD_D0_AF);
		s3c_gpio_setpull(GPIO_LCD_D0 + i, S3C_GPIO_PULL_NONE);
	}

	/* mDNIe SEL: why we shall write 0x2 ? */
#ifdef CONFIG_FB_S3C_MDNIE
	writel(0x1, S5P_MDNIE_SEL);
#else
	writel(0x2, S5P_MDNIE_SEL);
#endif
#if 0
	/* drive strength to max */
	writel(0xffffffff, S5PC_VA_GPIO + 0x12c);
	writel(0xffffffff, S5PC_VA_GPIO + 0x14c);
	writel(0xffffffff, S5PC_VA_GPIO + 0x16c);
	writel(0x000000ff, S5PC_VA_GPIO + 0x18c);
#endif

	/* DISPLAY_CS */
	s3c_gpio_cfgpin(GPIO_DISPLAY_CS, GPIO_DISPLAY_CS_AF);
	s3c_gpio_setpull(GPIO_DISPLAY_CS, S3C_GPIO_PULL_NONE);
	/* DISPLAY_CLK */
	s3c_gpio_cfgpin(GPIO_DISPLAY_CLK, GPIO_DISPLAY_CLK_AF);
	s3c_gpio_setpull(GPIO_DISPLAY_CLK, S3C_GPIO_PULL_NONE);
	/* DISPLAY_SI */
	s3c_gpio_cfgpin(GPIO_DISPLAY_SI, GPIO_DISPLAY_SI_AF);
	s3c_gpio_setpull(GPIO_DISPLAY_SI, S3C_GPIO_PULL_NONE);

	/*KGVS : configuring GPJ2(4) as FM interrupt */
	//s3c_gpio_cfgpin(S5PV210_GPJ2(4), S5PV210_GPJ2_4_GPIO_INT20_4);

}

void lcd_cfg_gpio_early_suspend(void)
{
	int i;
	printk("[%s]\n", __func__);

	/* DISPLAY_HSYNC */
	s3c_gpio_cfgpin(GPIO_DISPLAY_HSYNC, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_DISPLAY_HSYNC, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(GPIO_DISPLAY_HSYNC, 0);

	/* DISPLAY_VSYNC */
	s3c_gpio_cfgpin(GPIO_DISPLAY_VSYNC, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_DISPLAY_VSYNC, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(GPIO_DISPLAY_VSYNC, 0);

	/* DISPLAY_DE */
	s3c_gpio_cfgpin(GPIO_DISPLAY_DE, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_DISPLAY_DE, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(GPIO_DISPLAY_DE, 0);

	/* DISPLAY_PCLK */
	s3c_gpio_cfgpin(GPIO_DISPLAY_PCLK, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_DISPLAY_PCLK, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(GPIO_DISPLAY_PCLK, 0);

	/*
		WARNING:
			This code works on situation that LCD data pin is
			set serially by hardware
	 */
	for (i = 0; i < 24; i++)	{
		s3c_gpio_cfgpin(GPIO_LCD_D0 + i, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_LCD_D0 + i, S3C_GPIO_PULL_NONE);
		s3c_gpio_setpin(GPIO_LCD_D0 + i, 0);
	}
	// drive strength to min
	writel(0x00000000, S5P_VA_GPIO + 0x12c);		// GPF0DRV
	writel(0x00000000, S5P_VA_GPIO + 0x14c);		// GPF1DRV
	writel(0x00000000, S5P_VA_GPIO + 0x16c);		// GPF2DRV
	writel(0x00000000, S5P_VA_GPIO + 0x18c);		// GPF3DRV

	/* DISPLAY_CS */
	s3c_gpio_cfgpin(GPIO_DISPLAY_CS, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_DISPLAY_CS, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(GPIO_DISPLAY_CS, 0);
	/* DISPLAY_CLK */
	s3c_gpio_cfgpin(GPIO_DISPLAY_CLK, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_DISPLAY_CLK, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(GPIO_DISPLAY_CLK, 0);
	/* DISPLAY_SI */
	s3c_gpio_cfgpin(GPIO_DISPLAY_SI, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_DISPLAY_SI, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(GPIO_DISPLAY_SI, 0);
}
EXPORT_SYMBOL(lcd_cfg_gpio_early_suspend);

void lcd_cfg_gpio_late_resume(void)
{
	printk("[%s]\n", __func__);

}
EXPORT_SYMBOL(lcd_cfg_gpio_late_resume);

static int tl2796_reset_lcd(struct platform_device *pdev)
{
	int err;

	//  Ver1 & Ver2 universal board kyoungheon
	err = gpio_request(GPIO_MLCD_RST, "MLCD_RST");
	if (err) {
		printk(KERN_ERR "failed to request GPIO_MLCD_RST for "
				"lcd reset control\n");
		return err;
	}

	gpio_direction_output(GPIO_MLCD_RST, 1);
	msleep(10);

	gpio_set_value(GPIO_MLCD_RST, 0);
	msleep(10);

	gpio_set_value(GPIO_MLCD_RST, 1);
	msleep(10);

	gpio_free(GPIO_MLCD_RST);

	return 0;
}

static int tl2796_backlight_on(struct platform_device *pdev)
{

}

static struct s3c_platform_fb tl2796_data __initdata = {
        .hw_ver = 0x62,
        .clk_name = "sclk_fimd",
        .nr_wins = 5,
        .default_win = CONFIG_FB_S3C_DEFAULT_WINDOW,
        .swap = FB_SWAP_HWORD | FB_SWAP_WORD,

        .cfg_gpio = tl2796_cfg_gpio,
        .backlight_on = tl2796_backlight_on,
        .reset_lcd = tl2796_reset_lcd,
};

#define LCD_BUS_NUM     3
#define DISPLAY_CS      S5PV210_MP01(1)
#define SUB_DISPLAY_CS  S5PV210_MP01(2)
#define DISPLAY_CLK     S5PV210_MP04(1)
#define DISPLAY_SI      S5PV210_MP04(3)


static struct spi_board_info spi_board_info[] __initdata = {
        {
                .modalias       = "tl2796",
                .platform_data  = NULL,
                .max_speed_hz   = 1200000,
                .bus_num        = LCD_BUS_NUM,
                .chip_select    = 0,
                .mode           = SPI_MODE_3,
                .controller_data = (void *)DISPLAY_CS,
        },
};

static struct spi_gpio_platform_data tl2796_spi_gpio_data = {
        .sck    = DISPLAY_CLK,
        .mosi   = DISPLAY_SI,
        .miso   = 0,
        .num_chipselect = 2,
};

static struct platform_device s3c_device_spi_gpio = {
        .name   = "spi_gpio",
        .id     = LCD_BUS_NUM,
        .dev    = {
                .parent         = &s3c_device_fb.dev,
                .platform_data  = &tl2796_spi_gpio_data,
        },
};
#endif  /* S6D16A0X or TL2796 */



static  struct  i2c_gpio_platform_data  i2c4_platdata = {
        .sda_pin                = GPIO_AP_SDA_18V,
        .scl_pin                = GPIO_AP_SCL_18V,
        .udelay                 = 2,    /* 250KHz */
        .sda_is_open_drain      = 0,
        .scl_is_open_drain      = 0,
        .scl_is_output_only     = 0,
//      .scl_is_output_only     = 1,
      };

static struct platform_device s3c_device_i2c4 = {
        .name                           = "i2c-gpio",
        .id                                     = 4,
        .dev.platform_data      = &i2c4_platdata,
};

static  struct  i2c_gpio_platform_data  i2c5_platdata = {
        .sda_pin                = GPIO_AP_SDA_28V,
        .scl_pin                = GPIO_AP_SCL_28V,
        .udelay                 = 2,    /* 250KHz */
//      .udelay                 = 4,
        .sda_is_open_drain      = 0,
        .scl_is_open_drain      = 0,
        .scl_is_output_only     = 0,
//      .scl_is_output_only     = 1,
};

static struct platform_device s3c_device_i2c5 = {
        .name                           = "i2c-gpio",
        .id                                     = 5,
        .dev.platform_data      = &i2c5_platdata,
};


static  struct  i2c_gpio_platform_data  i2c6_platdata = {
        .sda_pin                = GPIO_AP_PMIC_SDA,
        .scl_pin                = GPIO_AP_PMIC_SCL,
        .udelay                 = 2,    /* 250KHz */
        .sda_is_open_drain      = 0,
        .scl_is_open_drain      = 0,
        .scl_is_output_only     = 0,
};

static struct platform_device s3c_device_i2c6 = {
        .name                   = "i2c-gpio",
        .id                     = 6,
        .dev.platform_data      = &i2c6_platdata,
};

#if 0 // TODO:FORTE
static  struct  i2c_gpio_platform_data  i2c7_platdata = {
        .sda_pin                = GPIO_USB_SDA_28V,
        .scl_pin                = GPIO_USB_SCL_28V,
        .udelay                 = 2,    /* 250KHz */
        .sda_is_open_drain      = 0,
        .scl_is_open_drain      = 0,
        .scl_is_output_only     = 0,
};

static struct platform_device s3c_device_i2c7 = {
        .name                           = "i2c-gpio",
        .id                                     = 7,
        .dev.platform_data      = &i2c7_platdata,
};
// For GRIP 7/31/2010 Added for Froyo Atlas
static	struct	i2c_gpio_platform_data	i2c8_platdata = {
	.sda_pin		= GPIO_GRIP_SDA_28V,
	.scl_pin		= GPIO_GRIP_SCL_28V,
	.udelay			= 2,	// 250KHz
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device s3c_device_i2c8 = {
        .name                           = "i2c-gpio",
        .id                                     = 8,
        .dev.platform_data      = &i2c8_platdata,
};
#endif

static  struct  i2c_gpio_platform_data  i2c9_platdata = {
        .sda_pin                = FUEL_SDA_18V,
        .scl_pin                = FUEL_SCL_18V,
        .udelay                 = 2,    /* 250KHz */
        .sda_is_open_drain      = 0,
        .scl_is_open_drain      = 0,
        .scl_is_output_only     = 0,
};

static struct platform_device s3c_device_i2c9 = {
        .name                   = "i2c-gpio",
        .id                     = 9,
        .dev.platform_data      = &i2c9_platdata,
};

#if 0   // TODO:FORTE
static  struct  i2c_gpio_platform_data  i2c10_platdata = {
        .sda_pin                = _3_TOUCH_SDA_28V,
        .scl_pin                = _3_TOUCH_SCL_28V,
        .udelay                 = 0,    /* 250KHz */
        .sda_is_open_drain      = 0,
        .scl_is_open_drain      = 0,
        .scl_is_output_only     = 0,
};
static struct platform_device s3c_device_i2c10 = {
        .name                   = "i2c-gpio",
        .id                     = 10,
        .dev.platform_data      = &i2c10_platdata,
};
#endif

static  struct  i2c_gpio_platform_data  i2c11_platdata = {
        .sda_pin                = GPIO_ALS_SDA_28V,
        .scl_pin                = GPIO_ALS_SCL_28V,
        .udelay                 = 2,    /* 250KHz */
        .sda_is_open_drain      = 0,
        .scl_is_open_drain      = 0,
        .scl_is_output_only     = 0,
};

static struct platform_device s3c_device_i2c11 = {
        .name                   = "i2c-gpio",
        .id                     = 11,
        .dev.platform_data      = &i2c11_platdata,
};

static  struct  i2c_gpio_platform_data  i2c12_platdata = {
        .sda_pin                = GPIO_BL_SDA_28V,
        .scl_pin                = GPIO_BL_SCL_28V,
        .udelay                 = 0,    /* 250KHz */
        .sda_is_open_drain      = 0,
        .scl_is_open_drain      = 0,
        .scl_is_output_only     = 0,
};

static struct platform_device s3c_device_i2c12 = {
        .name                   = "i2c-gpio",
        .id                     = 12,
        .dev.platform_data      = &i2c12_platdata,
};


static struct sec_jack_port sec_jack_port[] = {
	{
		{ // HEADSET detect info
			.eint       =IRQ_EINT6,
			.gpio       = GPIO_DET_35,
			.gpio_af    = GPIO_DET_35_AF  ,
			.low_active     = 1
		},
		{ // SEND/END info
			.eint       = IRQ_EINT13,
			.gpio       = GPIO_EAR_SEND_END,
			.gpio_af    = GPIO_EAR_SEND_END_AF,
			.low_active = 1
		},
	    { // Open_SEND_END info  added for Froyo Atlas
			.eint		= IRQ_EINT2,
			.gpio		= GPIO_EAR_SEND_END_OPEN,
			.gpio_af	= GPIO_EAR_SEND_END_OPEN_AF,
			.low_active	= 0
		}
	}
};

static struct sec_jack_platform_data sec_jack_data = {
	.port           = sec_jack_port,
	.nheadsets      = ARRAY_SIZE(sec_jack_port),
};

static struct platform_device sec_device_jack = {
	.name           = "sec_jack",
	.id             = -1,
	.dev            = {
		.platform_data  = &sec_jack_data,
	},
};



#ifdef CONFIG_S5PV210_ADCTS
static struct s3c_adcts_plat_info s3c_adcts_cfgs __initdata = {
	.channel = {
		{ /* 0 */
			.delay = 0xFF,
			.presc = 49,
			.resol = S3C_ADCCON_RESSEL_12BIT,
		},{ /* 1 */
			.delay = 0xFF,
			.presc = 49,
			.resol = S3C_ADCCON_RESSEL_12BIT,
		},{ /* 2 */
			.delay = 0xFF,
			.presc = 49,
			.resol = S3C_ADCCON_RESSEL_12BIT,
		},{ /* 3 */
			.delay = 0xFF,
			.presc = 49,
			.resol = S3C_ADCCON_RESSEL_12BIT,
		},{ /* 4 */
			.delay = 0xFF,
			.presc = 49,
			.resol = S3C_ADCCON_RESSEL_12BIT,
		},{ /* 5 */
			.delay = 0xFF,
			.presc = 49,
			.resol = S3C_ADCCON_RESSEL_12BIT,
		},{ /* 6 */
			.delay = 0xFF,
			.presc = 49,
			.resol = S3C_ADCCON_RESSEL_12BIT,
		},{ /* 7 */
			.delay = 0xFF,
			.presc = 49,
			.resol = S3C_ADCCON_RESSEL_12BIT,
		},
	},
};
#endif

#ifdef CONFIG_TOUCHSCREEN_S3C
static struct s3c_ts_mach_info s3c_ts_platform __initdata = {
	.adcts = {
		.delay = 0xFF,
	.presc                  = 49,
		.resol = S3C_ADCCON_RESSEL_12BIT,
	},
	.sampling_time = 18,
	.sampling_interval_ms = 20,
	.x_coor_min	= 180,
	.x_coor_max = 4000,
	.x_coor_fuzz = 32,
	.y_coor_min = 300,
	.y_coor_max = 3900,
	.y_coor_fuzz = 32,
	.use_tscal = false,
	.tscal = {0, 0, 0, 0, 0, 0, 0},
};
#endif

#if 0
#ifndef CONFIG_S5PV210_ADCTS
static struct s3c_adc_mach_info s3c_adc_platform __initdata = {
	/* s5pc110 support 12-bit resolution */
	.delay  = 10000,
	.presc  = 49,
	.resolution = 12,
};
#endif
#endif
#ifdef CONFIG_S5P_ADC
static struct s3c_adc_mach_info s3c_adc_platform __initdata = {
	/* s5pc110 support 12-bit resolution */
	.delay  = 10000,
	.presc  = 65,
	.resolution = 12,
};
#endif

#ifdef CONFIG_VIDEO_FIMC
/*
 * Guide for Camera Configuration for Aries
*/

 // cmk 2010.10.15 setup
 #ifdef CONFIG_VIDEO_S5K5CCGX
/****************************************************************************
 * s5k5ccgx_ldo_en()
 *
 * @param     en             LDO enable
 * @return    void
 * @remark    Function     Set_MAX8998_PM_OUTPUT_Voltage, Set_MAX8998_PM_REG, 
 *
 * Oct 15, 2010  initial revision
 *****************************************************************************/
static void s5k5ccgx_ldo_en(bool en)
{
    if(en)
    {
        /* VDDA : CAM_A_2.8V */		
        Set_MAX8998_PM_OUTPUT_Voltage(LDO13, VCC_2p800);
        Set_MAX8998_PM_REG(ELDO13, 1);
        udelay(5);

        /* VDD_REG : CAM_3M_1.2V */
        Set_MAX8998_PM_OUTPUT_Voltage(LDO15, VCC_1p200); 
        Set_MAX8998_PM_REG(ELDO15, 1);
        udelay(5);

        /* VDDIO : CAM_IO_2.8V */
        Set_MAX8998_PM_OUTPUT_Voltage(LDO14, VCC_2p800);
        Set_MAX8998_PM_REG(ELDO14, 1);
        udelay(5);

        /* VAF : CAM_AF_2.8V or 3.0V */	
        Set_MAX8998_PM_OUTPUT_Voltage(LDO11, VCC_3p000);
        Set_MAX8998_PM_REG(ELDO11, 1);
    } 
    else 
    {
        /* VAF : CAM_AF_2.8V or 3.0V */	
        Set_MAX8998_PM_REG(ELDO11, 0);
        udelay(10);

        /* VDDIO : CAM_IO_2.8V */
        Set_MAX8998_PM_REG(ELDO14, 0);
        udelay(5);
        
        /* VDD_REG : CAM_3M_1.2V */
        Set_MAX8998_PM_REG(ELDO15, 0);
        udelay(5);
        
        /* VDDA : CAM_A_2.8V */	
        Set_MAX8998_PM_REG(ELDO13, 0);
    }
}

/****************************************************************************
 * s5k5ccgx_power_on_sequence()
 *
 * @param     void 
 * @return    0
 * @remark    Function
 *
 * Oct 15, 2010  initial revision
 *****************************************************************************/
static int s5k5ccgx_power_on_sequence(void)
{
	int err;
	
	printk("camera_power_on\n");
	
	/* CAM_MEGA_EN - GPJ0(6) => GPA0(6)*/
	err = gpio_request(GPIO_CAM_MEGA_EN, "GPA0");
	if(err)
	{
		printk(KERN_ERR "failed to request GPIO for camera standby\n");
		return err;
	}

	/* CAM_MEGA_nRST - GPJ1(5) => GPE1(4) */
	err = gpio_request(GPIO_CAM_MEGA_nRST, "GPE1");
	if(err) 
	{
		printk(KERN_ERR "failed to request GPE1(4) for camera control\n");
		return err;
	}

	gpio_direction_output(GPIO_CAM_MEGA_EN, 0);
	gpio_direction_output(GPIO_CAM_MEGA_nRST, 0);
	
    // standby low
    gpio_set_value(GPIO_CAM_MEGA_EN, 0);
    // reset low
	gpio_set_value(GPIO_CAM_MEGA_nRST, 0);
	msleep(1);
	
    // Turn LDOs on
	s5k5ccgx_ldo_en(TRUE);
    udelay(5);

    // Enable MCLK
	s3c_gpio_cfgpin(GPIO_CAM_MCLK, S5PV210_GPE1_3_CAM_A_CLKOUT);
    udelay(10);

    // standby high
    gpio_set_value(GPIO_CAM_MEGA_EN, 1);
    udelay(50);

    // reset high
	gpio_set_value(GPIO_CAM_MEGA_nRST, 1);
	msleep(25);

	gpio_free(GPIO_CAM_MEGA_EN);
	gpio_free(GPIO_CAM_MEGA_nRST);
	
	return 0;
}

/****************************************************************************
 * s5k5ccgx_power_off_sequence()
 *
 * @param     void 
 * @return    err
 * @remark    Function
 *
 * Oct 15, 2010  initial revision
 *****************************************************************************/
static int s5k5ccgx_power_off_sequence(void)
{
	int err;
	
	printk("camera_power_off\n");

	/* CAM_MEGA_EN - GPJ0(6) => GPA0(6)*/
	err = gpio_request(GPIO_CAM_MEGA_EN, "GPA0");
	if(err)
	{
		printk(KERN_ERR "failed to request GPIO for camera standby\n");
		return err;
	}

	/* CAM_MEGA_nRST - GPJ1(5) => GPE1(4) */
	err = gpio_request(GPIO_CAM_MEGA_nRST, "GPE1");
	if(err) 
	{
		printk(KERN_ERR "failed to request GPE1(4) for camera control\n");
		return err;
	}

	gpio_direction_output(GPIO_CAM_MEGA_nRST, 1);
	gpio_direction_output(GPIO_CAM_MEGA_EN, 1);

    // reset low
	gpio_set_value(GPIO_CAM_MEGA_nRST, 0);
	udelay(100);

	// Disable MCLK
	s3c_gpio_cfgpin(GPIO_CAM_MCLK, 0);
	udelay(5);

    // standby low
	gpio_set_value(GPIO_CAM_MEGA_EN, 0); 
	udelay(5);

    // Turn LDOs off
	s5k5ccgx_ldo_en(FALSE);
	msleep(1);

	gpio_free(GPIO_CAM_MEGA_EN);
	gpio_free(GPIO_CAM_MEGA_nRST);

	return 0;
}

/****************************************************************************
 * s5k5ccgx_power_en()
 *
 * @param     on/off 
 * @return    err
 * @remark    Function
 *
 * Oct 15, 2010  initial revision
 *****************************************************************************/
static int s5k5ccgx_power_en(int onoff)
{
	if(onoff)
	{
		s5k5ccgx_power_on_sequence();
	} 
	else 
	{
		s5k5ccgx_power_off_sequence();
	}
	return 0;
}

static struct camera_platform_data s5k5ccgx_platform_data = {
	.default_width  = 1024,
	.default_height = 768,
	.pixelformat    = V4L2_PIX_FMT_UYVY,
	.mclk_freq      = 24000000,
	.is_mipi        = 0,
};

static struct i2c_board_info s5k5ccgx_i2c_board_info = 
{	
    I2C_BOARD_INFO("S5K5CCGX", (0x78 >> 1)),
    .platform_data = &s5k5ccgx_platform_data,
};

static struct s3c_platform_camera s5k5ccgx_platform_camera = 
{
	.id             = CAMERA_PAR_A,
	.type           = CAM_TYPE_ITU,
	.fmt            = ITU_601_YCBCR422_8BIT,
	.order422       = CAM_ORDER422_8BIT_CBYCRY,
	.i2c_busnum     = 0,
	.info           = &s5k5ccgx_i2c_board_info,
	.pixelformat    = V4L2_PIX_FMT_UYVY,
	.srclk_name     = "xusbxti",					
	.clk_name       = "sclk_cam0",
	.clk_rate       = 24000000,
	.line_length    = 1536,
	.width          = 1024,
	.height         = 768,
	.window = 
	{
		.left 	    = 0,
		.top        = 0,
		.width      = 1024,
		.height     = 768,
	},

	/* Polarity */
	.inv_pclk       = 0,
	.inv_vsync      = 1,
	.inv_href       = 0,
	.inv_hsync      = 0,
	.initialized    = 0,
	.cam_power      = s5k5ccgx_power_en,
};
#endif // CONFIG_VIDEO_S5K5CCGX

#ifdef CONFIG_VIDEO_CE147
/*
 * Guide for Camera Configuration for Jupiter board
 * ITU CAM CH A: CE147
*/
static void ce147_ldo_en(bool onoff)
{
	int err;

	//For Emul Rev0.1
	// Because B4, B5 do not use this GPIO, this GPIO is enabled in all HW version
	/* CAM_IO_EN - GPB(7) */
	err = gpio_request(GPIO_GPB7, "GPB7");

	if(err) {
		printk(KERN_ERR "failed to request GPB7 for camera control\n");

		return err;
	}

	if(onoff == TRUE) { //power on
		// Turn CAM_ISP_1.2V on
		Set_MAX8998_PM_OUTPUT_Voltage(BUCK4, VCC_1p200);

		Set_MAX8998_PM_REG(EN4, 1);

		mdelay(1);

		// Turn CAM_AF_2.8V on
		Set_MAX8998_PM_OUTPUT_Voltage(LDO11, VCC_2p800);

		Set_MAX8998_PM_REG(ELDO11, 1);

		// Turn CAM_SENSOR_1.2V on
		Set_MAX8998_PM_OUTPUT_Voltage(LDO12, VCC_1p200);

		Set_MAX8998_PM_REG(ELDO12, 1);

		// Turn CAM_SENSOR_A2.8V on
		Set_MAX8998_PM_OUTPUT_Voltage(LDO13, VCC_2p800);

		Set_MAX8998_PM_REG(ELDO13, 1);

		// Turn CAM_ISP_1.8V on
		Set_MAX8998_PM_OUTPUT_Voltage(LDO14, VCC_1p800);

		Set_MAX8998_PM_REG(ELDO14, 1);

		// Turn CAM_ISP_2.8V on
		Set_MAX8998_PM_OUTPUT_Voltage(LDO15, VCC_2p800);

		Set_MAX8998_PM_REG(ELDO15, 1);

		// Turn CAM_SENSOR_1.8V on
		Set_MAX8998_PM_OUTPUT_Voltage(LDO16, VCC_1p800);

		Set_MAX8998_PM_REG(ELDO16, 1);

		// Turn CAM_ISP_SYS_2.8V on
		gpio_direction_output(GPIO_GPB7, 0);

		gpio_set_value(GPIO_GPB7, 1);
	}

	else { // power off
		// Turn CAM_ISP_SYS_2.8V off
		gpio_direction_output(GPIO_GPB7, 1);

		gpio_set_value(GPIO_GPB7, 0);

		// Turn CAM_AF_2.8V off
		Set_MAX8998_PM_REG(ELDO11, 0);

		// Turn CAM_SENSOR_1.2V off
		Set_MAX8998_PM_REG(ELDO12, 0);

		// Turn CAM_SENSOR_A2.8V off
		Set_MAX8998_PM_REG(ELDO13, 0);

		// Turn CAM_ISP_1.8V off
		Set_MAX8998_PM_REG(ELDO14, 0);

		// Turn CAM_ISP_2.8V off
		Set_MAX8998_PM_REG(ELDO15, 0);

		// Turn CAM_SENSOR_1.8V off
		Set_MAX8998_PM_REG(ELDO16, 0);

		mdelay(1);

		// Turn CAM_ISP_1.2V off
		Set_MAX8998_PM_REG(EN4, 0);
	}

	gpio_free(GPIO_GPB7);
}


static int ce147_cam_en(bool onoff)
{
	int err;
	/* CAM_MEGA_EN - GPJ0(6) */
	err = gpio_request(S5PV210_GPJ0(6), "GPJ0");
        if (err) {
                printk(KERN_ERR "failed to request GPJ0 for camera control\n");
                return err;
        }

	gpio_direction_output(S5PV210_GPJ0(6), 0);
	msleep(1);
	gpio_direction_output(S5PV210_GPJ0(6), 1);
	msleep(1);

	if(onoff){
	gpio_set_value(S5PV210_GPJ0(6), 1);
	} else {
		gpio_set_value(S5PV210_GPJ0(6), 0);
	}
	msleep(1);

	gpio_free(S5PV210_GPJ0(6));

	return 0;
}

static int ce147_cam_nrst(bool onoff)
{
	int err;

	/* CAM_MEGA_nRST - GPJ1(5)*/
	err = gpio_request(S5PV210_GPJ1(5), "GPJ1");
        if (err) {
                printk(KERN_ERR "failed to request GPJ1 for camera control\n");
                return err;
        }

	gpio_direction_output(S5PV210_GPJ1(5), 0);
	msleep(1);
	gpio_direction_output(S5PV210_GPJ1(5), 1);
	msleep(1);

	gpio_set_value(S5PV210_GPJ1(5), 0);
	msleep(1);

	if(onoff){
	gpio_set_value(S5PV210_GPJ1(5), 1);
		msleep(1);
	}
	gpio_free(S5PV210_GPJ1(5));

	return 0;
}


static int ce147_power_on(void)
{
	int err;

	printk(KERN_DEBUG "ce147_power_on\n");

	/* CAM_MEGA_EN - GPJ0(6) */
	err = gpio_request(GPIO_CAM_MEGA_EN, "GPJ0");

	if(err) {
		printk(KERN_ERR "failed to request GPJ0 for camera control\n");

		return err;
	}

	/* CAM_MEGA_nRST - GPJ1(5) */
	err = gpio_request(GPIO_CAM_MEGA_nRST, "GPJ1");

	if(err) {
		printk(KERN_ERR "failed to request GPJ1 for camera control\n");

		return err;
	}

	/* CAM_VGA_nSTBY - GPB(0)  */
	err = gpio_request(GPIO_CAM_VGA_nSTBY, "GPB0");

	if (err) {
		printk(KERN_ERR "failed to request GPB0 for camera control\n");

		return err;
	}

	/* CAM_VGA_nRST - GPB(2) */
	err = gpio_request(GPIO_CAM_VGA_nRST, "GPB2");

	if (err) {
		printk(KERN_ERR "failed to request GPB2 for camera control\n");

		return err;
	}

	ce147_ldo_en(TRUE);

	mdelay(1);

	// CAM_VGA_nSTBY  HIGH
	gpio_direction_output(GPIO_CAM_VGA_nSTBY, 0);

	gpio_set_value(GPIO_CAM_VGA_nSTBY, 1);

	mdelay(1);

	// Mclk enable
	s3c_gpio_cfgpin(GPIO_CAM_MCLK, S5PV210_GPE1_3_CAM_A_CLKOUT);

	mdelay(1);

	// CAM_VGA_nRST  HIGH
	gpio_direction_output(GPIO_CAM_VGA_nRST, 0);

	gpio_set_value(GPIO_CAM_VGA_nRST, 1);

	mdelay(1);

	// CAM_VGA_nSTBY  LOW
	gpio_direction_output(GPIO_CAM_VGA_nSTBY, 1);

	gpio_set_value(GPIO_CAM_VGA_nSTBY, 0);

	mdelay(1);

	// CAM_MEGA_EN HIGH
	gpio_direction_output(GPIO_CAM_MEGA_EN, 0);

	gpio_set_value(GPIO_CAM_MEGA_EN, 1);

	mdelay(1);

	// CAM_MEGA_nRST HIGH
	gpio_direction_output(GPIO_CAM_MEGA_nRST, 0);

	gpio_set_value(GPIO_CAM_MEGA_nRST, 1);

	gpio_free(GPIO_CAM_MEGA_EN);

	gpio_free(GPIO_CAM_MEGA_nRST);

	gpio_free(GPIO_CAM_VGA_nSTBY);

	gpio_free(GPIO_CAM_VGA_nRST);

	mdelay(5);

	return 0;
}


static int ce147_power_off(void)
{
	int err;

	printk(KERN_DEBUG "ce147_power_off\n");

	/* CAM_MEGA_EN - GPJ0(6) */
	err = gpio_request(GPIO_CAM_MEGA_EN, "GPJ0");

	if(err) {
		printk(KERN_ERR "failed to request GPJ0 for camera control\n");

		return err;
	}

	/* CAM_MEGA_nRST - GPJ1(5) */
	err = gpio_request(GPIO_CAM_MEGA_nRST, "GPJ1");

	if(err) {
		printk(KERN_ERR "failed to request GPJ1 for camera control\n");

		return err;
	}

	/* CAM_VGA_nRST - GPB(2) */
	err = gpio_request(GPIO_CAM_VGA_nRST, "GPB2");

	if (err) {
		printk(KERN_ERR "failed to request GPB2 for camera control\n");

		return err;
	}

	// CAM_VGA_nRST  LOW
	gpio_direction_output(GPIO_CAM_VGA_nRST, 1);

	gpio_set_value(GPIO_CAM_VGA_nRST, 0);

	mdelay(1);

	// CAM_MEGA_nRST - GPJ1(5) LOW
	gpio_direction_output(GPIO_CAM_MEGA_nRST, 1);

	gpio_set_value(GPIO_CAM_MEGA_nRST, 0);

	mdelay(1);

	// Mclk disable
	s3c_gpio_cfgpin(GPIO_CAM_MCLK, 0);

	mdelay(1);

	// CAM_MEGA_EN - GPJ0(6) LOW
	gpio_direction_output(GPIO_CAM_MEGA_EN, 1);

	gpio_set_value(GPIO_CAM_MEGA_EN, 0);

	mdelay(1);

	ce147_ldo_en(FALSE);

	mdelay(1);

	gpio_free(GPIO_CAM_MEGA_EN);

	gpio_free(GPIO_CAM_MEGA_nRST);

	gpio_free(GPIO_CAM_VGA_nRST);

	return 0;
}


static int ce147_power_en(int onoff)
{
	int bd_level;
#if 0
	if(onoff){
		ce147_ldo_en(true);
		s3c_gpio_cfgpin(S5PV210_GPE1(3), S5PV210_GPE1_3_CAM_A_CLKOUT);
		ce147_cam_en(true);
		ce147_cam_nrst(true);
	} else {
		ce147_cam_en(false);
		ce147_cam_nrst(false);
		s3c_gpio_cfgpin(S5PV210_GPE1(3), 0);
		ce147_ldo_en(false);
	}

	return 0;
#endif

	if(onoff == 1) {
		ce147_power_on();
	}

	else {
		ce147_power_off();
		s3c_i2c0_force_stop();
	}

	return 0;
}

static int smdkc110_cam1_power(int onoff)
{
	int err;
	/* Implement on/off operations */

	/* CAM_VGA_nSTBY - GPB(0) */
	err = gpio_request(S5PV210_GPB(0), "GPB");

	if (err) {
		printk(KERN_ERR "failed to request GPB for camera control\n");

		return err;
	}

	gpio_direction_output(S5PV210_GPB(0), 0);

	mdelay(1);

	gpio_direction_output(S5PV210_GPB(0), 1);

	mdelay(1);

	gpio_set_value(S5PV210_GPB(0), 1);

	mdelay(1);

	gpio_free(S5PV210_GPB(0));

	mdelay(1);

	/* CAM_VGA_nRST - GPB(2) */
	err = gpio_request(S5PV210_GPB(2), "GPB");

	if (err) {
		printk(KERN_ERR "failed to request GPB for camera control\n");

		return err;
	}

	gpio_direction_output(S5PV210_GPB(2), 0);

	mdelay(1);

	gpio_direction_output(S5PV210_GPB(2), 1);

	mdelay(1);

	gpio_set_value(S5PV210_GPB(2), 1);

	mdelay(1);

	gpio_free(S5PV210_GPB(2));

	return 0;
}

/*
 * Guide for Camera Configuration for Jupiter board
 * ITU CAM CH A: CE147
*/

/* External camera module setting */
static struct ce147_platform_data ce147_plat = {
	.default_width = 640,
	.default_height = 480,
	.pixelformat = V4L2_PIX_FMT_UYVY,
	.freq = 24000000,
	.is_mipi = 0,
};

static struct i2c_board_info  ce147_i2c_info = {
	I2C_BOARD_INFO("CE147", 0x78>>1),
	.platform_data = &ce147_plat,
};

static struct s3c_platform_camera ce147 = {
	.id		= CAMERA_PAR_A,
	.type		= CAM_TYPE_ITU,
	.fmt		= ITU_601_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_CBYCRY,
	.i2c_busnum	= 0,
	.info		= &ce147_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_UYVY,
	.srclk_name	= "xusbxti",
	.clk_name	= "sclk_cam0",
	.clk_rate	= 24000000,
	.line_length	= 1920,
	.width		= 640,
	.height		= 480,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 640,
		.height	= 480,
	},

	/* Polarity */
	.inv_pclk	= 0,
	.inv_vsync 	= 1,
	.inv_href	= 0,
	.inv_hsync	= 0,

	.initialized 	= 0,
	.cam_power	= ce147_power_en,
};
#endif

/* External camera module setting */
#ifdef CONFIG_VIDEO_S5KA3DFX
static void s5ka3dfx_ldo_en(bool onoff)
{
//////////////[5B] To fix build error
#if 0
	if(onoff){
		pmic_ldo_enable(LDO_CAM_IO);
		pmic_ldo_enable(LDO_CAM_A);
		pmic_ldo_enable(LDO_CAM_CIF);
	} else {
		pmic_ldo_disable(LDO_CAM_IO);
		pmic_ldo_disable(LDO_CAM_A);
		pmic_ldo_disable(LDO_CAM_CIF);
	}
#endif
}

static int s5ka3dfx_cam_stdby(bool onoff)
{
	int err;
	/* CAM_VGA_nSTBY - GPB(0) */
	err = gpio_request(S5PV210_GPB(0), "GPB");
	if (err) {
		printk(KERN_ERR "failed to request GPJ0 for camera control\n");
		return err;
	}

	gpio_direction_output(S5PV210_GPB(0), 0);
	msleep(1);
	gpio_direction_output(S5PV210_GPB(0), 1);
	msleep(1);

	if(onoff){
		gpio_set_value(S5PV210_GPB(0), 1);
	} else {
		gpio_set_value(S5PV210_GPB(0), 0);
	}
	msleep(1);

	gpio_free(S5PV210_GPB(0));

	return 0;
}

static int s5ka3dfx_cam_nrst(bool onoff)
{
	int err;

	/* CAM_VGA_nRST - GPB(2)*/
	err = gpio_request(S5PV210_GPB(2), "GPB");
	if (err) {
		printk(KERN_ERR "failed to request GPJ1 for camera control\n");
		return err;
	}

	gpio_direction_output(S5PV210_GPB(2), 0);
	msleep(1);
	gpio_direction_output(S5PV210_GPB(2), 1);
	msleep(1);

	gpio_set_value(S5PV210_GPB(2), 0);
	msleep(1);

	if(onoff){
		gpio_set_value(S5PV210_GPB(2), 1);
		msleep(1);
	}
	gpio_free(S5PV210_GPB(2));

	return 0;
}



static int s5ka3dfx_power_on()
{
	int err;

	printk(KERN_DEBUG "s5ka3dfx_power_on\n");

	/* CAM_VGA_nSTBY - GPB(0)  */
	err = gpio_request(GPIO_CAM_VGA_nSTBY, "GPB0");

	if (err) {
		printk(KERN_ERR "failed to request GPB0 for camera control\n");

		return err;
	}

	/* CAM_VGA_nRST - GPB(2) */
	err = gpio_request(GPIO_CAM_VGA_nRST, "GPB2");

	if (err) {
		printk(KERN_ERR "failed to request GPB2 for camera control\n");

		return err;
	}

	/* CAM_IO_EN - GPB(7) */
	err = gpio_request(GPIO_GPB7, "GPB7");

	if(err) {
		printk(KERN_ERR "failed to request GPB7 for camera control\n");

		return err;
	}

	// Turn CAM_ISP_SYS_2.8V on
	gpio_direction_output(GPIO_GPB7, 0);
	gpio_set_value(GPIO_GPB7, 1);

	mdelay(1);

	// Turn CAM_SENSOR_A2.8V on
	Set_MAX8998_PM_OUTPUT_Voltage(LDO13, VCC_2p800);
	Set_MAX8998_PM_REG(ELDO13, 1);

	mdelay(1);

	// Turn CAM_ISP_HOST_2.8V on
	Set_MAX8998_PM_OUTPUT_Voltage(LDO15, VCC_2p800);
	Set_MAX8998_PM_REG(ELDO15, 1);

	mdelay(1);

	// Turn CAM_ISP_RAM_1.8V on
	Set_MAX8998_PM_OUTPUT_Voltage(LDO14, VCC_1p800);
	Set_MAX8998_PM_REG(ELDO14, 1);

	mdelay(1);

	gpio_free(GPIO_GPB7);

	// CAM_VGA_nSTBY  HIGH
	gpio_direction_output(GPIO_CAM_VGA_nSTBY, 0);
	gpio_set_value(GPIO_CAM_VGA_nSTBY, 1);

	mdelay(1);

	// Mclk enable
	s3c_gpio_cfgpin(GPIO_CAM_MCLK, S5PV210_GPE1_3_CAM_A_CLKOUT);

	mdelay(1);

	// CAM_VGA_nRST  HIGH
	gpio_direction_output(GPIO_CAM_VGA_nRST, 0);
	gpio_set_value(GPIO_CAM_VGA_nRST, 1);

	mdelay(4);

	gpio_free(GPIO_CAM_VGA_nSTBY);
	gpio_free(GPIO_CAM_VGA_nRST);

	return 0;
}



static int s5ka3dfx_power_off()
{
	int err;

	printk(KERN_DEBUG "s5ka3dfx_power_off\n");

	/* CAM_VGA_nSTBY - GPB(0)  */
	err = gpio_request(GPIO_CAM_VGA_nSTBY, "GPB0");

	if (err) {
		printk(KERN_ERR "failed to request GPB for camera control\n");

		return err;
	}

	/* CAM_VGA_nRST - GPB(2) */
	err = gpio_request(GPIO_CAM_VGA_nRST, "GPB2");

	if (err) {
		printk(KERN_ERR "failed to request GPB for camera control\n");

		return err;
	}


	// CAM_VGA_nRST  LOW
	gpio_direction_output(GPIO_CAM_VGA_nRST, 1);
	gpio_set_value(GPIO_CAM_VGA_nRST, 0);

	mdelay(1);

	// Mclk disable
	s3c_gpio_cfgpin(GPIO_CAM_MCLK, 0);

	mdelay(1);

	// CAM_VGA_nSTBY  LOW
	gpio_direction_output(GPIO_CAM_VGA_nSTBY, 1);
	gpio_set_value(GPIO_CAM_VGA_nSTBY, 0);

	mdelay(1);

	/* CAM_IO_EN - GPB(7) */
	err = gpio_request(GPIO_GPB7, "GPB7");

	if(err) {
		printk(KERN_ERR "failed to request GPB for camera control\n");

		return err;
	}

	// Turn CAM_ISP_HOST_2.8V off
	Set_MAX8998_PM_REG(ELDO15, 0);

	mdelay(1);

	// Turn CAM_SENSOR_A2.8V off
	Set_MAX8998_PM_REG(ELDO13, 0);

	// Turn CAM_ISP_RAM_1.8V off
	Set_MAX8998_PM_REG(ELDO14, 0);

	// Turn CAM_ISP_SYS_2.8V off
	gpio_direction_output(GPIO_GPB7, 1);
	gpio_set_value(GPIO_GPB7, 0);

	gpio_free(GPIO_GPB7);
	gpio_free(GPIO_CAM_VGA_nSTBY);
	gpio_free(GPIO_CAM_VGA_nRST);

	return 0;
}



static int s5ka3dfx_power_en(int onoff)
{
#if 0
	if(onoff){
		s5ka3dfx_ldo_en(true);
		s3c_gpio_cfgpin(S5PV210_GPE1(3), S5PV210_GPE1_3_CAM_A_CLKOUT);
		s5ka3dfx_cam_stdby(true);
		s5ka3dfx_cam_nrst(true);
		mdelay(100);
	} else {
		s5ka3dfx_cam_stdby(false);
		s5ka3dfx_cam_nrst(false);
		s3c_gpio_cfgpin(S5PV210_GPE1(3), 0);
		s5ka3dfx_ldo_en(false);
	}

	return 0;
#endif

	if(onoff){
		s5ka3dfx_power_on();
	} else {
		s5ka3dfx_power_off();
		s3c_i2c0_force_stop();
	}

	return 0;
}



static struct s5ka3dfx_platform_data s5ka3dfx_plat = {
	.default_width = 640,
	.default_height = 480,
	.pixelformat = V4L2_PIX_FMT_UYVY,
	.freq = 24000000,
	.is_mipi = 0,
};

static struct i2c_board_info  s5ka3dfx_i2c_info = {
	I2C_BOARD_INFO("S5KA3DFX", 0xc4>>1),
	.platform_data = &s5ka3dfx_plat,
};

static struct s3c_platform_camera s5ka3dfx = {
	.id		= CAMERA_PAR_A,
	.type		= CAM_TYPE_ITU,
	.fmt		= ITU_601_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_CBYCRY,
	.i2c_busnum	= 0,
	.info		= &s5ka3dfx_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_UYVY,
	.srclk_name	= "xusbxti",
	.clk_name	= "sclk_cam0",
	.clk_rate	= 24000000,
	.line_length	= 480,
	.width		= 640,
	.height		= 480,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 640,
		.height	= 480,
	},

	/* Polarity */
	.inv_pclk	= 0,
	.inv_vsync 	= 1,
	.inv_href	= 0,
	.inv_hsync	= 0,

	.initialized 	= 0,
	.cam_power	= s5ka3dfx_power_en,
};
#endif


/* Interface setting */
static struct s3c_platform_fimc fimc_plat = {
	.srclk_name	= "mout_mpll",
	.clk_name	= "sclk_fimc_lclk",
	.clk_rate	= 166750000,
	.default_cam	= CAMERA_PAR_A,
	.camera		= {
#ifdef CONFIG_VIDEO_CE147
		&ce147,
#endif // CONFIG_VIDEO_CE147
#ifdef CONFIG_VIDEO_S5KA3DFX
		&s5ka3dfx,
#endif // CONFIG_VIDEO_S5KA3DFX
#ifdef CONFIG_VIDEO_S5K5CCGX
		&s5k5ccgx_platform_camera,
#endif // CONFIG_VIDEO_S5K5CCGX
	},
	.hw_ver		= 0x43,
};

#endif

#if defined(CONFIG_HAVE_PWM)
static struct platform_pwm_backlight_data smdk_backlight_data = {
	.pwm_id  = 3,
	.max_brightness = 255,
	.dft_brightness = 255,
	.pwm_period_ns  = 78770,
};

static struct platform_device smdk_backlight_device = {
	.name      = "pwm-backlight",
	.id        = -1,
	.dev        = {
		.parent = &s3c_device_timer[3].dev,
		.platform_data = &smdk_backlight_data,
	},
};
static void __init smdk_backlight_register(void)
{
	int ret = platform_device_register(&smdk_backlight_device);
	if (ret)
		printk(KERN_ERR "smdk: failed to register backlight device: %d\n", ret);
}
#endif

#if defined(CONFIG_BLK_DEV_IDE_S3C)
static struct s3c_ide_platdata smdkv210_ide_pdata __initdata = {
	.setup_gpio     = s3c_ide_setup_gpio,
};
#endif

/* I2C0 */
static struct i2c_board_info i2c_devs0[] __initdata = {
};

static struct i2c_board_info i2c_devs4[] __initdata = {
#ifdef CONFIG_SND_SOC_WM8580
	{
		I2C_BOARD_INFO("wm8580", 0x1b),
	},
#endif
#ifdef CONFIG_SND_SOC_WM8994
	{
		I2C_BOARD_INFO("wm8994", (0x34>>1)),
	},
#endif
};


/* I2C1 */
static struct i2c_board_info i2c_devs1[] __initdata = {
};

/* i2c board & device info. */
static struct qt602240_platform_data qt602240_p1_platform_data = {
        .x_line = 19,
        .y_line = 11,
        .x_size = 1024,
        .y_size = 1024,
        .blen = 0x41,
        .threshold = 0x30,
        .orient = QT602240_VERTICAL_FLIP,
};

/* I2C2 */
static struct i2c_board_info i2c_devs2[] __initdata = {
    {
        I2C_BOARD_INFO("qt602240_ts", 0x4a),
        .platform_data  = &qt602240_p1_platform_data,
    },
};


/* I2C2 */

#if 0 // TODO:FORTE
static struct i2c_board_info i2c_devs10[] __initdata = {
    {
        I2C_BOARD_INFO("melfas_touchkey", 0x20),
       // .platform_data  = &qt602240_p1_platform_data,
    },
};
#endif

static struct i2c_board_info i2c_devs5[] __initdata = {
	{
		I2C_BOARD_INFO("kr3dh", 0x19),
	},
	{
		I2C_BOARD_INFO("fsa9480", (0x4A >> 1)),
	},
	{
		I2C_BOARD_INFO("yas529", 0x2E),
	},
};

static struct i2c_board_info i2c_devs6[] __initdata = {
#ifdef CONFIG_REGULATOR_MAX8998
	{
		/* The address is 0xCC used since SRAD = 0 */
		I2C_BOARD_INFO("max8998", (0xCC >> 1)),
		.platform_data = &max8998_platform_data,
	},
	{
		I2C_BOARD_INFO("rtc_max8998", (0x0D >> 1)),
	},
#endif
};

#if 0 // TODO:FORTE
static struct i2c_board_info i2c_devs7[] __initdata = {
	{
		I2C_BOARD_INFO("fsa9480", (0x4A >> 1)),
	},
};

static struct i2c_board_info i2c_devs8[] __initdata = {
	{
		I2C_BOARD_INFO("Si4709", (0x20 >> 1)),
	},
};
#endif

static struct i2c_board_info i2c_devs9[] __initdata = {
	{
		I2C_BOARD_INFO("fuelgauge", (0x6D >> 1)),
	},
};

static struct i2c_board_info i2c_devs11[] __initdata = {
	{
		I2C_BOARD_INFO("gp2a", (0x88 >> 1)),
	},
};

static struct i2c_board_info i2c_devs12[] __initdata = {
	{
		I2C_BOARD_INFO("bl_bd60910", (0xEC >> 1)),
	},
};

#ifdef CONFIG_DM9000
static void __init smdkv210_dm9000_set(void)
{
	unsigned int tmp;

	tmp = ((0<<28)|(0<<24)|(5<<16)|(0<<12)|(0<<8)|(0<<4)|(0<<0));
	__raw_writel(tmp, (S5P_SROM_BW+0x18));

	tmp = __raw_readl(S5P_SROM_BW);
	tmp &= ~(0xf << 20);

#ifdef CONFIG_DM9000_16BIT
	tmp |= (0x1 << 20);
#else
	tmp |= (0x2 << 20);
#endif
	__raw_writel(tmp, S5P_SROM_BW);

	tmp = __raw_readl(S5PV210_MP01CON);
	tmp &= ~(0xf << 20);
	tmp |= (2 << 20);

	__raw_writel(tmp, S5PV210_MP01CON);
}
#endif

#ifdef CONFIG_ANDROID_PMEM
static struct android_pmem_platform_data pmem_pdata = {
	.name = "pmem",
	.no_allocator = 1,
	.cached = 1,
	.start = 0, // will be set during proving pmem driver.
	.size = 0 // will be set during proving pmem driver.
};

static struct android_pmem_platform_data pmem_gpu1_pdata = {
   .name = "pmem_gpu1",
   .no_allocator = 1,
   .cached = 1,
   .buffered = 1,
   .start = 0,
   .size = 0,
};

static struct android_pmem_platform_data pmem_adsp_pdata = {
   .name = "pmem_adsp",
   .no_allocator = 1,
   .cached = 1,
   .buffered = 1,
   .start = 0,
   .size = 0,
};

static struct platform_device pmem_device = {
   .name = "android_pmem",
   .id = 0,
   .dev = { .platform_data = &pmem_pdata },
};

static struct platform_device pmem_gpu1_device = {
	.name = "android_pmem",
	.id = 1,
	.dev = { .platform_data = &pmem_gpu1_pdata },
};

static struct platform_device pmem_adsp_device = {
	.name = "android_pmem",
	.id = 2,
	.dev = { .platform_data = &pmem_adsp_pdata },
};

static void __init android_pmem_set_platdata(void)
{
	pmem_pdata.start = (u32)s3c_get_media_memory_bank(S3C_MDEV_PMEM, 0);
	pmem_pdata.size = (u32)s3c_get_media_memsize_bank(S3C_MDEV_PMEM, 0);

	pmem_gpu1_pdata.start = (u32)s3c_get_media_memory_bank(S3C_MDEV_PMEM_GPU1, 0);
	pmem_gpu1_pdata.size = (u32)s3c_get_media_memsize_bank(S3C_MDEV_PMEM_GPU1, 0);

	pmem_adsp_pdata.start = (u32)s3c_get_media_memory_bank(S3C_MDEV_PMEM_ADSP, 0);
	pmem_adsp_pdata.size = (u32)s3c_get_media_memsize_bank(S3C_MDEV_PMEM_ADSP, 0);
}
#endif
struct platform_device sec_device_battery = {
	.name	= "sec-battery", //Added for Froyo Atlas
	.id		= -1,
};

static struct platform_device opt_gp2a = {
	.name = "gp2a-opt",
	.id = -1,
};

/********************/
/* bluetooth - start >> */

static struct platform_device	sec_device_rfkill = {
	.name = "bt_rfkill",
	.id	  = -1,
};

static struct platform_device	sec_device_btsleep = {
	.name = "bt_sleep",
	.id	  = -1,
};

/* << bluetooth -end */
/******************/

#if defined(CONFIG_SAMSUNG_PHONE_SVNET) || defined(CONFIG_SAMSUNG_PHONE_SVNET_MODULE)

/* onedram */
static void onedram_cfg_gpio(void);

static struct onedram_platform_data onedram_data = {
	.cfg_gpio = onedram_cfg_gpio,
};

static struct resource onedram_res[] = {
	[0] = {
		.start = (S5PV210_PA_SDRAM + 0x05000000),
		.end = (S5PV210_PA_SDRAM + 0x05000000 + SZ_16M - 1),
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_EINT11,
		.end = IRQ_EINT11,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device onedram = {
	.name = "onedram",
	.id = -1,
	.num_resources = ARRAY_SIZE(onedram_res),
	.resource = onedram_res,
	.dev = {
		.platform_data = &onedram_data,
	},
};

static void onedram_cfg_gpio(void)
{
	//unsigned gpio_onedram_int_ap = S5PC11X_GPH1(3);
	s3c_gpio_cfgpin(GPIO_nINT_ONEDRAM_AP, S3C_GPIO_SFN(GPIO_nINT_ONEDRAM_AP_AF));
	s3c_gpio_setpull(GPIO_nINT_ONEDRAM_AP, S3C_GPIO_PULL_UP);
	set_irq_type(GPIO_nINT_ONEDRAM_AP, IRQ_TYPE_LEVEL_LOW);
}


/* Modem control */
static void modemctl_cfg_gpio(void);

static struct modemctl_platform_data mdmctl_data = {
	.name = "lte",
	.gpio_phone_active = S5PV210_GPH1(0),
	.gpio_pda_active = S5PV210_GPF3(5),
	.gpio_phone_on = S5PV210_GPH1(6),
	.gpio_cp_reset = S5PV210_GPH1(4),
	.cfg_gpio = modemctl_cfg_gpio,
};

static struct resource mdmctl_res[] = {
	[0] = {
		.start = IRQ_EINT8,
		.end = IRQ_EINT8,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device modemctl = {
	.name = "modemctl",
	.id = -1,
	.num_resources = ARRAY_SIZE(mdmctl_res),
	.resource = mdmctl_res,
	.dev = {
		.platform_data = &mdmctl_data,
	},
};

static void modemctl_cfg_gpio(void)
{
	int err = 0;

	unsigned gpio_phone_on = mdmctl_data.gpio_phone_on;
	unsigned gpio_phone_active = mdmctl_data.gpio_phone_active;
	unsigned gpio_cp_rst = mdmctl_data.gpio_cp_reset;
	unsigned gpio_pda_active = mdmctl_data.gpio_pda_active;
	unsigned gpio_sim_ndetect = mdmctl_data.gpio_sim_ndetect;

	err = gpio_request(gpio_phone_on, "PHONE_ON");
	if (err) {
		printk("fail to request gpio %s\n","PHONE_ON");
	} else {
		gpio_direction_output(gpio_phone_on, GPIO_LEVEL_LOW);
		s3c_gpio_setpull(gpio_phone_on, S3C_GPIO_PULL_NONE);
	}

	err = gpio_request(gpio_cp_rst, "CP_RST");
	if (err) {
		printk("fail to request gpio %s\n","CP_RST");
	} else {
		gpio_direction_output(gpio_cp_rst, GPIO_LEVEL_LOW);
		s3c_gpio_setpull(gpio_cp_rst, S3C_GPIO_PULL_UP);
	}

	err = gpio_request(gpio_pda_active, "PDA_ACTIVE");
	if (err) {
		printk("fail to request gpio %s\n","PDA_ACTIVE");
	} else {
		gpio_direction_output(gpio_pda_active, GPIO_LEVEL_HIGH);
		s3c_gpio_setpull(gpio_pda_active, S3C_GPIO_PULL_NONE);
	}

	s3c_gpio_cfgpin(gpio_phone_active, S3C_GPIO_INPUT);
	s3c_gpio_setpull(gpio_phone_active, S3C_GPIO_PULL_NONE);
	set_irq_type(gpio_phone_active, IRQ_TYPE_EDGE_BOTH);

	s3c_gpio_cfgpin(gpio_sim_ndetect, S3C_GPIO_SFN(0xF));
	s3c_gpio_setpull(gpio_sim_ndetect, S3C_GPIO_PULL_NONE);
	set_irq_type(gpio_sim_ndetect, IRQ_TYPE_EDGE_BOTH);
}
#endif  /*CONFIG_SAMSUNG_PHONE_SVNET || CONFIG_SAMSUNG_PHONE_SVNET_MODULE*/



/*Adding gpio settings during bootup and sleep*/


void s3c_config_gpio_table(int array_size, unsigned int (*gpio_table)[8])
{
        u32 i, gpio;
        for (i = 0; i < array_size; i++) {
		gpio = gpio_table[i][0];
		/* Off part */
		if((gpio <= S5PV210_GPG3(6)) ||
		   ((gpio <= S5PV210_GPJ4(7)) && (gpio >= S5PV210_GPI(0)))) {

        	        s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(gpio_table[i][1]));
                	s3c_gpio_setpull(gpio, gpio_table[i][3]);

                	if (gpio_table[i][2] != S3C_GPIO_SETPIN_NONE)
                    		gpio_set_value(gpio, gpio_table[i][2]);

                	s3c_gpio_set_drvstrength(gpio, gpio_table[i][4]);
                	s3c_gpio_set_slewrate(gpio, gpio_table[i][5]);

                	//s3c_gpio_slp_cfgpin(gpio, gpio_table[i][6]);
                	//s3c_gpio_slp_setpull_updown(gpio, gpio_table[i][7]);

		}
#if 1
		/* Alive part */
		else if((gpio <= S5PV210_GPH3(7)) && (gpio >= S5PV210_GPH0(0))) {
        	        s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(gpio_table[i][1]));
                	s3c_gpio_setpull(gpio, gpio_table[i][3]);

                	if (gpio_table[i][2] != S3C_GPIO_SETPIN_NONE)
                    		gpio_set_value(gpio, gpio_table[i][2]);

                	s3c_gpio_set_drvstrength(gpio, gpio_table[i][4]);
                	s3c_gpio_set_slewrate(gpio, gpio_table[i][5]);
		}
#endif
#if 0
		/* Memory part */
		else if((gpio >  S5PV210_GPJ4(4)) && (gpio <= S5PV210_MP07(7))) {
        	        s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(gpio_table[i][1]));
                	s3c_gpio_setpull(gpio, gpio_table[i][3]);

                	if (gpio_table[i][2] != S3C_GPIO_SETPIN_NONE)
                    		gpio_set_value(gpio, gpio_table[i][2]);

                	s3c_gpio_set_drvstrength(gpio, gpio_table[i][4]);
                	s3c_gpio_set_slewrate(gpio, gpio_table[i][5]);

                	//s3c_gpio_slp_cfgpin(gpio, gpio_table[i][6]);
                	//s3c_gpio_slp_setpull_updown(gpio, gpio_table[i][7]);

		}
#endif
	}
      //Thomas Ryu, 20100401 to set 4mA of this GPIO
	  // Added for Froyo Atlas
// TODO:SLEEP_DK08    s3c_gpio_set_drvstrength(S5PV210_GPH3(7), S3C_GPIO_DRVSTR_2X);
}


#define S5PV210_PS_HOLD_CONTROL_REG (S3C_VA_SYS+0xE81C)

static void smdkc110_power_off(void)
{
	int err;
#if 0
	printk("smdkc110_power_off\n");

	/* temporary power off code */
	/*PS_HOLD high  PS_HOLD_CONTROL, R/W, 0xE010_E81C */
	writel(readl(S5PV210_PS_HOLD_CONTROL_REG) & 0xFFFFFEFF,
	       S5PV210_PS_HOLD_CONTROL_REG);

#else
	int mode = REBOOT_MODE_NONE;
	char reset_mode = 'r';
	int phone_wait_cnt = 0;

	// Change this API call just before power-off to take the dump.
	// kernel_sec_clear_upload_magic_number();

	err = gpio_request(GPIO_N_POWER, "GPIO_N_POWER"); // will never be freed
	WARN(err, "failed to request GPIO_N_POWER");

	err = gpio_request(GPIO_PHONE_ACTIVE, "GPIO_PHONE_ACTIVE");
	WARN(err, "failed to request GPIO_PHONE_ACTIVE"); // will never be freed

	gpio_direction_input(GPIO_N_POWER);
	gpio_direction_input(GPIO_PHONE_ACTIVE);

	gpio_set_value(GPIO_PHONE_ON, 0);	//prevent phone reset when AP off

	// confirm phone off
#if 1
	while (1) {
		if (gpio_get_value(GPIO_PHONE_ACTIVE)) {
			//if (phone_wait_cnt > 10) {  /* Added for Froyo Atlas */
				printk(KERN_EMERG
				       "%s: Try to Turn Phone Off by CP_RST\n",
				       __func__);
			   //gpio_set_value(GPIO_CP_RST, 0);
			//}
			if (phone_wait_cnt > 12) {
				printk(KERN_EMERG "%s: PHONE OFF Failed\n",
				       __func__);
				break;
			}
			phone_wait_cnt++;
			msleep(1000);
		} else {
			printk(KERN_EMERG "%s: PHONE OFF Success\n", __func__);
			break;
		}
	}
#endif
#if 0				// check JIG connection
	// never watchdog reset at JIG. It cause the unwanted reset at final IMEI progress
	// infinite delay is better than reset because jig is not the user case.
	if (get_usb_cable_state() &
	    (JIG_UART_ON | JIG_UART_OFF | JIG_USB_OFF | JIG_USB_ON)) {
		/* Watchdog Reset */
		printk(KERN_EMERG "%s: JIG is connected, rebooting...\n",
		       __func__);
		arch_reset(reset_mode);
		printk(KERN_EMERG "%s: waiting for reset!\n", __func__);
		while (1) ;
	}
#endif

//	while (1) {
		// Reboot Charging
      if (Get_MAX8998_PM_REG(VDCINOK_status) == 1) {  // SCH-I510 (P110117-0044): Fix kernel panic during power on/off auto-test.
			mode = REBOOT_MODE_CHARGING;
			if (sec_set_param_value)
				sec_set_param_value(__REBOOT_MODE, &mode);
			/* Watchdog Reset */
			printk(KERN_EMERG
			       "%s: TA or USB connected, rebooting...\n",
			       __func__);
			kernel_sec_clear_upload_magic_number();
			kernel_sec_hw_reset(TRUE);
			printk(KERN_EMERG "%s: waiting for reset!\n", __func__);
			while (1) ;
		}
		kernel_sec_clear_upload_magic_number();
		// wait for power button release
//		if (gpio_get_value(GPIO_N_POWER)) {
		printk(KERN_EMERG "%s: set PS_HOLD low.\n", __func__);

			/*PS_HOLD high  PS_HOLD_CONTROL, R/W, 0xE010_E81C */
		writel(readl(S5PV210_PS_HOLD_CONTROL_REG) & 0xFFFFFEFF,
		       S5PV210_PS_HOLD_CONTROL_REG);

		printk(KERN_EMERG "%s: should not reach here!\n",
		       __func__);
//		}
		// if power button is not released, wait for a moment. then check TA again.
		printk(KERN_EMERG "%s: PowerButton is not released.\n",
		       __func__);
//		mdelay(1000);
//	}
#endif

	while (1) ;
}




void s3c_config_sleep_gpio_table(int array_size, unsigned int (*gpio_table)[3])
{
	u32 i, gpio;

	for (i = 0; i < array_size; i++)
	{
		gpio = gpio_table[i][0];
		s3c_gpio_slp_cfgpin(gpio, gpio_table[i][1]);
		s3c_gpio_slp_setpull_updown(gpio, gpio_table[i][2]);
	}

	if (gpio_get_value(GPIO_PS_ON))
	{
		s3c_gpio_slp_setpull_updown(GPIO_ALS_SDA_28V, S3C_GPIO_PULL_NONE);
		s3c_gpio_slp_setpull_updown(GPIO_ALS_SCL_28V, S3C_GPIO_PULL_NONE);
	}
	else
	{
		s3c_gpio_setpull(GPIO_PS_VOUT, S3C_GPIO_PULL_DOWN);
	}

	printk(KERN_DEBUG "SLPGPIO : BT(%d) WLAN(%d) BT+WIFI(%d)\n",
		gpio_get_value(GPIO_BT_nRST),gpio_get_value(GPIO_WLAN_nRST),gpio_get_value(GPIO_WLAN_BT_EN));
	printk(KERN_DEBUG "SLPGPIO : CODEC_LDO_EN(%d) MICBIAS_EN(%d) EARPATH_SEL(%d)\n",
		gpio_get_value(GPIO_CODEC_LDO_EN),gpio_get_value(GPIO_MICBIAS_EN),gpio_get_value(GPIO_EARPATH_SEL));
#if 0 // TODO:FORTE
	printk(KERN_DEBUG "SLPGPIO : PS_ON(%d) FM_RST(%d) UART_SEL(%d)\n",
		gpio_get_value(GPIO_PS_ON),gpio_get_value(GPIO_FM_RST),gpio_get_value(GPIO_UART_SEL));
#endif
}

// just for ref..
//
void s3c_config_sleep_gpio(void)
#if 1
{
    extern char lte_factory_test; // LTE_Factory_test

    /* VOUT_PS */
	s3c_gpio_cfgpin(S5PV210_GPH0(1), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH0(1), S3C_GPIO_PULL_NONE);

    /* 3_POLE_ SEND_END */
	s3c_gpio_cfgpin(S5PV210_GPH0(2), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH0(2), S3C_GPIO_PULL_NONE);

    /* WLAN_HOST_WAKE */
	s3c_gpio_cfgpin(S5PV210_GPH0(3), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH0(3), S3C_GPIO_PULL_NONE);

    /*  BT_HOST_WAKE */
	s3c_gpio_cfgpin(S5PV210_GPH0(4), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH0(4), S3C_GPIO_PULL_NONE);

    /* HALL_SW */
	s3c_gpio_cfgpin(S5PV210_GPH0(5), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH0(5), S3C_GPIO_PULL_NONE);

    /* EAR_DET */
	s3c_gpio_cfgpin(S5PV210_GPH0(6), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH0(6), S3C_GPIO_PULL_NONE);      /* HW PULL_UP */

    /* AP_PMIC_IRQ */
	s3c_gpio_cfgpin(S5PV210_GPH0(7), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH0(7), S3C_GPIO_PULL_NONE);      /* HW PULL_UP */

    // TODO:YUNG_TEST DPRAM_C111_INT_N (naming miss, Active High)

   // TODO: H/W Req.
    /* LTE_ACTIVE */
    s3c_gpio_cfgpin(S5PV210_GPH1(0), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH1(0), S3C_GPIO_PULL_NONE);
    
    /* DPRAM_C111_INT_N */
	s3c_gpio_cfgpin(S5PV210_GPH1(1), S3C_GPIO_SFN(0xF));
	s3c_gpio_setpull(S5PV210_GPH1(1), S3C_GPIO_PULL_UP);

	/* FUEL_GUAGE */
	s3c_gpio_cfgpin(S5PV210_GPH1(2), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH1(2), S3C_GPIO_PULL_NONE);      /* HW PULL_UP */

    /* INT_ONEDRAM_AP_N */
	s3c_gpio_cfgpin(S5PV210_GPH1(3), S3C_GPIO_SFN(0xF));
	s3c_gpio_setpull(S5PV210_GPH1(3), S3C_GPIO_PULL_UP);

#if 0 // CMC_RST must keep previous value when going to sleep.
    /* CMC_RST */
	s3c_gpio_cfgpin(S5PV210_GPH1(4), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV210_GPH1(4), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(S5PV210_GPH1(4), 0);
#endif

    /* SEND_END */
	s3c_gpio_cfgpin(GPIO_EAR_SEND_END, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_EAR_SEND_END, S3C_GPIO_PULL_DOWN);    /* active low */

#if 0 // 220_PMIC_PWRON must keep previous value when going to sleep.
    // LTE_Factory_test }}
    if(lte_factory_test == FALSE)
    {
        /* 220_PMIC_PWRON */
    	s3c_gpio_cfgpin(S5PV210_GPH1(6), S3C_GPIO_INPUT);
    	s3c_gpio_setpull(S5PV210_GPH1(6), S3C_GPIO_PULL_DOWN);
    }
    // LTE_Factory_test }}
#endif

    /* PHONE_ACTIVE(QSC_ACTIVE) */
	s3c_gpio_cfgpin(S5PV210_GPH1(7), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH1(7), S3C_GPIO_PULL_NONE);

#if 0
    /* KEYSCAN[0] */
	s3c_gpio_cfgpin(S5PV210_GPH2(0), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV210_GPH2(0), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(S5PV210_GPH2(0), 0);

    /* KEYSCAN[1] */
	s3c_gpio_cfgpin(S5PV210_GPH2(1), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV210_GPH2(1), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(S5PV210_GPH2(1), 0);

    /* KEYSCAN[2] */
	s3c_gpio_cfgpin(S5PV210_GPH2(2), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV210_GPH2(2), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(S5PV210_GPH2(2), 0);

    /* KEYSCAN[3] */
	s3c_gpio_cfgpin(S5PV210_GPH2(3), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV210_GPH2(3), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(S5PV210_GPH2(3), 0);	// hanapark (Fix: BATT_ID high) 20100422

    /* KEYSCAN[4] */
	s3c_gpio_cfgpin(S5PV210_GPH2(4), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV210_GPH2(4), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(S5PV210_GPH2(4), 0);

    /* KEYSCAN[5] */
	s3c_gpio_cfgpin(S5PV210_GPH2(5), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV210_GPH2(5), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(S5PV210_GPH2(5), 0);
#endif

    /* nPOWER */
	s3c_gpio_cfgpin(S5PV210_GPH2(6), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH2(6), S3C_GPIO_PULL_NONE);      /* HW PULL_UP */

    /* JACK_nINT */
	s3c_gpio_cfgpin(S5PV210_GPH2(7), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH2(7), S3C_GPIO_PULL_NONE);

#if 0
    /* KEYSENSE[0] */
	s3c_gpio_cfgpin(S5PV210_GPH3(0), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH3(0), S3C_GPIO_PULL_UP);
//	s3c_gpio_setpin(S5PV210_GPH3(0), 0);

    /* KEYSENSE[1] */
	s3c_gpio_cfgpin(S5PV210_GPH3(1), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH3(1), S3C_GPIO_PULL_UP);
//	s3c_gpio_setpin(S5PV210_GPH3(1), 0);

    /* KEYSENSE[2] */
	s3c_gpio_cfgpin(S5PV210_GPH3(2), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH3(2), S3C_GPIO_PULL_UP);
//	s3c_gpio_setpin(S5PV210_GPH3(2), 0);

    /* KEYSENSE[3] */
	s3c_gpio_cfgpin(S5PV210_GPH3(3), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH3(3), S3C_GPIO_PULL_UP);
//	s3c_gpio_setpin(S5PV210_GPH3(3), 0);

    /* KEYSENSE[4] */
	s3c_gpio_cfgpin(S5PV210_GPH3(4), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH3(4), S3C_GPIO_PULL_NONE);
	//s3c_gpio_setpin(S5PV210_GPH3(4), 0);

    /* KEYSENSE[5] */
	s3c_gpio_cfgpin(S5PV210_GPH3(5), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH3(5), S3C_GPIO_PULL_DOWN);
	//s3c_gpio_setpin(S5PV210_GPH3(5), 0);

    /* KEYSENSE[6] */
	s3c_gpio_cfgpin(S5PV210_GPH3(6), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH3(6), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(S5PV210_GPH3(6), 0);
#endif

    /* T_FLASH_DETECT */
	s3c_gpio_cfgpin(GPIO_T_FLASH_DETECT, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_T_FLASH_DETECT, S3C_GPIO_PULL_NONE);

    s3c_config_sleep_gpio_table(ARRAY_SIZE(jupiter_sleep_gpio_table), jupiter_sleep_gpio_table);
}
#else
{
#if 0
    /* VOUT_PS */
	s3c_gpio_cfgpin(S5PV210_GPH0(1), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH0(1), S3C_GPIO_PULL_DOWN);
	//s3c_gpio_setpin(S5PV210_GPH0(1), 0);

    /* 3_POLE_ SEND_END */
	s3c_gpio_cfgpin(S5PV210_GPH0(2), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV210_GPH0(2), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(S5PV210_GPH0(2), 0);

    /* WLAN_HOST_WAKE */
	s3c_gpio_cfgpin(S5PV210_GPH0(3), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH0(3), S3C_GPIO_PULL_DOWN);
	//s3c_gpio_setpin(S5PV210_GPH0(3), 0);

    /*  BT_HOST_WAKE */
	s3c_gpio_cfgpin(S5PV210_GPH0(4), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH0(4), S3C_GPIO_PULL_DOWN);
	//s3c_gpio_setpin(S5PV210_GPH0(4), 0);

    /* HALL_SW */
	s3c_gpio_cfgpin(S5PV210_GPH0(5), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH0(5), S3C_GPIO_PULL_DOWN);
	//s3c_gpio_setpin(S5PV210_GPH0(5), 0);

    /* EAR_DET */
	s3c_gpio_cfgpin(S5PV210_GPH0(6), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH0(6), S3C_GPIO_PULL_NONE);
	//s3c_gpio_setpin(S5PV210_GPH0(6), 0);

    /* AP_PMIC_IRQ */
	s3c_gpio_cfgpin(S5PV210_GPH0(7), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH0(7), S3C_GPIO_PULL_NONE);
	//s3c_gpio_setpin(S5PV210_GPH0(0), 0);

#if 0 // TODO:FORTE_USCC_REV00 BUG GPA0(5) --> next rev restore
    /* T_FLASH_DET */
    s3c_gpio_cfgpin(GPIO_T_FLASH_DETECT, S3C_GPIO_OUTPUT);
    s3c_gpio_setpull(GPIO_T_FLASH_DETECT, S3C_GPIO_PULL_DOWN);
#else
	//s3c_gpio_cfgpin(S5PV210_GPH1(0), S3C_GPIO_INPUT);
	s3c_gpio_cfgpin(S5PV210_GPH1(0), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH1(0), S3C_GPIO_PULL_DOWN);
	//s3c_gpio_setpin(S5PV210_GPH1(0), 0);
#endif

    // TODO:YUNG_TEST DPRAM_C111_INT_N (naming miss, Active High)
    /* DPRAM_C111_INT_N */
	s3c_gpio_cfgpin(S5PV210_GPH1(1), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH1(1), S3C_GPIO_PULL_DOWN);
	//s3c_gpio_setpin(S5PV210_GPH1(1), 0);

	/* FUEL_GUAGE */
	s3c_gpio_cfgpin(S5PV210_GPH1(2), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH1(2), S3C_GPIO_PULL_DOWN);
	//s3c_gpio_setpin(S5PV210_GPH1(2), 0);

    /* INT_ONEDRAM_AP_N */
	s3c_gpio_cfgpin(S5PV210_GPH1(3), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH1(3), S3C_GPIO_PULL_NONE);
	//s3c_gpio_setpin(S5PV210_GPH1(3), 0);
#endif

    /* CMC_RST */
	s3c_gpio_cfgpin(S5PV210_GPH1(4), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV210_GPH1(4), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(S5PV210_GPH1(4), 0);

#if 0 // FORTE_USCC_REV01
    /* MASSMEMORY_EN */
	s3c_gpio_cfgpin(GPIO_MASSMEMORY_EN, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_MASSMEMORY_EN, S3C_GPIO_PULL_NONE);
	// s3c_gpio_setpin(S5PV210_GPH1(5),0);
#endif

    /* 220_PMIC_PWRON */
	s3c_gpio_cfgpin(S5PV210_GPH1(6), S3C_GPIO_INPUT);   //suik_Fix
	s3c_gpio_setpull(S5PV210_GPH1(6), S3C_GPIO_PULL_DOWN);
	// s3c_gpio_setpin(S5PV210_GPH1(6), 0);

#if 0//!defined(CONFIG_FORTE_BASE)
    /* PHONE_ACTIVE(QSC_ACTIVE) */
	s3c_gpio_cfgpin(S5PV210_GPH1(7), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH1(7), S3C_GPIO_PULL_NONE); //20100722_inchul.im... input pull none(from HW)
	//s3c_gpio_setpin(S5PV210_GPH1(7), 0);

    /* KEYSCAN[0] */
	s3c_gpio_cfgpin(S5PV210_GPH2(0), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV210_GPH2(0), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(S5PV210_GPH2(0), 0);

    /* KEYSCAN[1] */
	s3c_gpio_cfgpin(S5PV210_GPH2(1), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV210_GPH2(1), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(S5PV210_GPH2(1), 0);

    /* KEYSCAN[2] */
	s3c_gpio_cfgpin(S5PV210_GPH2(2), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV210_GPH2(2), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(S5PV210_GPH2(2), 0);

    /* KEYSCAN[3] */
	s3c_gpio_cfgpin(S5PV210_GPH2(3), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV210_GPH2(3), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(S5PV210_GPH2(3), 0);	// hanapark (Fix: BATT_ID high) 20100422

    /* KEYSCAN[4] */
	s3c_gpio_cfgpin(S5PV210_GPH2(4), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV210_GPH2(4), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(S5PV210_GPH2(4), 0);

    /* KEYSCAN[5] */
	s3c_gpio_cfgpin(S5PV210_GPH2(5), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV210_GPH2(5), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(S5PV210_GPH2(5), 0);

    /* KEYSCAN[6] */
	s3c_gpio_cfgpin(S5PV210_GPH2(6), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV210_GPH2(6), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(S5PV210_GPH2(6), 0);

    /* KEYSENSE[0] */
	s3c_gpio_cfgpin(S5PV210_GPH2(7), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV210_GPH2(7), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(S5PV210_GPH2(7), 0);

    /* KEYSENSE[1] */
	s3c_gpio_cfgpin(S5PV210_GPH3(0), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH3(0), S3C_GPIO_PULL_UP);
//	s3c_gpio_setpin(S5PV210_GPH3(0), 0);

    /* KEYSENSE[2] */
	s3c_gpio_cfgpin(S5PV210_GPH3(1), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH3(1), S3C_GPIO_PULL_UP);
//	s3c_gpio_setpin(S5PV210_GPH3(1), 0);

    /* KEYSENSE[3] */
	s3c_gpio_cfgpin(S5PV210_GPH3(2), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH3(2), S3C_GPIO_PULL_UP);
//	s3c_gpio_setpin(S5PV210_GPH3(2), 0);

    /* KEYSENSE[4] */
	s3c_gpio_cfgpin(S5PV210_GPH3(3), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH3(3), S3C_GPIO_PULL_UP);
//	s3c_gpio_setpin(S5PV210_GPH3(3), 0);

    /* KEYSENSE[5] */
	s3c_gpio_cfgpin(S5PV210_GPH3(4), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH3(4), S3C_GPIO_PULL_NONE);
	//s3c_gpio_setpin(S5PV210_GPH3(4), 0);

    /* KEYSENSE[6] */
	s3c_gpio_cfgpin(S5PV210_GPH3(5), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH3(5), S3C_GPIO_PULL_DOWN);
	//s3c_gpio_setpin(S5PV210_GPH3(5), 0);

    /* KEYSENSE[7] */
	s3c_gpio_cfgpin(S5PV210_GPH3(6), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH3(6), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(S5PV210_GPH3(6), 0);
#endif

#if 0 // TODO:FORTE_USCC_REV01
    /* CP_RST */
	s3c_gpio_cfgpin(GPIO_QSC_RESET_N, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_QSC_RESET_N, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(GPIO_QSC_RESET_N, 1);
#endif

    s3c_config_sleep_gpio_table(ARRAY_SIZE(jupiter_sleep_gpio_table), jupiter_sleep_gpio_table);
}
#endif

EXPORT_SYMBOL(s3c_config_sleep_gpio);

void s3c_config_gpio_alive_table(int array_size, int (*gpio_table)[4])
{
	u32 i, gpio;

	for (i = 0; i < array_size; i++) {
		gpio = gpio_table[i][0];
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(gpio_table[i][1]));
		s3c_gpio_setpull(gpio, gpio_table[i][3]);
		if (gpio_table[i][2] != GPIO_LEVEL_NONE)
			gpio_set_value(gpio, gpio_table[i][2]);
	}
}

static struct platform_device *smdkc110_devices[] __initdata = {
#ifdef CONFIG_RTC_DRV_S3C
	&s5p_device_rtc,
#endif
	&s3c_device_keypad,

#ifdef CONFIG_REGULATOR_MAX8998
	&s3c_device_8998consumer,
#endif

#ifdef CONFIG_MTD_ONENAND
	&s3c_device_onenand,
#endif
#ifdef CONFIG_FB_S3C
	&s3c_device_fb,
#endif
#ifdef CONFIG_TOUCHSCREEN_S3C
	&s3c_device_ts,
#endif
#ifdef CONFIG_S5PV210_ADCTS
	&s3c_device_adcts,
#endif
#ifdef CONFIG_DM9000
	&s5p_device_dm9000,
#endif
#ifdef CONFIG_S3C2410_WATCHDOG
	&s3c_device_wdt,
#endif
#if defined(CONFIG_BLK_DEV_IDE_S3C)
	&s3c_device_cfcon,
#endif

#if defined(CONFIG_FB_S3C_TL2796) || defined(CONFIG_FB_S3C_S6D16A0X)
    &s3c_device_spi_gpio,
#endif

#ifdef CONFIG_HAVE_PWM
	&s3c_device_timer[0],
	&s3c_device_timer[1],
	&s3c_device_timer[2],
	&s3c_device_timer[3],
#endif

#ifdef	CONFIG_S5P_ADC
	&s3c_device_adc,
#endif
#ifdef CONFIG_SND_S3C24XX_SOC
	&s3c64xx_device_iis0,
#endif
#ifdef CONFIG_VIDEO_FIMC
	&s3c_device_fimc0,
	&s3c_device_fimc1,
	&s3c_device_fimc2,
	&s3c_device_csis,
	&s3c_device_ipc,
#endif
    &sec_device_jack,

#ifdef CONFIG_VIDEO_MFC50
	&s3c_device_mfc,
#endif

#ifdef CONFIG_VIDEO_JPEG_V2
	&s3c_device_jpeg,
#endif

#ifdef CONFIG_VIDEO_ROTATOR
	&s5p_device_rotator,
#endif


	&s3c_device_i2c0,  /* camera */
	&s3c_device_i2c1,  /* FSA, Accel, Compass */
	&s3c_device_i2c2,  /* TSP */
    &s3c_device_i2c4,  /* WM8994 */
    &s3c_device_i2c5,  /* accel sensor */
	&s3c_device_i2c6,  /* PMIC */
#if 0 // TODO:FORTE
	&s3c_device_i2c7,  /* For FSA micro USB switch */
	&s3c_device_i2c8,  /* For SI4709 */
#endif
    &s3c_device_i2c9,  /* fuel gague */
#if 0 // TODO:FORTE
	&s3c_device_i2c10, /* tsp */
#endif
    &s3c_device_i2c11, /* optical sensor */
	&s3c_device_i2c12, /* backlight */

#ifdef CONFIG_USB
	&s3c_device_usb_ehci,
	&s3c_device_usb_ohci,
#endif
#ifdef CONFIG_USB_GADGET
	&s3c_device_usbgadget,
#endif
#ifdef CONFIG_USB_ANDROID
	&s3c_device_android_usb,
	&s3c_device_usb_mass_storage,
#endif

#ifdef CONFIG_SAMSUNG_PHONE_TTY_MODULE // kt.hur
	&sec_device_dpram,
#endif

#ifdef CONFIG_S3C_DEV_HSMMC
	&s3c_device_hsmmc0,
#endif
#ifdef CONFIG_S3C_DEV_HSMMC1
	&s3c_device_hsmmc1,
#endif
#ifdef CONFIG_S3C_DEV_HSMMC2
	&s3c_device_hsmmc2,
#endif
#ifdef CONFIG_S3C_DEV_HSMMC3
	&s3c_device_hsmmc3,
#endif

#ifdef CONFIG_VIDEO_TV20
	&s5p_device_tvout,
	&s5p_device_cec,
	&s5p_device_hpd,
#endif

#ifdef CONFIG_ANDROID_PMEM
	&pmem_device,
	&pmem_gpu1_device,
	&pmem_adsp_device,
#endif
	&sec_device_battery,
#ifdef CONFIG_VIDEO_G2D
	&s5p_device_g2d,
#endif
	&sec_device_rfkill,
	&sec_device_btsleep,
#if defined(CONFIG_TOUCHSCREEN_QT602240)
	&s3c_device_qtts,
#endif
	&opt_gp2a
};

unsigned int HWREV=0;
EXPORT_SYMBOL(HWREV);

static int read_hwversion(void)
{
#if 1 /* FORTE not support */
    return 0x00 ;   // TODO:FORTE RE-CHECK IT!
#else
    int err;
    int hwver = -1;
        int hwver_0 = -1;
        int hwver_1 = -1;
        int hwver_2 = -1;

        err = gpio_request(S5PV210_GPJ0(2), "HWREV_MODE0");

        if (err) {
                printk(KERN_ERR "failed to request GPJ0(2) for "
                        "HWREV_MODE0\n");
                return err;
        }
        err = gpio_request(S5PV210_GPJ0(3), "HWREV_MODE1");

        if (err) {
                printk(KERN_ERR "failed to request GPJ0(3) for "
                        "HWREV_MODE1\n");
                return err;
        }
        err = gpio_request(S5PV210_GPJ0(4), "HWREV_MODE2");

        if (err) {
                printk(KERN_ERR "failed to request GPJ0(4) for "
                        "HWREV_MODE2\n");
                return err;
        }

        gpio_direction_input(S5PV210_GPJ0(2));
        gpio_direction_input(S5PV210_GPJ0(3));
        gpio_direction_input(S5PV210_GPJ0(4));

        hwver_0 = gpio_get_value(S5PV210_GPJ0(2));
        hwver_1 = gpio_get_value(S5PV210_GPJ0(3));
        hwver_2 = gpio_get_value(S5PV210_GPJ0(4));

        gpio_free(S5PV210_GPJ0(2));
        gpio_free(S5PV210_GPJ0(3));
        gpio_free(S5PV210_GPJ0(4));

	if((hwver_0 == 0)&&(hwver_1 == 1)&&(hwver_2 == 0)){
                hwver = 2;
                printk("+++++++++[I9000 Rev0.1 board]++++++++ hwver_0: %d, hwver_1: %d, hwver_2: %d\n", hwver_0, hwver_1, hwver_2);
        }
        else if((hwver_0 == 1)&&(hwver_1 == 0)&&(hwver_2 == 1)){
                hwver = 2;
                printk("+++++++++[B5 board]++++++++ hwver_0: %d, hwver_1: %d, hwver_2: %d\n", hwver_0, hwver_1, hwver_2);
        }
        else if((hwver_0 == 0)&&(hwver_1 == 1)&&(hwver_2 == 1)){
                hwver = 2;
                printk("+++++++++[ARIES B5 board]++++++++ hwver_0: %d, hwver_1: %d, hwver_2: %d\n", hwver_0, hwver_1, hwver_2);
        }
        else{
                hwver = 0;
                //printk("+++++++++[B2, B3 board]++++++++ hwver_0: %d, hwver_1: %d, hwver_2: %d\n", hwver_0, hwver_1, hwver_2);
        }

        return hwver;
#endif
}


static void __init smdkc110_fixup(struct machine_desc *desc,
                                       struct tag *tags, char **cmdline,
                                       struct meminfo *mi)
{

	mi->bank[0].start = 0x30000000;
	mi->bank[0].size = 80 * SZ_1M;
	mi->bank[0].node = 0;

	mi->bank[1].start = 0x40000000;
        //mi->bank[1].size = 256 * SZ_1M;
        mi->bank[1].size = 256 * SZ_1M; /* this value wil be changed to 256MB */
        mi->bank[1].node = 1;

	mi->nr_banks = 2;

        mi->bank[2].start = 0x50000000;
        mi->bank[2].size = 128 * SZ_1M;
        mi->bank[2].node = 2;
        mi->nr_banks = 3;

}

static void __init smdkc110_map_io(void)
{
	s5p_init_io(NULL, 0, S5P_VA_CHIPID);
	s3c24xx_init_clocks(24000000);
	s5pv210_gpiolib_init();
	s3c24xx_init_uarts(smdkv210_uartcfgs, ARRAY_SIZE(smdkv210_uartcfgs));
	s5pv210_reserve_bootmem();

#ifdef CONFIG_MTD_ONENAND
	s3c_device_onenand.name = "s5pc110-onenand";
#endif
}

#ifdef CONFIG_S3C_SAMSUNG_PMEM
static void __init s3c_pmem_set_platdata(void)
{
	pmem_pdata.start = s3c_get_media_memory_bank(S3C_MDEV_PMEM, 1);
	pmem_pdata.size = s3c_get_media_memsize_bank(S3C_MDEV_PMEM, 1);
}
#endif

#ifdef CONFIG_FB_S3C_LTE480WV
static struct s3c_platform_fb lte480wv_fb_data __initdata = {
	.hw_ver	= 0x62,
	.nr_wins = 5,
	.default_win = CONFIG_FB_S3C_DEFAULT_WINDOW,
	.swap = FB_SWAP_WORD | FB_SWAP_HWORD,
};
#endif
/* this function are used to detect s5pc110 chip version temporally */

int s5pc110_version ;

void _hw_version_check(void)
{
	void __iomem * phy_address ;
	int temp;

	phy_address = ioremap (0x40,1);

	temp = __raw_readl(phy_address);


	if (temp == 0xE59F010C)
	{
		s5pc110_version = 0;
	}
	else
	{
		s5pc110_version=1 ;
	}
	printk("S5PC110 Hardware version : EVT%d \n",s5pc110_version);

	iounmap(phy_address);
}

/* Temporally used
 * return value 0 -> EVT 0
 * value 1 -> evt 1
 */

int hw_version_check(void)
{
	return s5pc110_version ;
}
EXPORT_SYMBOL(hw_version_check);

/* touch screen device init */
static void __init qt_touch_init(void)
{
    int gpio, irq;

        /* qt602240 TSP */
    qt602240_p1_platform_data.blen = 0x1;
    qt602240_p1_platform_data.threshold = 0x13;
    qt602240_p1_platform_data.orient = QT602240_VERTICAL_FLIP;

    gpio = GPIO_TOUCH_EN; // forte_temp  S5PV210_GPG3(6);                     /* XMMC3DATA_3 */
    gpio_request(gpio, "TOUCH_EN");
    s3c_gpio_cfgpin(gpio, S3C_GPIO_OUTPUT);
    gpio_direction_output(gpio, 1);
    gpio_free(gpio);

    gpio = GPIO_TOUCH_INT; // forte_temp   S5PV210_GPJ0(5);                             /* XMSMADDR_5 */
    gpio_request(gpio, "TOUCH_INT");
    s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(0xf));
    s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP);
    irq = gpio_to_irq(gpio);
    gpio_free(gpio);

    i2c_devs2[0].irq = irq;//PREVENT fixed
}


extern void set_pmic_gpio(void);
static void jupiter_init_gpio(void)
{
    s3c_config_gpio_table(ARRAY_SIZE(jupiter_gpio_table), jupiter_gpio_table);
    //s3c_config_sleep_gpio_table(ARRAY_SIZE(jupiter_sleep_gpio_table), jupiter_sleep_gpio_table);

	/*Adding pmic gpio(GPH3, GPH4, GPH5) initialisation*/
	set_pmic_gpio();
}

static int arise_notifier_call(struct notifier_block *this, unsigned long code,
			       void *_cmd)
{
	int mode = REBOOT_MODE_NONE;

	if ((code == SYS_RESTART) && _cmd) {
		if (!strcmp((char *)_cmd, "arm11_fota"))
			mode = REBOOT_MODE_ARM11_FOTA;
		else if (!strcmp((char *)_cmd, "arm9_fota"))
			mode = REBOOT_MODE_ARM9_FOTA;
		else if (!strcmp((char *)_cmd, "recovery"))
		 {
		      mode = REBOOT_MODE_RECOVERY;
			  #ifdef CONFIG_KERNEL_DEBUG_SEC // Added for Froyo Atlas
			    //etinum.factory.reboot disable uart msg in bootloader for
			    // factory reset 2nd ack
			    kernel_sec_set_upload_cause(BLK_UART_MSG_FOR_FACTRST_2ND_ACK);
              #endif
		 }
		else if (!strcmp((char *)_cmd, "download"))
			mode = REBOOT_MODE_DOWNLOAD;

        #ifdef CONFIG_KERNEL_DEBUG_SEC
		//etinum.factory.reboot disable uart msg in bootloader for
		// factory reset 2nd ack
		// Added for Froyo Atlas
		else if (!strcmp((char *)_cmd, "factory_reboot")) {
			    mode = REBOOT_MODE_NONE;
			    kernel_sec_set_upload_cause(BLK_UART_MSG_FOR_FACTRST_2ND_ACK);
	            }
            #endif
	}

	if (code != SYS_POWER_OFF) {
		if (sec_set_param_value) {
			sec_set_param_value(__REBOOT_MODE, &mode);
		}
	}

	return NOTIFY_DONE;
}

static struct notifier_block arise_reboot_notifier = {
	.notifier_call = arise_notifier_call,
};

static void __init smdkc110_machine_init(void)
{
	/* Find out S5PC110 chip version */
	_hw_version_check();

	pm_power_off = smdkc110_power_off ;

#if 0 // TODO:FORTE
	s3c_gpio_cfgpin(GPIO_HWREV_MODE0, S3C_GPIO_INPUT);
	s3c_gpio_setpull( GPIO_HWREV_MODE0, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_HWREV_MODE1, S3C_GPIO_INPUT);
	s3c_gpio_setpull( GPIO_HWREV_MODE1, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_HWREV_MODE2, S3C_GPIO_INPUT);
	s3c_gpio_setpull( GPIO_HWREV_MODE2, S3C_GPIO_PULL_NONE);
	HWREV = gpio_get_value(GPIO_HWREV_MODE0);
	HWREV = HWREV | (gpio_get_value(GPIO_HWREV_MODE1) <<1);
	HWREV = HWREV | (gpio_get_value(GPIO_HWREV_MODE2) <<2);
	s3c_gpio_cfgpin(GPIO_HWREV_MODE3, S3C_GPIO_INPUT);
	s3c_gpio_setpull( GPIO_HWREV_MODE3, S3C_GPIO_PULL_NONE);
	HWREV = HWREV | (gpio_get_value(GPIO_HWREV_MODE3) <<3);
#else
    HWREV = 0x00 ; // TODO:FORTE
#endif
	printk("HWREV is 0x%x\n", HWREV);

	/*initialise the gpio's*/
	jupiter_init_gpio();

	/* OneNAND */
#ifdef CONFIG_MTD_ONENAND
	//s3c_device_onenand.dev.platform_data = &s5p_onenand_data;
#endif


	qt_touch_init();

#ifdef CONFIG_DM9000
	smdkv210_dm9000_set();
#endif

#ifdef CONFIG_ANDROID_PMEM
	android_pmem_set_platdata();
#endif
#if 0 // TODO:FORTE
	{
		int tint = GPIO_TOUCH_INT;
		s3c_gpio_cfgpin(tint, S3C_GPIO_INPUT);
		s3c_gpio_setpull(tint, S3C_GPIO_PULL_UP);
	}
#endif

	/* i2c */
	s3c_i2c0_set_platdata(NULL);
	s3c_i2c1_set_platdata(NULL);
	s3c_i2c2_set_platdata(NULL);
	i2c_register_board_info(0, i2c_devs0, ARRAY_SIZE(i2c_devs0));
	i2c_register_board_info(1, i2c_devs1, ARRAY_SIZE(i2c_devs1));
	i2c_register_board_info(2, i2c_devs2, ARRAY_SIZE(i2c_devs2));
	i2c_register_board_info(4, i2c_devs4, ARRAY_SIZE(i2c_devs4));
	i2c_register_board_info(5, i2c_devs5, ARRAY_SIZE(i2c_devs5));
	i2c_register_board_info(6, i2c_devs6, ARRAY_SIZE(i2c_devs6));
	// TODO:FORTEi2c_register_board_info(7, i2c_devs7, ARRAY_SIZE(i2c_devs7)); /* for fsa9480 */
	// TODO:FORTEi2c_register_board_info(8, i2c_devs8, ARRAY_SIZE(i2c_devs8)); /* for Si4709 */
	i2c_register_board_info(9, i2c_devs9, ARRAY_SIZE(i2c_devs9));
    // TODO:FORTE	i2c_register_board_info(10, i2c_devs10, ARRAY_SIZE(i2c_devs10)); /* for touchkey */
	i2c_register_board_info(11, i2c_devs11, ARRAY_SIZE(i2c_devs11)); /* optical sensor */
	i2c_register_board_info(11, i2c_devs12, ARRAY_SIZE(i2c_devs12)); /* backlight */

#ifdef CONFIG_FB_S3C_LTE480WV
	s3cfb_set_platdata(&lte480wv_fb_data);
#endif

#if defined(CONFIG_BLK_DEV_IDE_S3C)
	s3c_ide_set_platdata(&smdkv210_ide_pdata);
#endif

#if defined(CONFIG_TOUCHSCREEN_S3C)
	s3c_ts_set_platdata(&s3c_ts_platform);
#endif

#ifdef CONFIG_FB_S3C_TL2796
    spi_register_board_info(spi_board_info, ARRAY_SIZE(spi_board_info));
    s3cfb_set_platdata(&tl2796_data);
#elif   defined(CONFIG_FB_S3C_S6D16A0X)
    spi_register_board_info(spi_board_info, ARRAY_SIZE(spi_board_info));
    s3cfb_set_platdata(&s6d16a0x_data);
#endif

#if defined(CONFIG_S5PV210_ADCTS)
	s3c_adcts_set_platdata(&s3c_adcts_cfgs);
#endif

#if defined(CONFIG_S5P_ADC)
	s3c_adc_set_platdata(&s3c_adc_platform);
#endif

#if defined(CONFIG_PM)
	s3c_pm_init();
//	s5pc11x_pm_init();
#endif

	/* fimc */
	s3c_fimc0_set_platdata(&fimc_plat);
	s3c_fimc1_set_platdata(&fimc_plat);
	s3c_fimc2_set_platdata(&fimc_plat);
	s3c_csis_set_platdata(NULL);

#ifdef CONFIG_VIDEO_MFC50
	/* mfc */
	s3c_mfc_set_platdata(NULL);
#endif

#ifdef CONFIG_S3C_DEV_HSMMC
	s5pv210_default_sdhci0();
#endif
#ifdef CONFIG_S3C_DEV_HSMMC1
	s5pv210_default_sdhci1();
#endif
#ifdef CONFIG_S3C_DEV_HSMMC2
	s5pv210_default_sdhci2();
#endif
#ifdef CONFIG_S3C_DEV_HSMMC3
	s5pv210_default_sdhci3();
#endif
#ifdef CONFIG_S5PV210_SETUP_SDHCI
	s3c_sdhci_set_platdata();
#endif
	platform_add_devices(smdkc110_devices, ARRAY_SIZE(smdkc110_devices));
#if defined(CONFIG_SAMSUNG_PHONE_SVNET) || defined(CONFIG_SAMSUNG_PHONE_SVNET_MODULE)
	platform_device_register(&modemctl);
	platform_device_register(&onedram);
#endif
#if 0 // TODO:FORTE need not defined(CONFIG_HAVE_PWM)
	smdk_backlight_register();
#endif

#if 0 // TODO:FORTE
	s3c_gpio_cfgpin( AP_I2C_SCL_28V, 1 );
	s3c_gpio_setpull( AP_I2C_SCL_28V, S3C_GPIO_PULL_UP);
	s3c_gpio_cfgpin( AP_I2C_SDA_28V, 1 );
	s3c_gpio_setpull( AP_I2C_SDA_28V, S3C_GPIO_PULL_UP);
#endif

	register_reboot_notifier(&arise_reboot_notifier);

	jupiter_switch_init();

	BUG_ON(!sec_class);
	gps_dev = device_create(sec_class, NULL, 0, NULL, "gps");
	if (IS_ERR(gps_dev))
		pr_err("Failed to create device(gps)!\n");
 
#if !defined(FORTE_PRODUCT)
	if (gpio_is_valid(GPIO_MSENSE_nRST)) {
               if (gpio_request(GPIO_MSENSE_nRST, "GPB"))
                        printk(KERN_ERR "Failed to request GPIO_MSENSE_nRST! \n");
		gpio_direction_output(GPIO_MSENSE_nRST, 1);
	}
	gpio_free(GPIO_MSENSE_nRST);
#endif

	#ifdef RECOVERY_SYSFS
	recovery_class_init();
	#endif
}

#if defined(CONFIG_USB_SUPPORT) || defined(CONFIG_VIDEO_TV20)
static int ldo38_onoff_state = 0;
#define LDO38_USB_BIT    (0x1)
#define LDO38_TVOUT_BIT  (0x2)
static DEFINE_MUTEX(ldo38_mutex);

static int ldo38_control(int module, bool onoff)
{
    mutex_lock(&ldo38_mutex);
    if (onoff)
    {
        if (!ldo38_onoff_state)
        {
            Set_MAX8998_PM_REG(ELDO3, 1);
            msleep(1);
            Set_MAX8998_PM_REG(ELDO8, 1);
            msleep(1);
        }
        printk(KERN_INFO "%s : turn ON LDO3 and LDO8 (cur_stat=%d, req=%d)\n",__func__,ldo38_onoff_state,module);
        ldo38_onoff_state |= module;
    }
    else
    {
        printk(KERN_INFO "%s : turn OFF LDO3 and LDO8 (cur_stat=%d, req=%d)\n",__func__,ldo38_onoff_state,module);
        ldo38_onoff_state &= ~module;
        if (!ldo38_onoff_state)
        {
            Set_MAX8998_PM_REG(ELDO8, 0);
            msleep(1);
            Set_MAX8998_PM_REG(ELDO3, 0);
        }
    }
    mutex_unlock(&ldo38_mutex);
}

int ldo38_control_by_usb(bool onoff)
{
    ldo38_control(LDO38_USB_BIT, onoff);
}

#if defined(CONFIG_VIDEO_TV20)
int ldo38_control_by_tvout(bool onoff)
{
    ldo38_control(LDO38_TVOUT_BIT, onoff);
}
#endif  /* VIDEO_TV20 */

#endif

#ifdef CONFIG_USB_SUPPORT
/* Initializes OTG Phy. */
void otg_phy_init(void)
{
	__raw_writel(__raw_readl(S5P_USB_PHY_CONTROL)
		|(0x1<<0), S5P_USB_PHY_CONTROL); /*USB PHY0 Enable */
	__raw_writel((__raw_readl(S3C_USBOTG_PHYPWR)
		&~(0x3<<3)&~(0x1<<0))|(0x1<<5), S3C_USBOTG_PHYPWR);
	__raw_writel((__raw_readl(S3C_USBOTG_PHYCLK)
		&~(0x5<<2))|(0x3<<0), S3C_USBOTG_PHYCLK);
	__raw_writel((__raw_readl(S3C_USBOTG_RSTCON)
		&~(0x3<<1))|(0x1<<0), S3C_USBOTG_RSTCON);
	udelay(10);
	__raw_writel(__raw_readl(S3C_USBOTG_RSTCON)
		&~(0x7<<0), S3C_USBOTG_RSTCON);
	udelay(10);
}
EXPORT_SYMBOL(otg_phy_init);

/* USB Control request data struct must be located here for DMA transfer */
struct usb_ctrlrequest usb_ctrl __attribute__((aligned(64)));
EXPORT_SYMBOL(usb_ctrl);

/* OTG PHY Power Off */
void otg_phy_off(void)
{
	__raw_writel(__raw_readl(S3C_USBOTG_PHYPWR)
		|(0x3<<3), S3C_USBOTG_PHYPWR);
	__raw_writel(__raw_readl(S5P_USB_PHY_CONTROL)
		&~(1<<0), S5P_USB_PHY_CONTROL);
}
EXPORT_SYMBOL(otg_phy_off);

void usb_host_phy_init(void)
{
	struct clk *otg_clk;

	otg_clk = clk_get(NULL, "usbotg");
	clk_enable(otg_clk);

	if (readl(S5P_USB_PHY_CONTROL) & (0x1<<1))
		return;

	__raw_writel(__raw_readl(S5P_USB_PHY_CONTROL)
		|(0x1<<1), S5P_USB_PHY_CONTROL);
	__raw_writel((__raw_readl(S3C_USBOTG_PHYPWR)
		&~(0x1<<7)&~(0x1<<6))|(0x1<<8)|(0x1<<5), S3C_USBOTG_PHYPWR);
	__raw_writel((__raw_readl(S3C_USBOTG_PHYCLK)
		&~(0x1<<7))|(0x3<<0), S3C_USBOTG_PHYCLK);
	__raw_writel((__raw_readl(S3C_USBOTG_RSTCON))
		|(0x1<<4)|(0x1<<3), S3C_USBOTG_RSTCON);
	__raw_writel(__raw_readl(S3C_USBOTG_RSTCON)
		&~(0x1<<4)&~(0x1<<3), S3C_USBOTG_RSTCON);
}
EXPORT_SYMBOL(usb_host_phy_init);

void usb_host_phy_off(void)
{
	__raw_writel(__raw_readl(S3C_USBOTG_PHYPWR)
		|(0x1<<7)|(0x1<<6), S3C_USBOTG_PHYPWR);
	__raw_writel(__raw_readl(S5P_USB_PHY_CONTROL)
		&~(1<<1), S5P_USB_PHY_CONTROL);
}
EXPORT_SYMBOL(usb_host_phy_off);
#endif

#if defined(CONFIG_KEYPAD_S3C) || defined(CONFIG_KEYPAD_S3C_MODULE)
#if defined(CONFIG_KEYPAD_S3C_MSM)
void s3c_setup_keypad_cfg_gpio(void)
{
	unsigned int gpio;
	unsigned int end;

	/* gpio setting for KP_COL0 */
	s3c_gpio_cfgpin(S5PV210_GPJ1(5), S3C_GPIO_SFN(3));
	s3c_gpio_setpull(S5PV210_GPJ1(5), S3C_GPIO_PULL_NONE);

	/* gpio setting for KP_COL1 ~ KP_COL7 and KP_ROW0 */
	end = S5PV210_GPJ2(8);
	for (gpio = S5PV210_GPJ2(0); gpio < end; gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	}

	/* gpio setting for KP_ROW1 ~ KP_ROW8 */
	end = S5PV210_GPJ3(8);
	for (gpio = S5PV210_GPJ3(0); gpio < end; gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	}

	/* gpio setting for KP_ROW9 ~ KP_ROW13 */
	end = S5PV210_GPJ4(5);
	for (gpio = S5PV210_GPJ4(0); gpio < end; gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	}
}
#else
void s3c_setup_keypad_cfg_gpio(int rows, int columns)
{
	unsigned int gpio;
	unsigned int end;

    /* KEYPAD SENSE/ROW */
	end = S5PV210_GPH3(rows);

	/* Set all the necessary GPH2 pins to special-function 0 */
	for (gpio = S5PV210_GPH3(0); gpio < end; gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	}

    printk(KERN_ERR "%s columns = %d\n", __FUNCTION__, columns) ;
    /* KEYPAD SCAN/COLUMN */
    columns -= 2 ;  /* except scan[6~7] */
	end = S5PV210_GPH2(columns);

	/* Set all the necessary GPK pins to special-function 0 */
	for (gpio = S5PV210_GPH2(0); gpio < end; gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	}

    s3c_gpio_cfgpin(GPIO_KEYSCAN6, S3C_GPIO_OUTPUT) ;
	s3c_gpio_setpull(GPIO_KEYSCAN6, S3C_GPIO_PULL_NONE);
    s3c_gpio_cfgpin(GPIO_KEYSCAN7, S3C_GPIO_OUTPUT) ;
	s3c_gpio_setpull(GPIO_KEYSCAN7, S3C_GPIO_PULL_NONE);
}
#endif /* if defined(CONFIG_KEYPAD_S3C_MSM)*/
EXPORT_SYMBOL(s3c_setup_keypad_cfg_gpio);
#endif

MACHINE_START(SMDKC110, "SMDKC110")
	/* Maintainer: Kukjin Kim <kgene.kim@samsung.com> */
	.phys_io	= S3C_PA_UART & 0xfff00000,
	.io_pg_offst	= (((u32)S3C_VA_UART) >> 18) & 0xfffc,
	.boot_params	= S5P_PA_SDRAM + 0x100,
	.fixup		= smdkc110_fixup,
	.init_irq	= s5pv210_init_irq,
	.map_io		= smdkc110_map_io,
	.init_machine	= smdkc110_machine_init,
	.timer		= &s5p_systimer,
MACHINE_END

void s3c_setup_uart_cfg_gpio(unsigned char port)
{
	switch(port)
	{
	case 0:
		s3c_gpio_cfgpin(GPIO_BT_RXD, S3C_GPIO_SFN(GPIO_BT_RXD_AF));
		s3c_gpio_setpull(GPIO_BT_RXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_BT_TXD, S3C_GPIO_SFN(GPIO_BT_TXD_AF));
		s3c_gpio_setpull(GPIO_BT_TXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_BT_CTS, S3C_GPIO_SFN(GPIO_BT_CTS_AF));
		s3c_gpio_setpull(GPIO_BT_CTS, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_BT_RTS, S3C_GPIO_SFN(GPIO_BT_RTS_AF));
		s3c_gpio_setpull(GPIO_BT_RTS, S3C_GPIO_PULL_NONE);
/*
		s3c_gpio_slp_cfgpin(GPIO_BT_RXD, S3C_GPIO_SLP_PREV);
		s3c_gpio_slp_setpull_updown(GPIO_BT_RXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_slp_cfgpin(GPIO_BT_TXD, S3C_GPIO_SLP_PREV);
		s3c_gpio_slp_setpull_updown(GPIO_BT_TXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_slp_cfgpin(GPIO_BT_CTS, S3C_GPIO_SLP_PREV);
		s3c_gpio_slp_setpull_updown(GPIO_BT_CTS, S3C_GPIO_PULL_NONE);
		s3c_gpio_slp_cfgpin(GPIO_BT_RTS, S3C_GPIO_SLP_PREV);
		s3c_gpio_slp_setpull_updown(GPIO_BT_RTS, S3C_GPIO_PULL_NONE);
*/
		break;
	case 1:
    #if 0 // TODO:FORTE
		s3c_gpio_cfgpin(GPIO_GPS_RXD, S3C_GPIO_SFN(GPIO_GPS_RXD_AF));
		s3c_gpio_setpull(GPIO_GPS_RXD, S3C_GPIO_PULL_UP);
		s3c_gpio_cfgpin(GPIO_GPS_TXD, S3C_GPIO_SFN(GPIO_GPS_TXD_AF));
		s3c_gpio_setpull(GPIO_GPS_TXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_GPS_CTS, S3C_GPIO_SFN(GPIO_GPS_CTS_AF));
		s3c_gpio_setpull(GPIO_GPS_CTS, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_GPS_RTS, S3C_GPIO_SFN(GPIO_GPS_RTS_AF));
		s3c_gpio_setpull(GPIO_GPS_RTS, S3C_GPIO_PULL_NONE);
    #endif
		break;
	case 2:
		s3c_gpio_cfgpin(GPIO_AP_RXD, S3C_GPIO_SFN(GPIO_AP_RXD_AF));
		s3c_gpio_setpull(GPIO_AP_RXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_AP_TXD, S3C_GPIO_SFN(GPIO_AP_TXD_AF));
		s3c_gpio_setpull(GPIO_AP_TXD, S3C_GPIO_PULL_NONE);
		break;
	case 3:
    #if 0 // TODO:FORTE
		s3c_gpio_cfgpin(GPIO_FLM_RXD, S3C_GPIO_SFN(GPIO_FLM_RXD_AF));
		s3c_gpio_setpull(GPIO_FLM_RXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_FLM_TXD, S3C_GPIO_SFN(GPIO_FLM_TXD_AF));
		s3c_gpio_setpull(GPIO_FLM_TXD, S3C_GPIO_PULL_NONE);
    #endif
		break;
	default:
		break;
	}
}

EXPORT_SYMBOL(s3c_setup_uart_cfg_gpio);


/* << add uart gpio config - end */
/****************************/

static unsigned long wlan_reglock_flags = 0;
static spinlock_t wlan_reglock = SPIN_LOCK_UNLOCKED;

static unsigned int wlan_gpio_table[][4] = {
	{GPIO_WLAN_nRST, GPIO_WLAN_nRST_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_HOST_WAKE, GPIO_WLAN_HOST_WAKE_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN},
	// 2010.11.24 Fixed BT searching fail issue after wifi on
        //{GPIO_WLAN_BT_EN, GPIO_WLAN_BT_EN_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
};

static unsigned int wlan_sdio_on_table[][4] = {
	{GPIO_WLAN_SDIO_CLK, GPIO_WLAN_SDIO_CLK_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_CMD, GPIO_WLAN_SDIO_CMD_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D0, GPIO_WLAN_SDIO_D0_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D1, GPIO_WLAN_SDIO_D1_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D2, GPIO_WLAN_SDIO_D2_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D3, GPIO_WLAN_SDIO_D3_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
};

static unsigned int wlan_sdio_off_table[][4] = {
	{GPIO_WLAN_SDIO_CLK, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_CMD, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D0, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D1, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D2, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D3, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
};

void wlan_setup_power(int on, int flag)
{
	printk(/*KERN_INFO*/ "%s %s", __func__, on ? "on" : "down");
	if (flag != 1) {
		printk(/*KERN_DEBUG*/ "(on=%d, flag=%d)\n", on, flag);
//For Starting/Stopping Tethering service
#if 1
		if (on)
			gpio_set_value(GPIO_WLAN_nRST, GPIO_LEVEL_HIGH);
		else
			gpio_set_value(GPIO_WLAN_nRST, GPIO_LEVEL_LOW);
#endif
		return;
	}
	printk(/*KERN_INFO*/ " --enter\n");

	if (on) {
		s3c_config_gpio_alive_table(ARRAY_SIZE(wlan_gpio_table), wlan_gpio_table);
		s3c_config_gpio_alive_table(ARRAY_SIZE(wlan_sdio_on_table), wlan_sdio_on_table);

		/* PROTECT this check under spinlock.. No other thread should be touching
		 * GPIO_BT_REG_ON at this time.. If BT is operational, don't touch it. */
		spin_lock_irqsave(&wlan_reglock, wlan_reglock_flags);
		/* need delay between v_bat & reg_on for 2 cycle @ 38.4MHz */
		udelay(5);

		gpio_set_value(GPIO_WLAN_BT_EN, GPIO_LEVEL_HIGH);
		s3c_gpio_slp_cfgpin(GPIO_WLAN_BT_EN, S3C_GPIO_SLP_OUT1);

		gpio_set_value(GPIO_WLAN_nRST, GPIO_LEVEL_HIGH);
		s3c_gpio_slp_cfgpin(GPIO_WLAN_nRST, S3C_GPIO_SLP_OUT1);

		printk(KERN_DEBUG "WLAN: GPIO_WLAN_BT_EN = %d, GPIO_WLAN_nRST = %d\n",
			   gpio_get_value(GPIO_WLAN_BT_EN), gpio_get_value(GPIO_WLAN_nRST));

		spin_unlock_irqrestore(&wlan_reglock, wlan_reglock_flags);
	}
	else {
		/* PROTECT this check under spinlock.. No other thread should be touching
		 * GPIO_BT_REG_ON at this time.. If BT is operational, don't touch it. */
		spin_lock_irqsave(&wlan_reglock, wlan_reglock_flags);
		/* need delay between v_bat & reg_on for 2 cycle @ 38.4MHz */
		udelay(5);

		if (gpio_get_value(GPIO_BT_nRST) == 0) {
			gpio_set_value(GPIO_WLAN_BT_EN, GPIO_LEVEL_LOW);
			s3c_gpio_slp_cfgpin(GPIO_WLAN_BT_EN, S3C_GPIO_SLP_OUT0);
		}

		gpio_set_value(GPIO_WLAN_nRST, GPIO_LEVEL_LOW);
		s3c_gpio_slp_cfgpin(GPIO_WLAN_nRST, S3C_GPIO_SLP_OUT0);

		printk(KERN_DEBUG "WLAN: GPIO_WLAN_BT_EN = %d, GPIO_WLAN_nRST = %d\n",
			   gpio_get_value(GPIO_WLAN_BT_EN), gpio_get_value(GPIO_WLAN_nRST));

		spin_unlock_irqrestore(&wlan_reglock, wlan_reglock_flags);

		s3c_config_gpio_alive_table(ARRAY_SIZE(wlan_sdio_off_table), wlan_sdio_off_table);
	}

	/* mmc_rescan*/
	sdhci_s3c_force_presence_change(&s3c_device_hsmmc1);
}
EXPORT_SYMBOL(wlan_setup_power);

/* Added for Froyo Atlas */
//**** recovery utility interface ****//
#ifdef RECOVERY_SYSFS

struct class *recovery_class=NULL;
static atomic_t device_count;
static struct device *recovery_dev=NULL;
static char* recovery_name ="recovery";
static int recovery_index=0;

static ssize_t wipedata_read(struct device *dev, struct device_attribute *attr, char *buf)
{
	char cmdline[COMMAND_LINE_SIZE];
	char* pCmd = NULL;

    printk(KERN_INFO "%s\n", __func__);

    if(sec_get_param_value){
        sec_get_param_value(__CMDLINE, cmdline);

        printk("CMDLINE : %s\n",cmdline);
        //This is set from boot
        pCmd = strstr(cmdline, "wipedata");
        if( pCmd ){
            *pCmd = 0x00;
	         sec_set_param_value(__CMDLINE, cmdline);
	         return sprintf(buf, "%s", "wipedata");
        }

    }

	return sprintf(buf, "%s", "null");
}

static ssize_t wipedata_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	char cmdline[COMMAND_LINE_SIZE];
	char* pCmd = NULL;
	int mode;

    printk(KERN_INFO "%s\n", __func__);

    if(strstr(buf, "null"))
	{
        if(sec_get_param_value){
            sec_get_param_value(__CMDLINE, cmdline);

            printk("CMDLINE : %s\n",cmdline);

            //This is set from boot
            pCmd = strstr(cmdline, "wipedata");
            if( pCmd ){
                *pCmd = 0x00;
  	            sec_set_param_value(__CMDLINE, cmdline);
            }
        }
	}
    else if(strstr(buf, "wipedata"))
	{
        if(sec_get_param_value){
            sec_get_param_value(__CMDLINE, cmdline);

            printk("CMDLINE : %s\n",cmdline);

            //to reboot as recovery mode to format partitions.
            strcat(cmdline, "wipedata");
  	        sec_set_param_value(__CMDLINE, cmdline);

  	        mode = REBOOT_MODE_RECOVERY;
            sec_set_param_value(__REBOOT_MODE, &mode);

        }
	}


	return size;
}

static ssize_t name_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    printk(KERN_INFO "%s\n", __func__);

	return sprintf(buf, "%s\n", recovery_name);
}

static DEVICE_ATTR(wipedata, S_IRUGO | S_IWUSR, wipedata_read, wipedata_write);
static DEVICE_ATTR(name, S_IRUGO | S_IWUSR, name_show, NULL);

static int create_recovery_class(void)
{
    printk(KERN_INFO "%s\n", __func__);

	if (!recovery_class) {
		recovery_class = class_create(THIS_MODULE, "recovery");
		if (IS_ERR(recovery_class))
			return PTR_ERR(recovery_class);
		atomic_set(&device_count, 0);
	}

	return 0;
}

int recovery_dev_register(void)
{
	int ret;

    printk(KERN_INFO "%s\n", __func__);

	if (!recovery_class) {
		ret = create_recovery_class();
		if (ret < 0)
			return ret;
	}

	recovery_index = atomic_inc_return(&device_count);
	recovery_dev = device_create(recovery_class, NULL,
		MKDEV(0, recovery_index), NULL, recovery_name);
	if (IS_ERR(recovery_dev))
		return PTR_ERR(recovery_dev);

	ret = device_create_file(recovery_dev, &dev_attr_wipedata);
	if (ret < 0)
		goto err_create_file_1;
	ret = device_create_file(recovery_dev, &dev_attr_name);
	if (ret < 0)
		goto err_create_file_2;

	return 0;

err_create_file_2:
	device_remove_file(recovery_dev, &dev_attr_wipedata);
err_create_file_1:
	device_destroy(recovery_class, MKDEV(0, recovery_index));
	printk(KERN_ERR "recovery: Failed to register driver %s\n", recovery_name);

	return ret;
}
EXPORT_SYMBOL_GPL(recovery_dev_register);

void recovery_dev_unregister(struct device *dev)
{
    printk(KERN_INFO "%s\n", __func__);

	device_remove_file(dev, &dev_attr_name);
	device_remove_file(dev, &dev_attr_wipedata);
	device_destroy(recovery_class, MKDEV(0, recovery_index));
}
EXPORT_SYMBOL_GPL(recovery_dev_unregister);

static int __init recovery_class_init(void)
{
	return recovery_dev_register();
}

static void __exit recovery_class_exit(void)
{
    printk(KERN_INFO "%s\n", __func__);

	class_destroy(recovery_class);
}

#endif
