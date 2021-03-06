#ifndef BUILD_LK
#include <linux/string.h>
#include <mach/upmu_common_sw.h>
#endif
#include "lcm_drv.h"

#ifdef BUILD_LK
#include <platform/mt_gpio.h>
#include <platform/mt_i2c.h>
#include <platform/mt_pmic.h>
#elif defined(BUILD_UBOOT)
#include <asm/arch/mt_gpio.h>
#else
#include <mach/mt_gpio.h>
#endif

#include <cust_gpio_usage.h>
#include <cust_i2c.h>
#define LCM_DSI_CMD_MODE 1
#define FRAME_WIDTH (1440)
#define FRAME_HEIGHT (2560)
#define GPIO_65132_EN GPIO_LCD_BIAS_ENP_PIN

#define REGFLAG_PORT_SWAP 0xFFFA
#define REGFLAG_DELAY 0xFFFC
#define REGFLAG_END_OF_TABLE 0xFFFD 

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#define PWM_MIN 0x5
#define PWM_DEFAULT 0x46
#define PWM_MAX 0xD4

static const unsigned int BL_MIN_LEVEL = 20;
static LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v) (lcm_util.set_reset_pin((v)))
#define MDELAY(n) (lcm_util.mdelay(n))

#define dsi_set_cmd_by_cmdq_dual(handle,cmd,count,ppara,force_update)    lcm_util.dsi_set_cmdq_V23(handle,cmd,count,ppara,force_update)
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update) lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update) lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd) lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums) lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd) lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size) lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)
#define dsi_swap_port(swap) lcm_util.dsi_swap_port(swap)


#ifdef BUILD_LK

#define TPS65132_SLAVE_ADDR_WRITE  0x7C
static struct mt_i2c_t TPS65132_i2c;

static int TPS65132_write_byte(kal_uint8 addr, kal_uint8 value)
{
	kal_uint32 ret_code = I2C_OK;
	kal_uint8 write_data[2];
	kal_uint16 len;

	write_data[0]= addr;
	write_data[1] = value;

	TPS65132_i2c.id = I2C_I2C_LCD_BIAS_CHANNEL;
	
	TPS65132_i2c.addr = (TPS65132_SLAVE_ADDR_WRITE >> 1);
	TPS65132_i2c.mode = ST_MODE;
	TPS65132_i2c.speed = 100;
	len = 2;

	ret_code = i2c_write(&TPS65132_i2c, write_data, len);
	if(ret_code)
		LCD_LOG("[LK]r63423----tps6132----addr=%0x--i2c write error----\n",addr);
	else
		LCD_LOG("[LK]r63423----tps6132----addr=%0x--i2c write success----\n",addr);

	return ret_code;
}
#define TPS65132_WRITE_BYTE TPS65132_write_byte
#else
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>

#define TPS_I2C_BUSNUM I2C_I2C_LCD_BIAS_CHANNEL 
#define I2C_ID_NAME "tps65132"
#define TPS_ADDR 0x3E

static struct i2c_board_info __initdata tps65132_board_info = {I2C_BOARD_INFO(I2C_ID_NAME, TPS_ADDR)};
static struct i2c_client *tps65132_i2c_client = NULL;

static int tps65132_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int tps65132_remove(struct i2c_client *client);

struct tps65132_dev {
	struct i2c_client *client;
};

static const struct i2c_device_id tps65132_id[] = {
	{ I2C_ID_NAME, 0 },
	{ }
};

static struct i2c_driver tps65132_iic_driver = {
	.id_table	= tps65132_id,
	.probe		= tps65132_probe,
	.remove		= tps65132_remove,
	.driver		= {
		.owner = THIS_MODULE,
		.name = "tps65132",
	},
};

static int tps65132_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	tps65132_i2c_client = client;
	return 0;
}

static int tps65132_remove(struct i2c_client *client)
{
	tps65132_i2c_client = NULL;
	i2c_unregister_device(client);
	return 0;
}

static int tps65132_write_bytes(unsigned char addr, unsigned char value)
{
	int ret = 0;
	struct i2c_client *client = tps65132_i2c_client;

	if(client == NULL)
	{
		return 0;
	}

	char write_data[2]={0};
	write_data[0]= addr;
	write_data[1] = value;

	ret=i2c_master_send(client, write_data, 2);
	if(ret < 0)
		LCD_LOG("[KERNEL]r63423----tps6132---addr=%0x-- i2c write error-----\n",addr);
	else
		LCD_LOG("[KERNEL]r63423----tps6132---addr=%0x-- i2c write success-----\n",addr);
	if(ret<0)
		return ret ;
}

static int __init tps65132_iic_init(void)
{
	i2c_register_board_info(TPS_I2C_BUSNUM, &tps65132_board_info, 1);
	i2c_add_driver(&tps65132_iic_driver);
	return 0;
}

static void __exit tps65132_iic_exit(void)
{
	i2c_del_driver(&tps65132_iic_driver);
}

module_init(tps65132_iic_init);
module_exit(tps65132_iic_exit);

MODULE_AUTHOR("Xiaokuan Shi");
MODULE_DESCRIPTION("MTK TPS65132 I2C Driver");
MODULE_LICENSE("GPL");
#define TPS65132_WRITE_BYTE tps65132_write_bytes
#endif

struct LCM_setting_table {
	unsigned int cmd;
	unsigned char count;
	unsigned char para_list[64];
};

static struct LCM_setting_table lcm_initialization_setting[] =
{
	{0x2A, 4, {0x00, 0x00, 0x05, 0x9F}},
	{0x2B, 4, {0x00, 0x00, 0x09, 0xFF}},

	{0x11, 0, {}},
	{REGFLAG_DELAY, 80, {}},
	{0xB0, 1, {0x04}},
	{0xD6, 1, {0x01}},
	{0xC6,21, {0x5B, 0x5B, 0x5B, 0x04, 0x4D, 0x00, 0x00,
		   0x00, 0x00, 0x00, 0x00, 0x4A, 0x0B, 0x00,
		   0x00, 0x00, 0x00, 0x03, 0x15, 0x04, 0x5B}},
	{0xCA,36, {0x81, 0xC0, 0xC0, 0xBC, 0xB0, 0xA0, 0xA0,
		   0x0A, 0x20, 0x80, 0x81, 0x00, 0x00, 0x00,
		   0x00, 0x00, 0x00, 0x00, 0x0A, 0x4A, 0x37,
		   0x80, 0x55, 0xC8, 0x0C, 0x0C, 0x0C, 0x08,
		   0x0C, 0x08, 0x10, 0x10, 0x3F, 0x3F, 0x3F,
		   0x3F}},
	{0xC7,30, {0x00, 0x13, 0x1D, 0x2A, 0x3B, 0x4A, 0x55,
		   0x65, 0x4B, 0x53, 0x5E, 0x69, 0x70, 0x76,
		   0x7F, 0x00, 0x13, 0x1D, 0x2A, 0x3B, 0x4A,
		   0x55, 0x65, 0x4B, 0x53, 0x5E, 0x69, 0x70,
		   0x76, 0x7F}},
	{0xCE,25, {0x55, 0x40, 0x49, 0x53, 0x59, 0x5E, 0x63,
		   0x68, 0x6E, 0x74, 0x7E, 0x8A, 0x98, 0xA8,
		   0xBB, 0xD0, 0xFF, 0x04, 0x00, 0x04, 0x04,
		   0x00, 0x00, 0x69, 0x5A}},
	{0x44, 2, {0x00, 0x00}},
	{0x35, 1, {0x00}},
	{0x36, 1, {0x00}},
	{0x3A, 1, {0x77}},
	{0x29, 0, {}},
	{REGFLAG_DELAY, 50, {}},
	
	{0x53, 1, {0x24}},
	{0x5E, 1, {0x22}},

	{0x55, 1, {0x02}},
	{0xB9, 7, {0x6F, 0x3D, 0x28, 0x3C, 0x14, 0xC8, 0xC8}},

	{REGFLAG_END_OF_TABLE, 0x00, {}}
};


static struct LCM_setting_table lcm_suspend_setting[] = {
	{0x28,0,{}},
	{REGFLAG_DELAY, 24, {}},
	{0x34, 1, {0x00}},
	{0x10,0,{}},
	{REGFLAG_DELAY, 80, {}},

	
	{0x53, 1, {0x00}},
	{0x55, 1, {0x00}},

	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

	for(i = 0; i < count; i++)
	{
		unsigned cmd;
		cmd = table[i].cmd;

		switch (cmd) {
		case REGFLAG_DELAY :
			if(table[i].count <= 10)
				MDELAY(table[i].count);
			else
				MDELAY(table[i].count);
			break;

		case REGFLAG_END_OF_TABLE :
			break;

		case REGFLAG_PORT_SWAP:
			dsi_swap_port(1);
			break;
		default:
			dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
		}
	}
}

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}

static const LCM_UTIL_FUNCS* lcm_get_util_funcs(void)
{
	return &lcm_util;
}

static void lcm_get_params(LCM_PARAMS *params)
{
	memset(params, 0, sizeof(LCM_PARAMS));

	params->type = LCM_TYPE_DSI;
	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;
#if defined(CONFIG_CUSTOM_KERNEL_LCM_PHY_WIDTH) && defined(CONFIG_CUSTOM_KERNEL_LCM_PHY_HEIGHT)
	params->physical_width = CONFIG_CUSTOM_KERNEL_LCM_PHY_WIDTH;
	params->physical_height = CONFIG_CUSTOM_KERNEL_LCM_PHY_HEIGHT;
#endif
	params->lcm_if = LCM_INTERFACE_DSI_DUAL;
	params->lcm_cmd_if = LCM_INTERFACE_DSI0;

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode = CMD_MODE;
#else
	params->dsi.mode = SYNC_PULSE_VDO_MODE;
#endif
	params->dsi.dual_dsi_type = DUAL_DSI_CMD;

	
	
	params->dsi.LANE_NUM = LCM_FOUR_LANE;
	
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;

	
	params->dsi.packet_size = 256;
	params->dsi.ssc_disable = 0;
	params->dsi.ssc_range = 5;

	
	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;
	params->dsi.vertical_sync_active = 1;
	params->dsi.vertical_backporch = 7;
	params->dsi.vertical_frontporch	= 7;
	params->dsi.vertical_active_line = FRAME_HEIGHT;
	params->dsi.horizontal_sync_active = 40;
	params->dsi.horizontal_backporch = 80;
	params->dsi.horizontal_frontporch = 100;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	params->dsi.PLL_CLOCK = 416; 
	params->dsi.ufoe_enable = 1;
	params->dsi.ufoe_params.lr_mode_en = 1;

	params->dsi.esd_check_enable = 0;
	params->dsi.customization_esd_check_enable = 0;
	params->dsi.lcm_esd_check_table[2].cmd = 0xb0;
	params->dsi.lcm_esd_check_table[2].count = 1;
	params->dsi.lcm_esd_check_table[2].para_list[0] = 0x04;
	params->dsi.lcm_esd_check_table[1].cmd = 0x36;
	params->dsi.lcm_esd_check_table[1].count = 1;
	params->dsi.lcm_esd_check_table[1].para_list[0] = 0x40;
	params->dsi.lcm_esd_check_table[0].cmd = 0xd6;
	params->dsi.lcm_esd_check_table[0].count = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x01;
	params->dsi.clk_lp_per_line_enable = 0;

	
	params->dsi.lane_swap_en = 0;
	params->dsi.lane_swap[MIPITX_PHY_PORT_1][MIPITX_PHY_LANE_0] = MIPITX_PHY_LANE_2;
	params->dsi.lane_swap[MIPITX_PHY_PORT_1][MIPITX_PHY_LANE_1] = MIPITX_PHY_LANE_CK;
	params->dsi.lane_swap[MIPITX_PHY_PORT_1][MIPITX_PHY_LANE_2] = MIPITX_PHY_LANE_0;
	params->dsi.lane_swap[MIPITX_PHY_PORT_1][MIPITX_PHY_LANE_3] = MIPITX_PHY_LANE_1;
	params->dsi.lane_swap[MIPITX_PHY_PORT_1][MIPITX_PHY_LANE_CK] = MIPITX_PHY_LANE_3;
	params->dsi.lane_swap[MIPITX_PHY_PORT_1][MIPITX_PHY_LANE_RX] = MIPITX_PHY_LANE_2;

	params->dsi.lane_swap[MIPITX_PHY_PORT_0][MIPITX_PHY_LANE_0] = MIPITX_PHY_LANE_0;
	params->dsi.lane_swap[MIPITX_PHY_PORT_0][MIPITX_PHY_LANE_1] = MIPITX_PHY_LANE_3;
	params->dsi.lane_swap[MIPITX_PHY_PORT_0][MIPITX_PHY_LANE_2] = MIPITX_PHY_LANE_2;
	params->dsi.lane_swap[MIPITX_PHY_PORT_0][MIPITX_PHY_LANE_3] = MIPITX_PHY_LANE_1;
	params->dsi.lane_swap[MIPITX_PHY_PORT_0][MIPITX_PHY_LANE_CK] = MIPITX_PHY_LANE_CK;
	params->dsi.lane_swap[MIPITX_PHY_PORT_0][MIPITX_PHY_LANE_RX] = MIPITX_PHY_LANE_0;

#if defined(CONFIG_CUSTOM_KERNEL_PWM_MIN) && defined(CONFIG_CUSTOM_KERNEL_PWM_DEF) && defined(CONFIG_CUSTOM_KERNEL_PWM_MAX) && defined(CONFIG_CUSTOM_KERNEL_CAM_AP_LEVEL)
	params->pwm_min = CONFIG_CUSTOM_KERNEL_PWM_MIN;
	params->pwm_default = CONFIG_CUSTOM_KERNEL_PWM_DEF;
	params->pwm_max = CONFIG_CUSTOM_KERNEL_PWM_MAX;
	params->camera_blk = CONFIG_CUSTOM_KERNEL_CAM_AP_LEVEL;
	params->camera_dua_blk = CONFIG_CUSTOM_KERNEL_CAM_AP_LEVEL;
#endif
}

static void lcm_power_on(void) {
	unsigned char cmd = 0x0;
	unsigned char data = 0xFF;

	
	mt_set_gpio_mode((GPIO62 | 0x80000000), GPIO_MODE_00);
	mt_set_gpio_dir((GPIO62 | 0x80000000), GPIO_DIR_OUT);
	mt_set_gpio_out((GPIO62 | 0x80000000), GPIO_OUT_ONE);
	MDELAY(10);

#ifdef BUILD_LK
	mt6331_upmu_set_rg_vgp1_en(1);
	mt6331_upmu_set_rg_vcam_io_en(1);
#endif
	
	mt_set_gpio_mode(GPIO_LCD_BIAS_ENP_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCD_BIAS_ENP_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ONE);
	MDELAY(1);
	
	cmd = 0x00;
	data = 0x12;
	TPS65132_WRITE_BYTE(cmd, data);
	MDELAY(10);

	
	mt_set_gpio_mode(GPIO_LCD_BIAS_ENN_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCD_BIAS_ENN_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCD_BIAS_ENN_PIN, GPIO_OUT_ONE);
	MDELAY(1);
	
	cmd = 0x01;
	data = 0x12;
	TPS65132_WRITE_BYTE(cmd, data);
	MDELAY(10);

	
	mt_set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_01);

	SET_RESET_PIN(1);
	MDELAY(10);

	SET_RESET_PIN(0);
	MDELAY(10);

	SET_RESET_PIN(1);
	MDELAY(10);
}

static void lcm_power_off(void) {

	
	mt_set_gpio_mode(GPIO_LCD_BIAS_ENN_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCD_BIAS_ENN_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCD_BIAS_ENN_PIN, GPIO_OUT_ZERO);
	MDELAY(10);

	
	mt_set_gpio_mode(GPIO_LCD_BIAS_ENP_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCD_BIAS_ENP_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ZERO);
	MDELAY(10);

	SET_RESET_PIN(1);
	MDELAY(10);

	SET_RESET_PIN(0);
	MDELAY(10);

	
	mt_set_gpio_mode((GPIO62 | 0x80000000), GPIO_MODE_00);
	mt_set_gpio_dir((GPIO62 | 0x80000000), GPIO_DIR_OUT);
	mt_set_gpio_out((GPIO62 | 0x80000000), GPIO_OUT_ZERO);
}

static void lcm_init_power(void)
{
	lcm_power_on();
}

static void lcm_resume_power(void)
{
	lcm_power_on();
}

static void lcm_suspend_power(void)
{
	lcm_power_off();
}

static void lcm_init(void)
{
	
	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_suspend(void)
{
	push_table(lcm_suspend_setting, sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_resume(void)
{
	lcm_init();
}

static void lcm_update(unsigned int x, unsigned int y,
			unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0>>8)&0xFF);
	unsigned char x0_LSB = (x0&0xFF);
	unsigned char x1_MSB = ((x1>>8)&0xFF);
	unsigned char x1_LSB = (x1&0xFF);
	unsigned char y0_MSB = ((y0>>8)&0xFF);
	unsigned char y0_LSB = (y0&0xFF);
	unsigned char y1_MSB = ((y1>>8)&0xFF);
	unsigned char y1_LSB = (y1&0xFF);

	unsigned int data_array[16];

	data_array[0]= 0x00053902;
	data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
	data_array[2]= (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0]= 0x00053902;
	data_array[1]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
	data_array[2]= (y1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0]= 0x002c3909;
	dsi_set_cmdq(data_array, 1, 0);
}

static unsigned int lcm_compare_id(void)
{
	return 1;
}

static unsigned int lcm_check_id(void)
{
	unsigned int retval = (which_lcd_module_triple() == 0) ? 1 : 0;
	return retval;
}

static void lcm_setbacklight_cmdq(void* handle,unsigned int level)
{
	LCD_LOG("%s, backlight: level = %d\n", __func__, level);
	

	unsigned int cmd = 0x51;
	unsigned int count =1;
	unsigned int value = level;
	dsi_set_cmd_by_cmdq_dual(handle, cmd, count, &value, 1);
}

static void lcm_pmic_setting()
{
	mt6332_upmu_set_rg_iwled_ovp(1);
}

LCM_DRIVER r63423_wqhd_jdi_lcm_55_drv =
{
	.name = "r63423_wqhd_jdi_55",
	.set_util_funcs = lcm_set_util_funcs,
	.get_util_funcs = lcm_get_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume  = lcm_resume,
	.compare_id = lcm_compare_id,
	.check_id = lcm_check_id,
	.init_power	= lcm_init_power,
	.resume_power = lcm_resume_power,
	.suspend_power = lcm_suspend_power,
	.set_backlight_cmdq  = lcm_setbacklight_cmdq,
#if (LCM_DSI_CMD_MODE)
	.update = lcm_update,
#endif
	.pmic_setting = lcm_pmic_setting,
};
