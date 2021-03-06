#ifdef CONFIG_COMPAT

#include <linux/fs.h>
#include <linux/compat.h>

#endif
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/time.h>
#include "kd_flashlight.h"
#include <asm/io.h>
#include <asm/uaccess.h>
#include "kd_camera_hw.h"
#include <cust_gpio_usage.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/xlog.h>
#include <linux/version.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
#include <linux/mutex.h>
#else
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
#include <linux/semaphore.h>
#else
#include <asm/semaphore.h>
#endif
#endif

#include <linux/i2c.h>
#include <linux/leds.h>
#include <linux/dma-mapping.h>

#include <linux/htc_flashlight.h>


#define FLASH_ktd2693

static DEFINE_SPINLOCK(fl_ktd2693b_lock);

#define FLASHLIGHT_NAME "flashlight"


struct mt6332_led_data {
	u8			num_leds;
	struct i2c_client	*client_dev;
	struct tps61310_data 	*tps61310;
	int status;
	struct led_classdev	cdev;
	int			max_current;
	int			id;
	u8			default_state;
	int                     torch_mode;
	struct mutex		lock;
	struct work_struct	work;
};

static struct led_classdev	this_fit;
struct delayed_work ktd2693_delayed_work;
static struct workqueue_struct *ktd2693_work_queue;

#define TAG_NAME "leds_strobe.c"
#define PK_DBG_NONE(fmt, arg...)    do {} while (0)
#define PK_DBG_FUNC(fmt, arg...)    xlog_printk(ANDROID_LOG_DEBUG  , TAG_NAME, KERN_INFO  "%s: " fmt, __FUNCTION__ ,##arg)
#define PK_WARN(fmt, arg...)        xlog_printk(ANDROID_LOG_WARNING, TAG_NAME, KERN_WARNING  "%s: " fmt, __FUNCTION__ ,##arg)
#define PK_NOTICE(fmt, arg...)      xlog_printk(ANDROID_LOG_DEBUG  , TAG_NAME, KERN_NOTICE  "%s: " fmt, __FUNCTION__ ,##arg)
#define PK_INFO(fmt, arg...)        xlog_printk(ANDROID_LOG_INFO   , TAG_NAME, KERN_INFO  "%s: " fmt, __FUNCTION__ ,##arg)
#define PK_TRC_FUNC(f)              xlog_printk(ANDROID_LOG_DEBUG  , TAG_NAME,  "<%s>\n", __FUNCTION__);
#define PK_TRC_VERBOSE(fmt, arg...) xlog_printk(ANDROID_LOG_VERBOSE, TAG_NAME,  fmt, ##arg)
#define PK_ERROR(fmt, arg...)       xlog_printk(ANDROID_LOG_ERROR  , TAG_NAME, KERN_ERR "%s: " fmt, __FUNCTION__ ,##arg)


#define DEBUG_LEDS_STROBE
#ifdef  DEBUG_LEDS_STROBE
	#define PK_DBG printk
	#define PK_VER printk
	#define PK_ERR printk
#else
	#define PK_DBG(a,...)
	#define PK_VER(a,...)
	#define PK_ERR(a,...)
#endif

#define SEND_LOW() do {\
        mt_set_gpio_out(GPIO_FLT_CTRL, GPIO_OUT_ZERO); \
                udelay(80);\
        mt_set_gpio_out(GPIO_FLT_CTRL, GPIO_OUT_ONE); \
                udelay(40);\
} while(0)
 
#define SEND_HIGH() do {\
        mt_set_gpio_out(GPIO_FLT_CTRL, GPIO_OUT_ZERO); \
                udelay(40);\
        mt_set_gpio_out(GPIO_FLT_CTRL, GPIO_OUT_ONE); \
                udelay(80);\
} while(0)


#define FLASHLIGHT_DEVNAME            "ktd2693"
struct flash_chip_data {
	struct led_classdev cdev_flash;
	struct led_classdev cdev_torch;

	struct mutex lock;

	int mode;
	int torch_level;
};

static struct flash_chip_data chipconf;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
static DEFINE_MUTEX(g_strobeSem);
#else
static DECLARE_MUTEX(g_strobeSem);
#endif

#define STROBE_DEVICE_ID 0x60

struct ktd2693_platform_data {
	u8 torch_pin_enable;    
	u8 pam_sync_pin_enable; 
	u8 thermal_comp_mode_enable;
	u8 strobe_pin_disable;  
	u8 vout_mode_enable;  
};

struct ktd2693_chip_data {
	struct i2c_client *client;

	struct led_classdev cdev_flash;
	struct led_classdev cdev_torch;
	struct led_classdev cdev_indicator;

	struct ktd2693_platform_data *pdata;
	struct mutex lock;

	u8 last_flag;
	u8 no_pdata;
};

static int ktd2693_senddata(uint8_t data)
{
	unsigned long int_flags;
	struct timeval tv1,tv2;
	printk("[FLT] Start: data=%2X\n", data);

	do_gettimeofday(&tv1);
	spin_lock_irqsave(&fl_ktd2693b_lock, int_flags);
	mt_set_gpio_out(GPIO_FLT_CTRL, GPIO_OUT_ONE);
	udelay(30); 
	
	if (data&0x80) SEND_HIGH(); else SEND_LOW();
	if (data&0x40) SEND_HIGH(); else SEND_LOW();
	if (data&0x20) SEND_HIGH(); else SEND_LOW();
	if (data&0x10) SEND_HIGH(); else SEND_LOW();
	if (data&0x08) SEND_HIGH(); else SEND_LOW();
	if (data&0x04) SEND_HIGH(); else SEND_LOW();
	if (data&0x02) SEND_HIGH(); else SEND_LOW();
	if (data&0x01) SEND_HIGH(); else SEND_LOW();
	
	mt_set_gpio_out(GPIO_FLT_CTRL, GPIO_OUT_ZERO);
	udelay(20); 
	mt_set_gpio_out(GPIO_FLT_CTRL, GPIO_OUT_ONE);
	udelay(1000); 
	spin_unlock_irqrestore(&fl_ktd2693b_lock, int_flags);
	do_gettimeofday(&tv2);
	printk("[FLT] data send time:(%ld)\n", (tv2.tv_usec - tv1.tv_usec));
	return 0;
}

static int ktd2693_turn_off(void)
{
	printk("[FLT] %s\n", __func__);
	ktd2693_senddata(0xA0);

	return 0;
}

static void ktd2693_turn_off_work(struct work_struct *work)
{
	printk("[FLT] %s\n", __func__);
	ktd2693_turn_off();
}

static void led_set(struct led_classdev *led_cdev,
				enum led_brightness value)
{
	

	printk(KERN_INFO "[FLT] Flashlight set value %d.\n", value);


   	if(value > 0)
   	{
   		switch(value)
   		{
   			
   			case 125:
				ktd2693_senddata(0x61);
				ktd2693_senddata(0xA1);
				break;
			
			case 126:
				ktd2693_senddata(0x63);    
				ktd2693_senddata(0xA1);
				break;
			
			case 127:
				ktd2693_senddata(0x65);    
				ktd2693_senddata(0xA1);
				break;
			
			case 128:
				ktd2693_senddata(0x64);    
				ktd2693_senddata(0xA1);
				break;
			
			case 129:
				mt_set_gpio_out(GPIO_FLT_EN, GPIO_OUT_ZERO);
				ktd2693_senddata(0x80);    
				ktd2693_senddata(0xA2);
				queue_delayed_work(ktd2693_work_queue, &ktd2693_delayed_work, msecs_to_jiffies(600));
				break;
			
			case 130:
				mt_set_gpio_out(GPIO_FLT_EN, GPIO_OUT_ZERO);
				ktd2693_senddata(0x81);    
				ktd2693_senddata(0xA2);
				queue_delayed_work(ktd2693_work_queue, &ktd2693_delayed_work, msecs_to_jiffies(600));
				break;
			
			case 131:
				mt_set_gpio_out(GPIO_FLT_EN, GPIO_OUT_ZERO);
				ktd2693_senddata(0x82);    
				ktd2693_senddata(0xA2);
				queue_delayed_work(ktd2693_work_queue, &ktd2693_delayed_work, msecs_to_jiffies(600));
				break;
			
			case 132:
				mt_set_gpio_out(GPIO_FLT_EN, GPIO_OUT_ZERO);
				ktd2693_senddata(0x83);    
				ktd2693_senddata(0xA2);
				queue_delayed_work(ktd2693_work_queue, &ktd2693_delayed_work, msecs_to_jiffies(600));
				break;
			
			case 133:
				mt_set_gpio_out(GPIO_FLT_EN, GPIO_OUT_ZERO);
				ktd2693_senddata(0x84);    
				ktd2693_senddata(0xA2);
				queue_delayed_work(ktd2693_work_queue, &ktd2693_delayed_work, msecs_to_jiffies(600));
				break;
			
			case 134:
				mt_set_gpio_out(GPIO_FLT_EN, GPIO_OUT_ZERO);
				ktd2693_senddata(0x85);    
				ktd2693_senddata(0xA2);
				queue_delayed_work(ktd2693_work_queue, &ktd2693_delayed_work, msecs_to_jiffies(600));
				break;
			
			case 135:
				mt_set_gpio_out(GPIO_FLT_EN, GPIO_OUT_ZERO);
				ktd2693_senddata(0x86);    
				ktd2693_senddata(0xA2);
				queue_delayed_work(ktd2693_work_queue, &ktd2693_delayed_work, msecs_to_jiffies(600));
				break;
			
			case 136:
				mt_set_gpio_out(GPIO_FLT_EN, GPIO_OUT_ZERO);
				ktd2693_senddata(0x87);    
				ktd2693_senddata(0xA2);
				queue_delayed_work(ktd2693_work_queue, &ktd2693_delayed_work, msecs_to_jiffies(600));
				break;
			
			case 137:
				mt_set_gpio_out(GPIO_FLT_EN, GPIO_OUT_ZERO);
				ktd2693_senddata(0x88);    
				ktd2693_senddata(0xA2);
				queue_delayed_work(ktd2693_work_queue, &ktd2693_delayed_work, msecs_to_jiffies(600));
				break;
			
			case 138:
				mt_set_gpio_out(GPIO_FLT_EN, GPIO_OUT_ZERO);
				ktd2693_senddata(0x89);    
				ktd2693_senddata(0xA2);
				queue_delayed_work(ktd2693_work_queue, &ktd2693_delayed_work, msecs_to_jiffies(600));
				break;
			
			case 139:
				mt_set_gpio_out(GPIO_FLT_EN, GPIO_OUT_ZERO);
				ktd2693_senddata(0x8a);    
				ktd2693_senddata(0xA2);
				queue_delayed_work(ktd2693_work_queue, &ktd2693_delayed_work, msecs_to_jiffies(600));
				break;
			
			case 140:
				mt_set_gpio_out(GPIO_FLT_EN, GPIO_OUT_ZERO);
				ktd2693_senddata(0x8b);    
				ktd2693_senddata(0xA2);
				queue_delayed_work(ktd2693_work_queue, &ktd2693_delayed_work, msecs_to_jiffies(600));
				break;
			
			case 141:
				mt_set_gpio_out(GPIO_FLT_EN, GPIO_OUT_ZERO);
				ktd2693_senddata(0x8c);    
				ktd2693_senddata(0xA2);
				queue_delayed_work(ktd2693_work_queue, &ktd2693_delayed_work, msecs_to_jiffies(600));
				break;
			
			case 142:
				mt_set_gpio_out(GPIO_FLT_EN, GPIO_OUT_ZERO);
				ktd2693_senddata(0x8c);    
				ktd2693_senddata(0xA2);
				queue_delayed_work(ktd2693_work_queue, &ktd2693_delayed_work, msecs_to_jiffies(600));
				break;
			
			case 255:
				mt_set_gpio_out(GPIO_FLT_EN, GPIO_OUT_ZERO);
				ktd2693_senddata(0x8c);    
				ktd2693_senddata(0xA2);
				queue_delayed_work(ktd2693_work_queue, &ktd2693_delayed_work, msecs_to_jiffies(600));
				break;
   		}

		
   	}
	else if(value == 0)
	{
		ktd2693_senddata(0xA0);
		printk("[FLT] set value = 0\n");
	}
	else{
		printk("[FLT] Exception value:  0x%x\n",value);
	}

	
	
}

static enum led_brightness led_get(struct led_classdev *led_cdev)
{
	struct mt6332_led_data *led;

	led = container_of(led_cdev, struct mt6332_led_data, cdev);

	printk("[FLT] Flashlight get value %d.\n", led->cdev.brightness);

	return led->cdev.brightness;
}


int ktd2693_flt_flash(struct led_classdev *flt, uint32_t mA)
{
	u8 lv = 0x0;
	uint32_t regval= 0x0;
	int ret = 0;

	lv=(u8)((int)mA/56);
	printk("[FLT] Flashlight ktd2693_flt_flash set %d mA, lv value 0x%x.\n", mA,lv);

	if (mA == 0) {
		ktd2693_senddata(0xA0);
	} else if(mA > 0 && mA <= 1000) {
		mt_set_gpio_out(GPIO_FLT_EN, GPIO_OUT_ZERO);
		ktd2693_senddata(0x8c);    
		ktd2693_senddata(0xA2);
		queue_delayed_work(ktd2693_work_queue, &ktd2693_delayed_work, msecs_to_jiffies(600));
		printk("[FLT] Flashlight ktd2693_flt_flash set %d mA, reg val: %x, lv value %d.\n", mA,regval,lv);
	} else {
		printk("[FLT] Flashlight unsupport value.\n");
	}

	return ret;
}

int ktd2693_flt_torch(struct led_classdev *flt, uint32_t mA)
{
	u8 lv = 0x0;
	uint32_t regval= 0x0;
	int ret = 0;

	lv=(u8)((int)mA/28);

	printk("[FLT] Flashlight ktd2693_flt_torch set %d mA, lv value 0x%x.\n", mA,lv);

	if (mA == 0) {
		ktd2693_senddata(0xA0);
	} else if(mA > 0 && mA <= 224) {
		ktd2693_senddata(0x65);    
		ktd2693_senddata(0xA1);
		printk("[FLT] Flashlight ktd2693_flt_torch set %d mA, reg val: %x, lv value %d.\n", mA,regval,lv);
	} else {
		printk("[FLT] Flashlight unsupport value.\n");
	}

    return ret;
}

#if defined(CONFIG_HTC_FLASHLIGHT_COMMON)

static int ktd2693_flt_flash_adapter(int mA1, int mA2){
	return ktd2693_flt_flash(&this_fit, mA1);
}

static int ktd2693_flt_torch_adapter(int mA1, int mA2){
	return ktd2693_flt_torch(&this_fit, mA1);
}
#endif

static int ktd2693_suspend(struct platform_device *dev)
{
	printk("[FLT] %s:\n", __func__);
	ktd2693_turn_off();
	mt_set_gpio_out(GPIO_FLT_CTRL, GPIO_OUT_ZERO);
	return 0;
}



static int flashchip_probe(struct platform_device *dev)
{
	struct flash_chip_data *chip;

	int ret;
	static struct led_classdev	cdev;

	cdev.name			   = FLASHLIGHT_NAME;
	cdev.brightness_set    = led_set;
	cdev.brightness_get    = led_get;

	printk("[FLT] probe start\n");
	PK_ERR("[flashchip_probe] start\n");
	chip = &chipconf;
	chip->mode = 0;
	chip->torch_level = 0;
	
	mutex_init(&chip->lock);

	printk("[FLT] probe Done\n");

	ret = led_classdev_register(&dev->dev, &cdev);
	if (ret) {
		printk(KERN_ERR "[FLT] "
			"unable to register led rec=%d\n", ret);
		
	}

    PK_ERR("[flashchip_probe] Done\n");

#if defined(CONFIG_HTC_FLASHLIGHT_COMMON)
	printk("if defined(CONFIG_HTC_FLASHLIGHT_COMMON) %s %d\n",__FUNCTION__,__LINE__);
	htc_flash_main			= &ktd2693_flt_flash_adapter;
	htc_torch_main			= &ktd2693_flt_torch_adapter;
#endif

	INIT_DELAYED_WORK(&ktd2693_delayed_work, ktd2693_turn_off_work);
	ktd2693_work_queue = create_singlethread_workqueue("ktd2693_wq");
	if (!ktd2693_work_queue)
		goto err_chip_init;

	mt_set_gpio_mode(GPIO_FLT_STROBE, 0);
	mt_set_gpio_dir(GPIO_FLT_STROBE, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_FLT_STROBE, GPIO_OUT_ZERO);

	mt_set_gpio_mode(GPIO_FLT_EN, 0);
	mt_set_gpio_dir(GPIO_FLT_EN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_FLT_EN, GPIO_OUT_ZERO);

	mt_set_gpio_mode(GPIO_FLT_CTRL, 0);
	mt_set_gpio_dir(GPIO_FLT_CTRL, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_FLT_CTRL, GPIO_OUT_ZERO);

	ktd2693_senddata(0x00);    
	ktd2693_senddata(0x20);    
	return 0;

err_chip_init:
	printk(KERN_ERR "[flashchip_probe] is failed !\n");
	return -ENODEV;

}

static int flashchip_remove(struct platform_device *dev)
{
	struct flash_chip_data *chip = &chipconf;
    PK_DBG("[flashchip_remove] start\n");

	led_classdev_unregister(&chip->cdev_torch);
	led_classdev_unregister(&chip->cdev_flash);


    PK_DBG("[flashchip_remove] Done\n");
    return 0;
}

static struct platform_driver flashchip_platform_driver =
{
    .probe      = flashchip_probe,
    .remove     = flashchip_remove,
    .suspend   = ktd2693_suspend,
    .driver     = {
        .name = FLASHLIGHT_DEVNAME,
		.owner	= THIS_MODULE,
		
    },
};



static struct platform_device flashchip_platform_device = {
    .name = FLASHLIGHT_DEVNAME,
    .id = 0,
    .dev = {
    }
};

static int __init flashchip_init(void)
{
    int ret = 0;
    printk("[FLT][flashchip_init] start\n");

	ret = platform_device_register (&flashchip_platform_device);
	if (ret) {
        PK_ERR("[flashchip_init] platform_device_register fail\n");
        return ret;
	}

    ret = platform_driver_register(&flashchip_platform_driver);
	if(ret){
		PK_ERR("[flashchip_init] platform_driver_register fail\n");
		return ret;
	}

	printk("[FLT][flashchip_init] done!\n");
    return ret;
}

static void __exit flashchip_exit(void)
{
    printk("[FLT][flashchip_exit] start\n");
    platform_driver_unregister(&flashchip_platform_driver);
    printk("[FLT][flashchip_exit] done!\n");
}

late_initcall(flashchip_init);
module_exit(flashchip_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("pengfei_li@htc.com>");
MODULE_DESCRIPTION("ktd2693 flash control Driver");
