/* driver/i2c/chip/rt5506.c
 *
 * Richtek Headphone Amp
 *
 * Copyright (C) 2010 HTC Corporation
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/freezer.h>
#include <linux/mutex.h>
#include <linux/debugfs.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/wakelock.h>
#include <linux/jiffies.h>
#include <linux/of_gpio.h>
#include <sound/htc_acoustic_alsa.h>
#include "rt5506.h"
#if HTC_MSM8994_BRINGUP_OPTION
#include <sound/htc_headset_mgr.h>
#else
#endif

#include <mach/mt_gpio.h>
#define GPIO_RT5506_ENABLE_PIN         (GPIO47|0x80000000)
#define GPIO_RT5506_POWER_SOURCE       (GPIO49|0x80000000)

#undef pr_info
#undef pr_err
#define pr_info(fmt, ...) printk(KERN_INFO "[AUD] "fmt, ##__VA_ARGS__)
#define pr_err(fmt, ...) printk(KERN_ERR "[AUD] "fmt, ##__VA_ARGS__)

#define DEBUG (1)
#define AMP_ON_CMD_LEN 7
#define RETRY_CNT 5
#define RT5506_VERSION_ID 0
#define RT5507_VERSION_ID 2

#define DRIVER_NAME "RT5506"

static int set_rt5506_amp(int on, int dsp);

enum AMP_REG_MODE {
	REG_PWM_MODE = 0,
	REG_AUTO_MODE,
};

struct headset_query {
	struct mutex mlock;
	struct delayed_work hs_imp_detec_work;
	struct wake_lock hs_wake_lock;
	struct wake_lock gpio_wake_lock;
	enum HEADSET_QUERY_STATUS hs_qstatus;
	enum AMP_STATUS rt5506_status;
	enum HEADSET_OM headsetom;
	enum PLAYBACK_MODE curmode;
	enum AMP_GPIO_STATUS gpiostatus;
	enum AMP_REG_MODE regstatus;
	int gpio_off_cancel;
	struct delayed_work volume_ramp_work;
	struct delayed_work gpio_off_work;
	int rt55xx_version;
};

static struct i2c_client *this_client;
static struct rt5506_platform_data *pdata;
static int rt5506Connect = 0;

struct rt5506_config_data* rt5506_config_data_ptr = NULL;
static struct mutex hp_amp_lock;
static int rt5506_opened;
static int last_spkamp_state;
struct rt5506_config RT5506_AMP_ON = {7,{{0x0,0xc0},{0x1,0x1c},{0x2,0x00},{0x7,0x7f},{0x9,0x1},{0xa,0x0},{0xb,0xc7},}};
struct rt5506_config RT5506_AMP_INIT = {11,{{0,0xc0},{0x81,0x30},{0x87,0xf6},{0x93,0x8d},{0x95,0x7d},{0xa4,0x52},\
                                        {0x96,0xae},{0x97,0x13},{0x99,0x35},{0x9b,0x68},{0x9d,0x68},}};

struct rt5506_config RT5506_AMP_MUTE = {1,{{0x1,0xC7},}};;
struct rt5506_config RT5506_AMP_OFF = {1,{{0x0,0x1},}};

static int rt5506_write_reg(u8 reg, u8 val);
static void hs_imp_detec_func(struct work_struct *work);
static int rt5506_i2c_read_addr(unsigned char *rxData, unsigned char addr);
static int rt5506_i2c_write(struct rt5506_reg_data *txData, int length);
static void set_amp(int on, struct rt5506_config *i2c_command);
struct headset_query rt5506_query;
static struct workqueue_struct *hs_wq;

static struct workqueue_struct *ramp_wq;
static struct workqueue_struct *gpio_wq;

#ifdef CONFIG_DEBUG_FS
static struct dentry *debugfs_rt5506_dent;
static struct dentry *debugfs_rt5506_ps_control;

static int rt5506_amp_debug_get(void *data, u64 *val)
{
	pr_info("%s: read gpio 0x%x, status get = %d\n",
		__func__, GPIO_RT5506_POWER_SOURCE, mt_get_gpio_out(GPIO_RT5506_POWER_SOURCE));
	*val = (u64) mt_get_gpio_out(GPIO_RT5506_POWER_SOURCE);
	return 0;
}

static int rt5506_amp_debug_set(void *data, u64 val)
{
	if (val) {
		mt_set_gpio_mode(GPIO_RT5506_POWER_SOURCE, GPIO_MODE_00);
		mt_set_gpio_dir(GPIO_RT5506_POWER_SOURCE, GPIO_DIR_OUT);
		mt_set_gpio_out(GPIO_RT5506_POWER_SOURCE, GPIO_OUT_ONE);
		pr_info("%s: enable gpio 0x%x, val=%lld\n", __func__, GPIO_RT5506_POWER_SOURCE, val);
	} else {
		mt_set_gpio_mode(GPIO_RT5506_POWER_SOURCE, GPIO_MODE_00);
		mt_set_gpio_dir(GPIO_RT5506_POWER_SOURCE, GPIO_DIR_OUT);
		mt_set_gpio_out(GPIO_RT5506_POWER_SOURCE, GPIO_OUT_ZERO);
		pr_info("%s: disable gpio 0x%x\n", __func__, GPIO_RT5506_POWER_SOURCE);
	}
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(rt5506_amp_debug_ops, rt5506_amp_debug_get, rt5506_amp_debug_set, "%lld\n")
#endif

static int set_rt5506_regulator(enum AMP_REG_MODE mode)
{
#if HTC_MSM8994_BRINGUP_OPTION
	if(pdata->power_reg) {

		switch(mode) {
			case REG_PWM_MODE:
				pr_info("%s:set regulator to PWM mode\n",__func__);
				rpm_regulator_set_mode(pdata->power_reg,RPM_REGULATOR_MODE_HPM);
				break;

			case REG_AUTO_MODE:
				pr_info("%s:set regulator to AUTO mode\n",__func__);
				rpm_regulator_set_mode(pdata->power_reg,RPM_REGULATOR_MODE_AUTO);
				break;

			default:
				break;
		}
	}
#endif
	return 0;
}

void set_rt55xx_init_reg(int version, int delay) {
	pr_info("%s: version 0x%x(dts 0x%x), delay %d\n", __func__, version, pdata->rt55xx_version, delay);

	if (version == pdata->rt55xx_version) {
		switch(version) {
			case RT5506_VERSION_ID:
				rt5506_write_reg(0x0,0x4); 
				mdelay(delay);
				rt5506_write_reg(0x0,0xc0);
				rt5506_write_reg(0x81,0x30);
				
				rt5506_write_reg(0x90,0xd0);
				rt5506_write_reg(0x93,0x9d);
				rt5506_write_reg(0x95,0x7b);
				rt5506_write_reg(0xa4,0x52);
				
				rt5506_write_reg(0x97,0x00);
				rt5506_write_reg(0x98,0x22);
				rt5506_write_reg(0x99,0x33);
				rt5506_write_reg(0x9a,0x55);
				rt5506_write_reg(0x9b,0x66);
				rt5506_write_reg(0x9c,0x99);
				rt5506_write_reg(0x9d,0x66);
				rt5506_write_reg(0x9e,0x99);
				break;
			case RT5507_VERSION_ID:
				rt5506_write_reg(0,0x04); 
				mdelay(delay);
				rt5506_write_reg(0x0,0xc0);
				rt5506_write_reg(0x81,0x30);
				rt5506_write_reg(0x87,0xea);
				rt5506_write_reg(0x90,0xd0);
				rt5506_write_reg(0x93,0x9d);
				rt5506_write_reg(0x95,0x7b);
				rt5506_write_reg(0xa4,0x52);
				rt5506_write_reg(0x97,0x00);
				rt5506_write_reg(0x98,0x22);
				rt5506_write_reg(0x99,0x33);
				rt5506_write_reg(0x9a,0x55);
				rt5506_write_reg(0x9b,0x66);
				rt5506_write_reg(0x9c,0x99);
				rt5506_write_reg(0x9d,0x66);
				rt5506_write_reg(0x9e,0x99);
				break;
			default:
				pr_err("%s: not support version 0x%x, disable power source of RT55xx and make it no sound!!!\n",
					__func__, version);
				mt_set_gpio_out(GPIO_RT5506_POWER_SOURCE, GPIO_OUT_ZERO);
				break;
		}
	} else {
		pr_err("%s: verson(0x%x) does not match dts version(0x%x), disable power source of RT55xx and make it no sound!!!\n",
			__func__, version, pdata->rt55xx_version);
		mt_set_gpio_out(GPIO_RT5506_POWER_SOURCE, GPIO_OUT_ZERO);
	}
}

int rt5506_headset_detect(void *private_data, int on)
{

	if(!rt5506Connect)
		return 0;

	if(on) {

		pr_info("%s: headset in, rt55xx version 0x%x(dts 0x%x) ++\n",
			__func__, rt5506_query.rt55xx_version, pdata->rt55xx_version);
		cancel_delayed_work_sync(&rt5506_query.hs_imp_detec_work);
		mutex_lock(&rt5506_query.mlock);
		rt5506_query.hs_qstatus = QUERY_HEADSET;
		rt5506_query.headsetom = HEADSET_OM_UNDER_DETECT;

		if(rt5506_query.rt5506_status == STATUS_PLAYBACK) {
			rt5506_write_reg(1,0xc7);

			last_spkamp_state = 0;
			pr_info("%s: OFF\n", __func__);

			rt5506_query.rt5506_status = STATUS_SUSPEND;
		}
		pr_info("%s: headset in --\n",__func__);
		mutex_unlock(&rt5506_query.mlock);
		
		queue_delayed_work(hs_wq,&rt5506_query.hs_imp_detec_work,msecs_to_jiffies(5));
		pr_info("%s: headset in --2\n",__func__);

	} else {

		pr_info("%s: headset remove ++\n",__func__);
		cancel_delayed_work_sync(&rt5506_query.hs_imp_detec_work);
		flush_work(&rt5506_query.volume_ramp_work.work);
		mutex_lock(&rt5506_query.mlock);
		rt5506_query.hs_qstatus = QUERY_OFF;
		rt5506_query.headsetom = HEADSET_OM_UNDER_DETECT;

		if(rt5506_query.regstatus == REG_AUTO_MODE) {
			set_rt5506_regulator(REG_PWM_MODE);
			rt5506_query.regstatus = REG_PWM_MODE;
		}

		if(rt5506_query.rt5506_status == STATUS_PLAYBACK) {
			rt5506_write_reg(1,0xc7);

			last_spkamp_state = 0;
			pr_info("%s: OFF\n", __func__);

			rt5506_query.rt5506_status = STATUS_SUSPEND;
		}

		rt5506_query.curmode = PLAYBACK_MODE_OFF;
		pr_info("%s: headset remove --1\n",__func__);


		mutex_unlock(&rt5506_query.mlock);

		pr_info("%s: headset remove --2\n",__func__);

	}

	return 0;
}

static void rt5506_register_hs_notification(void)
{
#if HTC_MSM8994_BRINGUP_OPTION
	struct hs_notify_t notifier;
	notifier.private_data = NULL;
	notifier.callback_f = rt5506_headset_detect;
	htc_acoustic_register_hs_notify(HS_AMP_N, &notifier);
#endif
}

static int rt5506_write_reg(u8 reg, u8 val)
{
	int err;
	struct i2c_msg msg[1];
	unsigned char data[2];

	msg->addr = this_client->addr;
	msg->flags = 0;
	msg->ext_flag = I2C_HS_FLAG;
	msg->timing = 400;
	msg->len = 2;
	msg->buf = data;
	data[0] = reg;
	data[1] = val;
	pr_info("%s: write reg 0x%x val 0x%x\n",__func__,data[0],data[1]);
	err = i2c_transfer(this_client->adapter, msg, 1);
	if (err >= 0)
		return 0;
        else {

            pr_info("%s: write error error %d\n",__func__,err);
            return err;
        }
}

static int rt5506_i2c_write(struct rt5506_reg_data *txData, int length)
{
	int i, retry, pass = 0;
	char buf[2];
	struct i2c_msg msg[] = {
		{
		 .addr = this_client->addr,
		 .flags = 0,
	     .ext_flag = I2C_HS_FLAG,
	     .timing = 400,
		 .len = 2,
		 .buf = buf,
		},
	};
	for (i = 0; i < length; i++) {
		
		
		buf[0] = txData[i].addr;
		buf[1] = txData[i].val;

#if DEBUG
		pr_info("%s:i2c_write addr 0x%x val 0x%x\n", __func__,buf[0], buf[1]);
#endif
		msg->buf = buf;
		retry = RETRY_CNT;
		pass = 0;
		while (retry--) {
			if (i2c_transfer(this_client->adapter, msg, 1) < 0) {
				pr_err("%s: I2C transfer error %d retry %d\n",
						__func__, i, retry);
				msleep(20);
			} else {
				pass = 1;
				break;
			}
		}
		if (pass == 0) {
			pr_err("I2C transfer error, retry fail\n");
			return -EIO;
		}
	}
	return 0;
}

static int rt5506_i2c_read_addr(unsigned char *rxData, unsigned char addr)
{
	int rc;
	struct i2c_msg msgs[] = {
		{
		 .addr = this_client->addr,
		 .flags = 0,
	     .ext_flag = I2C_HS_FLAG,
	     .timing = 400,
		 .len = 1,
		 .buf = rxData,
		},
		{
		 .addr = this_client->addr,
		 .flags = I2C_M_RD,
	     .ext_flag = I2C_HS_FLAG,
	     .timing = 400,
		 .len = 1,
		 .buf = rxData,
		},
	};

	if(!rxData)
		return -1;

	*rxData = addr;

	rc = i2c_transfer(this_client->adapter, msgs, 2);
	if (rc < 0) {
		pr_err("%s:[1] transfer error %d\n", __func__, rc);
		return rc;
	}

	pr_info("%s:i2c_read addr 0x%x value = 0x%x\n", __func__, addr, *rxData);
	return 0;
}

static int rt5506_open(struct inode *inode, struct file *file)
{
	int rc = 0;

	mutex_lock(&hp_amp_lock);

	if (rt5506_opened) {
		pr_err("%s: busy\n", __func__);
		rc = -EBUSY;
		goto done;
	}
	rt5506_opened = 1;
done:
	mutex_unlock(&hp_amp_lock);
	return rc;
}

static int rt5506_release(struct inode *inode, struct file *file)
{
	mutex_lock(&hp_amp_lock);
	rt5506_opened = 0;
	mutex_unlock(&hp_amp_lock);

	return 0;
}

static void hs_imp_gpio_off(struct work_struct *work)
{
	u64 timeout = get_jiffies_64() + 3*HZ;
	wake_lock(&rt5506_query.gpio_wake_lock);

	while(1) {
		if(time_after64(get_jiffies_64(),timeout))
			break;
		else if(rt5506_query.gpio_off_cancel) {
			wake_unlock(&rt5506_query.gpio_wake_lock);
			return;
		} else
			msleep(10);
	}

	mutex_lock(&rt5506_query.mlock);
	pr_info("%s: disable gpio %d\n",__func__,pdata->gpio_rt55xx_enable);
	
	mt_set_gpio_out(GPIO_RT5506_ENABLE_PIN, GPIO_OUT_ZERO);
	
	rt5506_query.gpiostatus = AMP_GPIO_OFF;

	if(rt5506_query.regstatus == REG_PWM_MODE) {
		set_rt5506_regulator(REG_AUTO_MODE);
		rt5506_query.regstatus = REG_AUTO_MODE;
	}

	mutex_unlock(&rt5506_query.mlock);
	wake_unlock(&rt5506_query.gpio_wake_lock);
}

static void hs_imp_detec_func(struct work_struct *work)
{
	struct headset_query *hs;
	unsigned char temp[8]={0x1,};
	unsigned char r_channel;
	int ret;
	pr_info("%s: read rt5506 hs imp \n",__func__);

	hs = container_of(work, struct headset_query, hs_imp_detec_work.work);
	wake_lock(&hs->hs_wake_lock);

	rt5506_query.gpio_off_cancel = 1;
	cancel_delayed_work_sync(&rt5506_query.gpio_off_work);
	mutex_lock(&hs->mlock);

	if(hs->hs_qstatus != QUERY_HEADSET) {
		mutex_unlock(&hs->mlock);
		wake_unlock(&hs->hs_wake_lock);
		return;
	}


	if(hs->gpiostatus == AMP_GPIO_OFF) {

		if(rt5506_query.regstatus == REG_AUTO_MODE) {
			set_rt5506_regulator(REG_PWM_MODE);
			rt5506_query.regstatus = REG_PWM_MODE;
			msleep(1);
		}
		pr_info("%s: enable gpio %d\n",__func__,pdata->gpio_rt55xx_enable);
		
		mt_set_gpio_out(GPIO_RT5506_ENABLE_PIN, GPIO_OUT_ONE);
		
		rt5506_query.gpiostatus = AMP_GPIO_ON;
	}

	usleep_range(20000,20000);

	rt5506_write_reg(0,0x04); 
	rt5506_write_reg(0xa4,0x52);
	rt5506_write_reg(1,0x7);
	msleep(10);
	rt5506_write_reg(0x3,0x81);

	msleep(101);

	ret = rt5506_i2c_read_addr(temp,0x4);

	if(ret < 0) {
		pr_err("%s: read rt5506 status error %d\n",__func__,ret);

		if(hs->gpiostatus == AMP_GPIO_ON) {

			rt5506_query.gpio_off_cancel = 0;
			queue_delayed_work(gpio_wq, &rt5506_query.gpio_off_work, msecs_to_jiffies(0));
		}

		mutex_unlock(&hs->mlock);
		wake_unlock(&hs->hs_wake_lock);
		return;
	}

	rt5506_i2c_read_addr(&r_channel,0x6);

	if (rt5506_query.rt55xx_version == RT5506_VERSION_ID || rt5506_query.rt55xx_version == RT5507_VERSION_ID)
		set_rt55xx_init_reg(rt5506_query.rt55xx_version, 1);
	else {
		pr_err("%s: unknown version ID 0x%d\n",
			__func__, rt5506_query.rt55xx_version);
		set_rt55xx_init_reg(rt5506_query.rt55xx_version, 1);
	}

	if(temp[0] & AMP_SENSE_READY) {

		unsigned char om, hsmode;
		enum HEADSET_OM hsom;

		hsmode = (temp[0] & 0x30) >> 4;
		om = (temp[0] & 0xe) >> 1;

		if(r_channel == 0) {
			
			hsom = HEADSET_MONO;
		} else {

			switch(om) {
				case 0:
					hsom = HEADSET_8OM;
					break;
				case 1:
					hsom = HEADSET_16OM;
					break;
				case 2:
					hsom = HEADSET_32OM;
					break;
				case 3:
					hsom = HEADSET_64OM;
					break;
				case 4:
					hsom = HEADSET_128OM;
					break;
				case 5:
					hsom = HEADSET_256OM;
					break;
				case 6:
					hsom = HEADSET_500OM;
					break;
				case 7:
					hsom = HEADSET_1KOM;
					break;

				default:
					hsom = HEADSET_OM_UNDER_DETECT;
					break;
			}
		}

		hs->hs_qstatus = QUERY_FINISH;
		hs->headsetom = hsom;

		pr_info("rt5506 hs imp value 0x%x hsmode %d om 0x%x hsom %d r_channel 0x%x, rt55xx version 0x%x(dts 0x%x)\n",\
			temp[0] & 0xf, hsmode, om, hsom, r_channel, rt5506_query.rt55xx_version, pdata->rt55xx_version);
	} else {
		pr_info("%s: impedance detection is not ready, hs status %d, temp[0] 0x%x\n", __func__, hs->hs_qstatus, temp[0]);
		if(hs->hs_qstatus == QUERY_HEADSET)
			queue_delayed_work(hs_wq,&rt5506_query.hs_imp_detec_work,QUERY_LATTER);
	}

	rt5506_write_reg(1,0xc7);

	if(hs->gpiostatus == AMP_GPIO_ON) {

		rt5506_query.gpio_off_cancel = 0;
		queue_delayed_work(gpio_wq, &rt5506_query.gpio_off_work, msecs_to_jiffies(0));

	}

	mutex_unlock(&hs->mlock);

	if(hs->rt5506_status == STATUS_SUSPEND)
		set_rt5506_amp(1,0);

	wake_unlock(&hs->hs_wake_lock);
}

static void volume_ramp_func(struct work_struct *work)
{

	if(rt5506_query.rt5506_status != STATUS_PLAYBACK) {

		mdelay(1);
		
		rt5506_write_reg(0x2,0x0);
		mdelay(1);
	}

	set_amp(1, &RT5506_AMP_ON);
}

static void set_amp_l(int on, struct rt5506_config *i2c_command)
{
	pr_info("%s: on %d, om 0x%x\n", __func__, on, rt5506_query.headsetom);

	if(rt5506_query.hs_qstatus == QUERY_HEADSET)
		rt5506_query.hs_qstatus = QUERY_FINISH;

	if (on) {
		rt5506_query.rt5506_status = STATUS_PLAYBACK;
		if (rt5506_i2c_write(i2c_command->reg, i2c_command->reg_len) == 0) {
			last_spkamp_state = 1;
			pr_info("%s: ON \n",__func__);
		}

	} else {
		rt5506_write_reg(1,0xc7);

		if(rt5506_query.rt5506_status == STATUS_PLAYBACK) {
			last_spkamp_state = 0;
			pr_info("%s: OFF\n", __func__);
		}
		rt5506_query.rt5506_status = STATUS_OFF;
		rt5506_query.curmode = PLAYBACK_MODE_OFF;
	}
}


static void set_amp(int on, struct rt5506_config *i2c_command)
{
	pr_info("%s: %d\n", __func__, on);
	mutex_lock(&rt5506_query.mlock);
	mutex_lock(&hp_amp_lock);
	set_amp_l(on, i2c_command);
	mutex_unlock(&hp_amp_lock);
	mutex_unlock(&rt5506_query.mlock);
}

int query_rt5506(void)
{
    return rt5506Connect;
}

static int set_rt5506_amp(int on, int dsp)
{
	if(!rt5506Connect)
		return 0;

	pr_info("%s: on = %d\n", __func__, on);
	rt5506_query.gpio_off_cancel = 1;
	cancel_delayed_work_sync(&rt5506_query.gpio_off_work);
	cancel_delayed_work_sync(&rt5506_query.volume_ramp_work);
	
	mutex_lock(&rt5506_query.mlock);

	if(on) {

		if(rt5506_query.gpiostatus == AMP_GPIO_OFF) {

			if(rt5506_query.regstatus == REG_AUTO_MODE) {
				set_rt5506_regulator(REG_PWM_MODE);
				rt5506_query.regstatus = REG_PWM_MODE;
				msleep(1);
			}

			pr_info("%s: enable gpio %d\n", __func__, pdata->gpio_rt55xx_enable);
			
			mt_set_gpio_out(GPIO_RT5506_ENABLE_PIN, GPIO_OUT_ONE);
			
			rt5506_query.gpiostatus = AMP_GPIO_ON;
			usleep_range(20000,20000);
		}
		queue_delayed_work(ramp_wq, &rt5506_query.volume_ramp_work, msecs_to_jiffies(0));

	} else {
		set_amp_l(0, &RT5506_AMP_ON);
		if(rt5506_query.gpiostatus == AMP_GPIO_ON) {

			rt5506_query.gpio_off_cancel = 0;
			queue_delayed_work(gpio_wq, &rt5506_query.gpio_off_work, msecs_to_jiffies(0));
		}

	}

	mutex_unlock(&rt5506_query.mlock);
	return 0;
}

static int update_amp_parameter(int mode)
{
	if (rt5506_config_data_ptr == NULL) {
		pr_err("%s: rt5506_config_data_ptr doesn't init", __func__);
		return -ENOMEM;
	}
	if (mode >= rt5506_config_data_ptr->mode_num)
		return -EINVAL;

	pr_info("%s: set mode %d\n", __func__, mode);

	if (mode == PLAYBACK_MODE_OFF)
		memcpy(&RT5506_AMP_OFF, &rt5506_config_data_ptr->cmd_data[mode].config,
				sizeof(struct rt5506_config));
	else if (mode == AMP_INIT)
		memcpy(&RT5506_AMP_INIT, &rt5506_config_data_ptr->cmd_data[mode].config,
				sizeof(struct rt5506_config));
	else if (mode == AMP_MUTE)
		memcpy(&RT5506_AMP_MUTE, &rt5506_config_data_ptr->cmd_data[mode].config,
				sizeof(struct rt5506_config));
	else {
		memcpy(&RT5506_AMP_ON, &rt5506_config_data_ptr->cmd_data[mode].config,
				sizeof(struct rt5506_config));
	}
	return 0;
}


static long rt5506_ioctl(struct file *file, unsigned int cmd,
	   unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	int rc = 0, modeid = 0;
	int premode = 0;
	struct amp_ctrl ampctrl;
	struct rt5506_reg_data reg;
	enum AMP_GPIO_STATUS curgpiostatus;

	switch (_IOC_NR(cmd)) {
	case _IOC_NR(AMP_SET_MODE):
		if (copy_from_user(&modeid, argp, sizeof(modeid)))
			return -EFAULT;

		if (rt5506_config_data_ptr == NULL) {
			pr_err("%s: doesn't init the config_data_ptr\n", __func__);
			return -ENOMEM;
		}
		if (!rt5506_config_data_ptr->cmd_data) {
			pr_err("%s: out of memory\n", __func__);
			return -ENOMEM;
		}

		if (modeid >= rt5506_config_data_ptr->mode_num || modeid < 0) {
			pr_err("unsupported rt5506 mode %d\n", modeid);
			return -EINVAL;
		}
		mutex_lock(&hp_amp_lock);
		premode = rt5506_query.curmode;
		rt5506_query.curmode = modeid;
		rc = update_amp_parameter(modeid);
		mutex_unlock(&hp_amp_lock);
		pr_info("%s:set rt5506 mode %d -> %d om 0x%x curstatus %d\n",
			__func__, premode, modeid, rt5506_query.headsetom, rt5506_query.rt5506_status);
		mutex_lock(&rt5506_query.mlock);
		if(rt5506_query.rt5506_status == STATUS_PLAYBACK && premode != rt5506_query.curmode) {
			flush_work(&rt5506_query.volume_ramp_work.work);
			queue_delayed_work(ramp_wq, &rt5506_query.volume_ramp_work, msecs_to_jiffies(280));
		}
		mutex_unlock(&rt5506_query.mlock);
		break;
	case _IOC_NR(AMP_SET_PARAM):
		mutex_lock(&hp_amp_lock);
		if(sizeof(struct rt5506_config_data) == _IOC_SIZE(cmd)) {

			if (rt5506_config_data_ptr == NULL) {
				rt5506_config_data_ptr = kzalloc(sizeof(struct rt5506_config_data), GFP_KERNEL);
				if (NULL == rt5506_config_data_ptr) {
					pr_err("%s: out of memory\n", __func__);
					mutex_unlock(&hp_amp_lock);
					rc = -ENOMEM;
					break;
				}
			}

			if (copy_from_user((void*)rt5506_config_data_ptr, (void*)argp, sizeof(struct rt5506_config_data))) {
				pr_err("%s: set_param copy_from_user failed\n", __func__);
				mutex_unlock(&hp_amp_lock);
				rc = -EFAULT;
				break;
			}

			pr_info("%s: update rt5506 i2c commands #%d success.\n",
					__func__, rt5506_config_data_ptr->mode_num);
			
			update_amp_parameter(PLAYBACK_MODE_OFF);
			update_amp_parameter(AMP_MUTE);
			update_amp_parameter(AMP_INIT);
		}
		else {
			pr_err("%s: RT55XX_SET_PARAM error! sizeof(struct rt5506_config_data)= %#lx, _IOC_SIZE(cmd)= %#x\n", __func__, sizeof(struct rt5506_config_data), _IOC_SIZE(cmd));
			rc = -EINVAL;
		}
		mutex_unlock(&hp_amp_lock);
		break;
	case _IOC_NR(AMP_QUERY_OM):
		mutex_lock(&rt5506_query.mlock);
		rc = rt5506_query.headsetom;
		mutex_unlock(&rt5506_query.mlock);
		pr_info("%s: query headset om %d\n", __func__,rc);

		if (copy_to_user(argp, &rc, sizeof(rc)))
			rc = -EFAULT;
		else
			rc = 0;
		break;
	case _IOC_NR(ACOUSTIC_AMP_CTRL):
		if (copy_from_user(&ampctrl, argp, sizeof(ampctrl)))
			return -EFAULT;

		if(!this_client)
			return -EFAULT;

		if(ampctrl.slave != AUD_AMP_SLAVE_ALL && ampctrl.slave != this_client->addr)
			break;

		mutex_lock(&rt5506_query.mlock);
		mutex_lock(&hp_amp_lock);

		rc = 0;
		curgpiostatus = rt5506_query.gpiostatus;

		if(rt5506_query.gpiostatus == AMP_GPIO_OFF) {

			if(rt5506_query.regstatus == REG_AUTO_MODE) {
				set_rt5506_regulator(REG_PWM_MODE);
				rt5506_query.regstatus = REG_PWM_MODE;
				msleep(1);
			}

			pr_info("%s: enable gpio %d\n",__func__,pdata->gpio_rt55xx_enable);
			
			mt_set_gpio_out(GPIO_RT5506_ENABLE_PIN, GPIO_OUT_ONE);
			
			usleep_range(20000,20000);
		}

		if(ampctrl.ctrl == AMP_WRITE) {
			reg.addr = (unsigned char)ampctrl.reg;
			reg.val = (unsigned char)ampctrl.val;
			rt5506_write_reg(reg.addr,reg.val);
		} else if (ampctrl.ctrl == AMP_READ) {
			reg.addr = (unsigned char)ampctrl.reg;
			rt5506_i2c_read_addr(&reg.val, reg.addr);
			ampctrl.val = (unsigned int)reg.val;

			if (copy_to_user(argp, &ampctrl, sizeof(ampctrl)))
				rc = -EFAULT;
		}

		if(curgpiostatus == AMP_GPIO_OFF) {
			rt5506_query.gpio_off_cancel = 0;
			queue_delayed_work(gpio_wq, &rt5506_query.gpio_off_work, msecs_to_jiffies(0));
		}

		mutex_unlock(&hp_amp_lock);
		mutex_unlock(&rt5506_query.mlock);
		break;
	case _IOC_NR(AMP_READ_CONFIG):
		mutex_lock(&rt5506_query.mlock);
		rc = rt5506_query.rt55xx_version;
		mutex_unlock(&rt5506_query.mlock);
		pr_info("%s: query RT55xx version 0x%x\n", __func__, rc);

		if (copy_to_user(argp, &rc, sizeof(rc)))
			rc = -EFAULT;
		else
			rc = 0;
		break;
	default:
		pr_err("%s: Invalid command\n", __func__);
		rc = -EINVAL;
		break;
	}
	return rc;
}

static int rt55xx_parse_pfdata(struct device *dev, struct rt5506_platform_data *ppdata)
{
	struct device_node *dt = dev->of_node;
	enum of_gpio_flags flags;
	int ret;
	struct property *prop;

	pdata->rt55xx_version = -1;

	if (dt) {
		prop = of_find_property(dt, "rt55xx,version", NULL);
		if (prop) {
			of_property_read_u32(dt, "rt55xx,version", (u32*)&(pdata->rt55xx_version));
			printk("%s:  rt55xx_version 0x%x", __func__, pdata->rt55xx_version);
		} else {
			printk("%s:  rt55xx_version not found on dts", __func__);
		}
	} else {
		pr_info("%s: dt is NULL\n", __func__);
	}

	return 0;
#if 0
	pdata->gpio_rt55xx_enable = -EINVAL;
	pdata->power_supply = NULL;
	pdata->power_reg = NULL;

	if (dt) {
		pdata->gpio_rt55xx_enable = of_get_named_gpio_flags(dt,"richtek,enable-gpio",0, &flags);
		ret = of_property_read_string(dt,"power_supply",&pdata->power_supply);

		if(ret < 0) {
			pdata->power_supply = NULL;
			pr_err("%s:parse power supply fail\n",__func__);
		}

	} else {
		if(dev->platform_data) {
			pdata->gpio_rt55xx_enable = ((struct rt5506_platform_data *)dev->platform_data)->gpio_rt55xx_enable;
			pdata->power_supply = ((struct rt5506_platform_data *)dev->platform_data)->power_supply;
		}
	}

	pr_info("%s: rt5506 gpio %d\n",__func__,pdata->gpio_rt55xx_enable);

	if(pdata->power_supply)
		pr_info("%s:power supply %s\n",__func__,pdata->power_supply);

	if(pdata->power_supply != NULL) {
#if HTC_MSM8994_BRINGUP_OPTION
		pdata->power_reg = rpm_regulator_get(NULL, pdata->power_supply);
#endif

		if (IS_ERR(pdata->power_reg)) {
			pdata->power_reg = NULL;
			pr_err("%s: reqest regulator %s fail\n",__func__,pdata->power_supply);
		}
	}

	if(gpio_is_valid(pdata->gpio_rt55xx_enable))
		return 0;
	else
		return -EINVAL;
#endif
}

static struct file_operations rt5506_fops = {
	.owner = THIS_MODULE,
	.open = rt5506_open,
	.release = rt5506_release,
	.unlocked_ioctl = rt5506_ioctl,
	.compat_ioctl = rt5506_ioctl,
};

static struct miscdevice rt5506_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "rt5506",
	.fops = &rt5506_fops,
};

int rt5506_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;

	pr_info("rt5506_probe");
	htc_amp_power_enable(true);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("%s: i2c check functionality error\n", __func__);
		ret = -ENODEV;
		goto err_alloc_data_failed;
	}

	if (pdata == NULL) {

		pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
		if (pdata == NULL) {
			ret = -ENOMEM;
			pr_err("%s: platform data is NULL\n", __func__);
			goto err_alloc_data_failed;
		}
	}

	if(rt55xx_parse_pfdata(&client->dev, pdata) < 0)
		goto err_free_allocated_mem;

	this_client = client;

	if(1) {
		unsigned char temp[2];

		
		
		pr_info("%s: enable rt550x power source %x\n", __func__, GPIO_RT5506_POWER_SOURCE);
		mt_set_gpio_mode(GPIO_RT5506_POWER_SOURCE, GPIO_MODE_00);
		mt_set_gpio_dir(GPIO_RT5506_POWER_SOURCE, GPIO_DIR_OUT);
		mt_set_gpio_out(GPIO_RT5506_POWER_SOURCE, GPIO_OUT_ONE);
		mdelay(10);

		
		pr_info("%s: enable rt550x %x\n", __func__, GPIO_RT5506_ENABLE_PIN);
		mt_set_gpio_mode(GPIO_RT5506_ENABLE_PIN, GPIO_MODE_00);
		mt_set_gpio_dir(GPIO_RT5506_ENABLE_PIN, GPIO_DIR_OUT);
		mt_set_gpio_out(GPIO_RT5506_ENABLE_PIN, GPIO_OUT_ONE);
		mdelay(10);
		pdata->gpio_rt55xx_enable = (GPIO_RT5506_ENABLE_PIN & ~(0x80000000));
		pr_info("%s:[2]current gpio %d value %d\n",__func__, pdata->gpio_rt55xx_enable, mt_get_gpio_out(pdata->gpio_rt55xx_enable));
		

		
		unsigned char version = 0;
		const int TRY_COUNT = 10;
		const int DELAY_MS = 10;
		int i = 0;
		mutex_lock(&rt5506_query.mlock);
		for (i = TRY_COUNT; i > 0; i--) {
			ret = rt5506_i2c_read_addr(&version, 0xc0);
			if (ret == 0) {
				pr_info("%s: RT55xx version 0x%x\n", __func__, version);
				rt5506_query.rt55xx_version = version;
				break;
			}
			pr_info("%s: read RT55xx version failed, ret %d, re-try %d\n", __func__, ret, TRY_COUNT - i);
			mdelay(DELAY_MS);
		}
		if (ret < 0 && i == 0) {
			pr_err("%s: can Not read version from rt55xx and set to -1, ret %d, re-try %d\n", __func__, ret, TRY_COUNT - i);
			rt5506_query.rt55xx_version = -1;
		}
		mutex_unlock(&rt5506_query.mlock);

		if (rt5506_query.rt55xx_version == RT5506_VERSION_ID)
			set_rt55xx_init_reg(rt5506_query.rt55xx_version, 5);
		else if (rt5506_query.rt55xx_version == RT5507_VERSION_ID)
			set_rt55xx_init_reg(rt5506_query.rt55xx_version, 1);
		else {
			pr_err("%s: unknown version ID 0x%d\n",
				__func__, rt5506_query.rt55xx_version);
			set_rt55xx_init_reg(rt5506_query.rt55xx_version, 1);
		}

		rt5506_write_reg(0x1,0xc7);
		mdelay(10);
		ret = rt5506_i2c_read_addr(temp, 0x1);
		if(ret < 0) {
			pr_err("rt5506 is not connected\n");
			rt5506Connect = 0;
		} else {
			pr_info("rt5506 is connected\n");
			rt5506Connect = 1;
		}
		rt5506Connect = 1;

		
		mt_set_gpio_out(GPIO_RT5506_ENABLE_PIN, GPIO_OUT_ZERO);
		

	}

	if(rt5506Connect) {
		pr_info("%s: resiter hs amp\n", __func__);
		htc_acoustic_register_hs_amp(set_rt5506_amp,&rt5506_fops);
		ret = misc_register(&rt5506_device);
		if (ret) {
			pr_err("%s: rt5506_device register failed\n", __func__);
			goto err_free_allocated_mem;
		}

		hs_wq = create_workqueue("rt5506_hsdetect");
		INIT_DELAYED_WORK(&rt5506_query.hs_imp_detec_work,hs_imp_detec_func);
		wake_lock_init(&rt5506_query.hs_wake_lock, WAKE_LOCK_SUSPEND, "rt5506 hs wakelock");
		wake_lock_init(&rt5506_query.gpio_wake_lock, WAKE_LOCK_SUSPEND, "rt5506 gpio wakelock");
		ramp_wq = create_workqueue("rt5506_volume_ramp");
		INIT_DELAYED_WORK(&rt5506_query.volume_ramp_work, volume_ramp_func);
		gpio_wq = create_workqueue("rt5506_gpio_off");
		INIT_DELAYED_WORK(&rt5506_query.gpio_off_work, hs_imp_gpio_off);
		
		

		
		rt5506_query.hs_qstatus = QUERY_HEADSET;
		rt5506_query.headsetom = HEADSET_OM_UNDER_DETECT;
		queue_delayed_work(hs_wq,&rt5506_query.hs_imp_detec_work,msecs_to_jiffies(5));
	}

#ifdef CONFIG_DEBUG_FS
	debugfs_rt5506_dent = debugfs_create_dir("rt5506", NULL);
	if (!IS_ERR(debugfs_rt5506_dent)) {
		pr_err("%s: rt5506_device debugfs_create_dir success\n", __func__);
		debugfs_rt5506_ps_control = debugfs_create_file("rt5506_power_source_control",
		S_IFREG | S_IRUGO, debugfs_rt5506_dent,
		(void *) "rt5506_power_source_control", &rt5506_amp_debug_ops);
	} else
		pr_err("%s: rt5506_device debugfs_create_dir failed\n", __func__);
#endif

	return 0;

err_free_allocated_mem:
	if(pdata) {
		pr_info("%s: free pdata\n", __func__);
		kfree(pdata);
		pdata = NULL;
	}
err_alloc_data_failed:
        rt5506Connect = 0;
	return ret;
}

static int rt5506_remove(struct i2c_client *client)
{
	struct rt5506_platform_data *p5506data = i2c_get_clientdata(client);
	pr_info("%s:\n",__func__);
	if(p5506data)
		kfree(p5506data);

        if(rt5506Connect) {
            misc_deregister(&rt5506_device);
            cancel_delayed_work_sync(&rt5506_query.hs_imp_detec_work);
            destroy_workqueue(hs_wq);
        }
	return 0;
}

static void rt5506_shutdown(struct i2c_client *client)
{
	rt5506_query.gpio_off_cancel = 1;
	cancel_delayed_work_sync(&rt5506_query.gpio_off_work);
	cancel_delayed_work_sync(&rt5506_query.volume_ramp_work);

	mutex_lock(&hp_amp_lock);
	mutex_lock(&rt5506_query.mlock);

	if(rt5506_query.gpiostatus == AMP_GPIO_OFF) {

		if(rt5506_query.regstatus == REG_AUTO_MODE) {
			set_rt5506_regulator(REG_PWM_MODE);
			rt5506_query.regstatus = REG_PWM_MODE;
			msleep(1);
		}

		pr_info("%s: enable gpio %d\n",__func__,pdata->gpio_rt55xx_enable);
		
		mt_set_gpio_out(GPIO_RT5506_ENABLE_PIN, GPIO_OUT_ONE);
		
		rt5506_query.gpiostatus = AMP_GPIO_ON;
		usleep_range(20000,20000);
	}
	pr_info("%s: reset rt5506\n",__func__);
	rt5506_write_reg(0x0,0x4);
	mdelay(1);

	if(rt5506_query.gpiostatus == AMP_GPIO_ON) {
		pr_info("%s: disable gpio %d\n",__func__,pdata->gpio_rt55xx_enable);
		
		mt_set_gpio_out(GPIO_RT5506_ENABLE_PIN, GPIO_OUT_ZERO);
		
		rt5506_query.gpiostatus = AMP_GPIO_OFF;

		if(rt5506_query.regstatus == REG_PWM_MODE) {
			set_rt5506_regulator(REG_AUTO_MODE);
			rt5506_query.regstatus = REG_AUTO_MODE;
		}
	}

	rt5506Connect = 0;

	mutex_unlock(&rt5506_query.mlock);
	mutex_unlock(&hp_amp_lock);

}

static int rt5506_suspend(struct i2c_client *client, pm_message_t mesg)
{
	return 0;
}

static int rt5506_resume(struct i2c_client *client)
{
	return 0;
}

static struct of_device_id rt5506_match_table[] = {
	{ .compatible = "richtek,rt5506-amp",},
	{ },
};

static const struct i2c_device_id rt5506_id[] = {
	{ RT5506_I2C_NAME, 0 },
	{ },
};

static struct i2c_driver rt5506_driver = {
	.probe = rt5506_probe,
	.remove = rt5506_remove,
	.shutdown = rt5506_shutdown,
	.suspend = rt5506_suspend,
	.resume = rt5506_resume,
	.id_table = rt5506_id,
	.driver = {
		.owner	= THIS_MODULE,
		.name = RT5506_I2C_NAME,
#ifdef CONFIG_OF
		.of_match_table = rt5506_match_table,
#endif
	},
};

static int __init rt5506_init(void)
{
	mutex_init(&hp_amp_lock);
	mutex_init(&rt5506_query.mlock);
	rt5506_query.rt5506_status = STATUS_OFF;
	rt5506_query.hs_qstatus = QUERY_OFF;
	rt5506_query.headsetom = HEADSET_8OM;
	rt5506_query.curmode = PLAYBACK_MODE_OFF;
	rt5506_query.gpiostatus = AMP_GPIO_OFF;
	rt5506_query.regstatus = REG_AUTO_MODE;
	rt5506_query.rt55xx_version = -1;
	return i2c_add_driver(&rt5506_driver);
}

static void __exit rt5506_exit(void)
{
	i2c_del_driver(&rt5506_driver);

#ifdef CONFIG_DEBUG_FS
	if (debugfs_rt5506_ps_control)
		debugfs_remove(debugfs_rt5506_ps_control);
	if (debugfs_rt5506_dent)
		debugfs_remove(debugfs_rt5506_dent);
#endif

#if HTC_MSM8994_BRINGUP_OPTION
	if(pdata->power_reg)
		rpm_regulator_put(pdata->power_reg);
#endif
	if(rt5506_config_data_ptr) {
		kfree(rt5506_config_data_ptr);
		rt5506_config_data_ptr = NULL;
	}

	if(pdata) {
		pr_info("%s: free pdata\n", __func__);
		kfree(pdata);
		pdata = NULL;
	}
}

module_init(rt5506_init);
module_exit(rt5506_exit);

MODULE_DESCRIPTION("rt5506 Headphone Amp driver");
MODULE_LICENSE("GPL");
