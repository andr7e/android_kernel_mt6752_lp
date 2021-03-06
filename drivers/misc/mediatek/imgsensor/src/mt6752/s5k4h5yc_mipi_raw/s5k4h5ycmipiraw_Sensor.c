
#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <linux/xlog.h>

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "s5k4h5ycmipiraw_Sensor.h"

#define OPEN_OTP_FUNCTION

#ifdef OPEN_OTP_FUNCTION
extern bool s5k4h5yc_otp_update();
#endif
#define PFX "S5K4H5YC_camera_sensor"
#define LOG_INF(format, args...)	xlog_printk(ANDROID_LOG_INFO   , PFX, "[%s] " format, __FUNCTION__, ##args)
extern int iReadReg(u16 a_u2Addr , u8 * a_puBuff , u16 i2cId);
extern int iWriteReg(u16 a_u2Addr , u32 a_u4Data , u32 a_u4Bytes , u16 i2cId);
#define write_cmos_sensor(addr, para) iWriteReg((u16) addr , (u32) para , 1, imgsensor.i2c_write_id)
static DEFINE_SPINLOCK(imgsensor_drv_lock);

#ifdef SLT_DEVINFO_CMM 
#include  <linux/dev_info.h>
static struct devinfo_struct *s_DEVINFO_ccm;   
#endif

static imgsensor_info_struct imgsensor_info = { 
	.sensor_id = S5K4H5YC_SENSOR_ID,
	
	.checksum_value = 0xc3ad8ba4,
	
	.pre = {
		.pclk = 140000000,	
		.linelength = 3688,
		.framelength = 1280,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1600,		
		.grabwindow_height = 1200,		
		
		.mipi_data_lp2hs_settle_dc = 14,
		
		.max_framerate = 300,	
	},
	.cap = {
		.pclk = 140000000,
		.linelength = 3688,
		.framelength = 2512,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 3200,	
		.grabwindow_height = 2400,	
		.mipi_data_lp2hs_settle_dc = 14,
		.max_framerate = 150,
	},
	.cap1 = {
		.pclk = 140000000,
		.linelength = 3688,
		.framelength = 2512,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 3200,
		.grabwindow_height = 2400,
		.mipi_data_lp2hs_settle_dc = 14,
		.max_framerate = 150,
	},
	.normal_video = {
		.pclk = 140000000,
		.linelength = 3688,
		.framelength = 1280,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1600,
		.grabwindow_height = 1200,
		.mipi_data_lp2hs_settle_dc = 14,
		.max_framerate = 300,	
	},
	.hs_video = {
		.pclk = 140000000,
		.linelength = 3688,
		.framelength = 640,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 820,
		.grabwindow_height = 616,
		.mipi_data_lp2hs_settle_dc = 14,
		.max_framerate = 600,
	},
	.slim_video = {
		.pclk = 140000000,
		.linelength = 3688,
		.framelength = 1280,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1280,
		.grabwindow_height = 720,
		.mipi_data_lp2hs_settle_dc = 14,
		.max_framerate = 300,
	},
	.margin = 16,
	.min_shutter = 3,
	.max_frame_length = 0xffff,
	.ae_shut_delay_frame = 0,
	.ae_sensor_gain_delay_frame = 1,
	.ae_ispGain_delay_frame = 2,
	.ihdr_support = 0,	  
	.ihdr_le_firstline = 0,  
	.sensor_mode_num = 5,	  
	
	.cap_delay_frame = 3, 
	.pre_delay_frame = 2, 
	.video_delay_frame = 3,
	.hs_video_delay_frame = 3,
	.slim_video_delay_frame = 3,
	
	.isp_driving_current = ISP_DRIVING_8MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_Gb,
	.mclk = 24,
	.mipi_lane_num = SENSOR_MIPI_2_LANE,
	.i2c_addr_table = {0x30,0x6c,0x6e,0xff},
};


static imgsensor_struct imgsensor = {
	.mirror = IMAGE_NORMAL,				
	.sensor_mode = IMGSENSOR_MODE_INIT, 
	.shutter = 0x3D0,					
	.gain = 0x100,						
	.dummy_pixel = 0,					
	.dummy_line = 0,					
    .current_fps = 30,  
    .autoflicker_en = KAL_TRUE,  
	.test_pattern = KAL_FALSE,		
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,
	.ihdr_en = 0, 
	.i2c_write_id = 0x30,
};


static SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[5] =	 
{{ 2624, 1956,	  8,	2, 2608, 1952, 1296,  972, 0000, 0000, 1296,  972,	  2,	2, 1280,  960}, 
 { 2624, 1956,	 16,	6, 2592, 1944, 2592, 1944, 0000, 0000, 2592, 1944,	  2,	2, 2560, 1920}, 
 { 2624, 1956,	 16,	6, 2592, 1944, 2592, 1944, 0000, 0000, 2592, 1944,	  3,	3, 2560, 1920}, 
 { 2624, 1956,	  2,  250, 2620, 1456, 1920, 1080, 0000, 0000, 1920, 1080,	  2,	2, 1920, 1080}, 
 { 2624, 1956,	  8,  246, 2608, 1460, 1280,  720, 0000, 0000, 1280,  720,	  2,	2, 1280,  720}};


static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;
    iReadReg((u16) addr ,(u8*)&get_byte,imgsensor.i2c_write_id);
    return get_byte;
}


static void set_dummy()
{
	LOG_INF("dummyline = %d, dummypixels = %d \n", imgsensor.dummy_line, imgsensor.dummy_pixel);

	write_cmos_sensor(0x0340, imgsensor.frame_length >> 8);
	write_cmos_sensor(0x0341, imgsensor.frame_length & 0xFF);	  
	write_cmos_sensor(0x0342, imgsensor.line_length >> 8);
	write_cmos_sensor(0x0343, imgsensor.line_length & 0xFF);
}
		

static void set_max_framerate(UINT16 framerate,kal_bool min_framelength_en)
{
	kal_int16 dummy_line;
	kal_uint32 frame_length = imgsensor.frame_length;
	

	LOG_INF("framerate = %d, min framelength should enable? \n", framerate,min_framelength_en);
	frame_length = imgsensor.pclk / framerate * 10 / imgsensor.line_length;
	spin_lock(&imgsensor_drv_lock);
	imgsensor.frame_length = (frame_length > imgsensor.min_frame_length) ? frame_length : imgsensor.min_frame_length; 
	imgsensor.dummy_line = imgsensor.frame_length - imgsensor.min_frame_length;
	
	
		
	
		
	
	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
	{
		imgsensor.frame_length = imgsensor_info.max_frame_length;
		imgsensor.dummy_line = imgsensor.frame_length - imgsensor.min_frame_length;
	}
	if (min_framelength_en)
		imgsensor.min_frame_length = imgsensor.frame_length;
	spin_unlock(&imgsensor_drv_lock);
	set_dummy();
}	


static void write_shutter(kal_uint16 shutter)
	{
		kal_uint16 realtime_fps = 0;
		kal_uint32 frame_length = 0;
		
		
		
		
		
		
	    LOG_INF("[ylf]enter write shutter shutter =%d, framelength =%d\n", shutter,imgsensor.frame_length);
		if(imgsensor.sensor_mode == IMGSENSOR_MODE_HIGH_SPEED_VIDEO)
		{
			if(shutter > imgsensor.min_frame_length - imgsensor_info.margin)
				shutter = imgsensor.min_frame_length - imgsensor_info.margin;
			write_cmos_sensor(0x0340, imgsensor.frame_length >> 8);
			write_cmos_sensor(0x0341, imgsensor.frame_length & 0xFF);
			write_cmos_sensor(0x0202, (shutter >> 8) & 0xFF); 
 			write_cmos_sensor(0x0203, shutter  & 0xFF);
			LOG_INF("shutter =%d, framelength =%d\n", shutter,imgsensor.frame_length);
			return;
		}
		spin_lock(&imgsensor_drv_lock);
		if (shutter > imgsensor.min_frame_length - imgsensor_info.margin)
			{
			imgsensor.frame_length = shutter + imgsensor_info.margin;
			LOG_INF("[ylf]shutter bigger imgsensor.min_frame_length shutter =%d, framelength =%d\n", shutter,imgsensor.frame_length);
			}
		else
			{
			imgsensor.frame_length = imgsensor.min_frame_length;
			LOG_INF("[ylf]shutter smaller imgsensor.min_frame_length shutter =%d, framelength =%d\n", shutter,imgsensor.frame_length);
			}
		if (imgsensor.frame_length > imgsensor_info.max_frame_length)
			{
			imgsensor.frame_length = imgsensor_info.max_frame_length;
			LOG_INF("[ylf]imgsensor.frame_length > imgsensor_info.max_frame_length shutter =%d, framelength =%d\n", shutter,imgsensor.frame_length);
			}
		spin_unlock(&imgsensor_drv_lock);
		if (shutter < imgsensor_info.min_shutter) 
			shutter = imgsensor_info.min_shutter;
		
		if (imgsensor.autoflicker_en == KAL_TRUE) { 
			realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
			if(realtime_fps >= 297 && realtime_fps <= 305)
			{
				set_max_framerate(296,0);
				
			}
			else if(realtime_fps >= 147 && realtime_fps <= 150)
			{
				set_max_framerate(146,0);	
				
			}
			else{
			write_cmos_sensor(0x0340, imgsensor.frame_length >> 8);
			write_cmos_sensor(0x0341, imgsensor.frame_length & 0xFF);
			}
		} else {
			
			write_cmos_sensor(0x0340, imgsensor.frame_length >> 8);
			write_cmos_sensor(0x0341, imgsensor.frame_length & 0xFF);
		}
	
		
		
		
		write_cmos_sensor(0x0202, (shutter >> 8) & 0xFF); 
 		write_cmos_sensor(0x0203, shutter  & 0xFF);
		LOG_INF("shutter =%d, framelength =%d\n", shutter,imgsensor.frame_length);
	
		
		
	}




static void set_shutter(kal_uint16 shutter)
{
	unsigned long flags;
	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);
	write_shutter(shutter);
}	



static kal_uint16 gain2reg(const kal_uint16 gain)
{
}

static kal_uint16 set_gain(kal_uint16 gain)
{
	gain = gain / 2;
	
	
	write_cmos_sensor(0x0204,(gain>>8));
	write_cmos_sensor(0x0205,(gain&0xff));
	
}   

static void ihdr_write_shutter_gain(kal_uint16 le, kal_uint16 se, kal_uint16 gain)
{
	LOG_INF("le:0x%x, se:0x%x, gain:0x%x\n",le,se,gain);

}



static void set_mirror_flip(kal_uint8 image_mirror)
{
	LOG_INF("image_mirror = %d\n", image_mirror);

	
	switch (image_mirror)
    {
        case IMAGE_NORMAL: 
            write_cmos_sensor(0x0101, 0x03);	
            break;
        case IMAGE_V_MIRROR: 
            write_cmos_sensor(0x0101, 0x01);	
            break;
        case IMAGE_H_MIRROR: 
            write_cmos_sensor(0x0101, 0x02);	
            break;
        case IMAGE_HV_MIRROR: 
            write_cmos_sensor(0x0101, 0x00);	
            break;
    }

}

static void night_mode(kal_bool enable)
{
 
}	

static void sensor_init(void)
{
}   


static void preview_setting(void)
{ 
		write_cmos_sensor(0x0100,0x00);
		write_cmos_sensor(0x0101,0x03);
		write_cmos_sensor(0x0204,0x00);
		write_cmos_sensor(0x0205,0x20);
		
		
		
		
		
		
		write_cmos_sensor(0x0344,0x00);
		write_cmos_sensor(0x0345,0x00);
		write_cmos_sensor(0x0346,0x00);
		write_cmos_sensor(0x0347,0x00);
		write_cmos_sensor(0x0348,0x0C);
		write_cmos_sensor(0x0349,0xD1);
		write_cmos_sensor(0x034A,0x09);
		write_cmos_sensor(0x034B,0x9F);
		write_cmos_sensor(0x034C,0x06);
		write_cmos_sensor(0x034D,0x68);
		write_cmos_sensor(0x034E,0x04);
		write_cmos_sensor(0x034F,0xD0);
		write_cmos_sensor(0x0390,0x01);
		write_cmos_sensor(0x0391,0x22);
		write_cmos_sensor(0x0940,0x00);
		write_cmos_sensor(0x0381,0x01);
		write_cmos_sensor(0x0383,0x03);
		write_cmos_sensor(0x0385,0x01);
		write_cmos_sensor(0x0387,0x03);
		write_cmos_sensor(0x0301,0x04);
		write_cmos_sensor(0x0303,0x01);
		write_cmos_sensor(0x0305,0x06);
		write_cmos_sensor(0x0306,0x00);
		write_cmos_sensor(0x0307,0x8C);
		write_cmos_sensor(0x0309,0x02);
		write_cmos_sensor(0x030B,0x01);
		write_cmos_sensor(0x3C59,0x00);
		write_cmos_sensor(0x030D,0x06);
		write_cmos_sensor(0x030E,0x00);
		write_cmos_sensor(0x030F,0xAF);
		write_cmos_sensor(0x3C5A,0x00);
		write_cmos_sensor(0x0310,0x01);
		write_cmos_sensor(0x3C50,0x53);
		write_cmos_sensor(0x3C62,0x02);
		write_cmos_sensor(0x3C63,0xBC);
		write_cmos_sensor(0x3C64,0x00);
		write_cmos_sensor(0x3C65,0x00);
		#ifdef USE_MIPI_2_LANES
		write_cmos_sensor(0x0114,0x01);
		#else
		write_cmos_sensor(0x0114,0x03);
		#endif
		write_cmos_sensor(0x3C1E,0x0F);
		write_cmos_sensor(0x3500,0x0C);
		write_cmos_sensor(0x3C1A,0xA8);
		write_cmos_sensor(0x3B29,0x01);
		write_cmos_sensor(0x3300,0x01);
		write_cmos_sensor(0x3000,0x07);
		write_cmos_sensor(0x3001,0x05);
		write_cmos_sensor(0x3002,0x03);
		write_cmos_sensor(0x0200,0x0C);
		write_cmos_sensor(0x0201,0xB4);
		write_cmos_sensor(0x300A,0x03);
		write_cmos_sensor(0x300C,0x65);
		write_cmos_sensor(0x300D,0x54);
		write_cmos_sensor(0x3010,0x00);
		write_cmos_sensor(0x3012,0x14);
		write_cmos_sensor(0x3014,0x19);
		write_cmos_sensor(0x3017,0x0F);
		write_cmos_sensor(0x3018,0x1A);
		write_cmos_sensor(0x3019,0x6C);
		write_cmos_sensor(0x301A,0x78);
		write_cmos_sensor(0x306F,0x00);
		write_cmos_sensor(0x3070,0x00);
		write_cmos_sensor(0x3071,0x00);
		write_cmos_sensor(0x3072,0x00);
		write_cmos_sensor(0x3073,0x00);
		write_cmos_sensor(0x3074,0x00);
		write_cmos_sensor(0x3075,0x00);
		write_cmos_sensor(0x3076,0x0A);
		write_cmos_sensor(0x3077,0x03);
		write_cmos_sensor(0x3078,0x84);
		write_cmos_sensor(0x3079,0x00);
		write_cmos_sensor(0x307A,0x00);
		write_cmos_sensor(0x307B,0x00);
		write_cmos_sensor(0x307C,0x00);
		write_cmos_sensor(0x3085,0x00);
		write_cmos_sensor(0x3086,0x72);
		write_cmos_sensor(0x30A6,0x01);
		write_cmos_sensor(0x30A7,0x0E);
		write_cmos_sensor(0x3032,0x01);
		write_cmos_sensor(0x3037,0x02);
		write_cmos_sensor(0x304A,0x01);
		write_cmos_sensor(0x3054,0xF0);
		write_cmos_sensor(0x3044,0x20);
		write_cmos_sensor(0x3045,0x20);
		write_cmos_sensor(0x3047,0x04);
		write_cmos_sensor(0x3048,0x11);
		write_cmos_sensor(0x303D,0x08);
		write_cmos_sensor(0x304B,0x31);
		write_cmos_sensor(0x3063,0x00);
		write_cmos_sensor(0x303A,0x0B);
		write_cmos_sensor(0x302D,0x7F);
		write_cmos_sensor(0x3039,0x45);
		write_cmos_sensor(0x3038,0x10);
		write_cmos_sensor(0x3097,0x11);
		write_cmos_sensor(0x3096,0x01);
		write_cmos_sensor(0x3042,0x01);
		write_cmos_sensor(0x3053,0x01);
		write_cmos_sensor(0x320B,0x40);
		write_cmos_sensor(0x320C,0x06);
		write_cmos_sensor(0x320D,0xC0);
		write_cmos_sensor(0x3202,0x00);
		write_cmos_sensor(0x3203,0x3D);
		write_cmos_sensor(0x3204,0x00);
		write_cmos_sensor(0x3205,0x3D);
		write_cmos_sensor(0x3206,0x00);
		write_cmos_sensor(0x3207,0x3D);
		write_cmos_sensor(0x3208,0x00);
		write_cmos_sensor(0x3209,0x3D);
		write_cmos_sensor(0x3211,0x02);
		write_cmos_sensor(0x3212,0x21);
		write_cmos_sensor(0x3213,0x02);
		write_cmos_sensor(0x3214,0x21);
		write_cmos_sensor(0x3215,0x02);
		write_cmos_sensor(0x3216,0x21);
		write_cmos_sensor(0x3217,0x02);
		write_cmos_sensor(0x3218,0x21);
		write_cmos_sensor(0x0100,0x01);
	}   

static void capture_setting(kal_uint16 currefps)
{
    if (0) 
	{ 
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	write_cmos_sensor(0x0100,0x00); 	
	
	
	write_cmos_sensor(0x0101,0x03); 	
	
	
	
	
	
	
	
	
			  
	
	write_cmos_sensor(0x0340,0x09); 	
	write_cmos_sensor(0x0341,0xD0); 	
	write_cmos_sensor(0x0342,0x0E); 	
	write_cmos_sensor(0x0343,0x68); 	
	
	
	write_cmos_sensor(0x0344,0x00); 	
	write_cmos_sensor(0x0345,0x00); 	
	write_cmos_sensor(0x0346,0x00); 	
	write_cmos_sensor(0x0347,0x00); 	
	write_cmos_sensor(0x0348,0x0C); 	
	write_cmos_sensor(0x0349,0xCF); 	
	write_cmos_sensor(0x034A,0x09); 	
	write_cmos_sensor(0x034B,0x9F); 	
	write_cmos_sensor(0x034C,0x0C); 	
	write_cmos_sensor(0x034D,0xD0); 	
	write_cmos_sensor(0x034E,0x09); 	
	write_cmos_sensor(0x034F,0xA0); 	
	
	
	write_cmos_sensor(0x0390,0x00); 	
	write_cmos_sensor(0x0391,0x00); 	
	
	
	write_cmos_sensor(0x0940,0x00); 	
	
	
	write_cmos_sensor(0x0381,0x01); 	
	write_cmos_sensor(0x0383,0x01); 	
	write_cmos_sensor(0x0385,0x01); 	
	write_cmos_sensor(0x0387,0x01); 	
	
	
	write_cmos_sensor(0x0301,0x04); 	
	write_cmos_sensor(0x0303,0x01); 	
	write_cmos_sensor(0x0305,0x06); 	
	write_cmos_sensor(0x0306,0x00); 	
	write_cmos_sensor(0x0307,0xe0); 	
	write_cmos_sensor(0x0309,0x02); 	
	write_cmos_sensor(0x030B,0x01); 	
	write_cmos_sensor(0x3C59,0x00); 	
	write_cmos_sensor(0x030D,0x06); 	
	write_cmos_sensor(0x030E,0x00); 	
	write_cmos_sensor(0x030F,0x8c); 	
	write_cmos_sensor(0x3C5A,0x00); 	
	write_cmos_sensor(0x0310,0x01); 	
	write_cmos_sensor(0x3C50,0x83); 	
			
	write_cmos_sensor(0x3C62,0x02); 	
	write_cmos_sensor(0x3C63,0x30); 	
	write_cmos_sensor(0x3C64,0x00); 	
	write_cmos_sensor(0x3C65,0x00); 	
	
	
		write_cmos_sensor(0x0114,0x03);
	write_cmos_sensor(0x3C1E,0x0F); 	
			
			
			
	
	
	
	write_cmos_sensor(0x3500,0x0C); 	
		write_cmos_sensor(0x3C1A,0xA8);
	
		write_cmos_sensor(0x3B29,0x01);
		write_cmos_sensor(0x3300,0x01);
	
	write_cmos_sensor(0x3000,0x07);    
	write_cmos_sensor(0x3001,0x05);    
	write_cmos_sensor(0x3002,0x03);    
		write_cmos_sensor(0x0200,0x0C);
	write_cmos_sensor(0x0201,0xB4);    
	write_cmos_sensor(0x300A,0x03);    
	write_cmos_sensor(0x300C,0x65);    
	write_cmos_sensor(0x300D,0x54);    
	write_cmos_sensor(0x3010,0x00);    
	write_cmos_sensor(0x3012,0x14);    
	write_cmos_sensor(0x3014,0x19);    
	write_cmos_sensor(0x3017,0x0F);    
	write_cmos_sensor(0x3018,0x1A);    
	write_cmos_sensor(0x3019,0x6C);    
	write_cmos_sensor(0x301A,0x78);    
	write_cmos_sensor(0x306F,0x00);    
	write_cmos_sensor(0x3070,0x00);    
	write_cmos_sensor(0x3071,0x00);    
	write_cmos_sensor(0x3072,0x00);    
	write_cmos_sensor(0x3073,0x00);    
	write_cmos_sensor(0x3074,0x00);    
	write_cmos_sensor(0x3075,0x00);    
	write_cmos_sensor(0x3076,0x0A);    
	write_cmos_sensor(0x3077,0x03);    
	write_cmos_sensor(0x3078,0x84);    
	write_cmos_sensor(0x3079,0x00);    
	write_cmos_sensor(0x307A,0x00);    
	write_cmos_sensor(0x307B,0x00);    
	write_cmos_sensor(0x307C,0x00);    
	write_cmos_sensor(0x3085,0x00);    
	write_cmos_sensor(0x3086,0x72);    
	write_cmos_sensor(0x30A6,0x01);    
	write_cmos_sensor(0x30A7,0x0E);    
	write_cmos_sensor(0x3032,0x01);    
	write_cmos_sensor(0x3037,0x02);    
	write_cmos_sensor(0x304A,0x01);    
	write_cmos_sensor(0x3054,0xF0);    
	write_cmos_sensor(0x3044,0x20);    
	write_cmos_sensor(0x3045,0x20);    
	write_cmos_sensor(0x3047,0x04);    
	write_cmos_sensor(0x3048,0x11);    
	write_cmos_sensor(0x303D,0x08);    
	write_cmos_sensor(0x304B,0x31);    
	write_cmos_sensor(0x3063,0x00);    
	write_cmos_sensor(0x303A,0x0B);    
	write_cmos_sensor(0x302D,0x7F);    
	write_cmos_sensor(0x3039,0x45);    
	write_cmos_sensor(0x3038,0x10);    
	write_cmos_sensor(0x3097,0x11);    
	write_cmos_sensor(0x3096,0x01);    
	write_cmos_sensor(0x3042,0x01);    
	write_cmos_sensor(0x3053,0x01);    
	write_cmos_sensor(0x320B,0x40);    
	write_cmos_sensor(0x320C,0x06);    
	write_cmos_sensor(0x320D,0xC0);    
	write_cmos_sensor(0x3202,0x00);    
	write_cmos_sensor(0x3203,0x3D);    
	write_cmos_sensor(0x3204,0x00);    
	write_cmos_sensor(0x3205,0x3D);    
	write_cmos_sensor(0x3206,0x00);    
	write_cmos_sensor(0x3207,0x3D);    
	write_cmos_sensor(0x3208,0x00);    
	write_cmos_sensor(0x3209,0x3D);    
	write_cmos_sensor(0x3211,0x02);    
	write_cmos_sensor(0x3212,0x21);    
	write_cmos_sensor(0x3213,0x02);    
	write_cmos_sensor(0x3214,0x21);    
	write_cmos_sensor(0x3215,0x02);    
	write_cmos_sensor(0x3216,0x21);    
	write_cmos_sensor(0x3217,0x02);    
	write_cmos_sensor(0x3218,0x21);    
	
	 
	
	write_cmos_sensor(0x0100,0x01);    
	
    }
	else
	{ 
		write_cmos_sensor(0x0100,0x00);
		write_cmos_sensor(0x0101,0x03);
		write_cmos_sensor(0x0204,0x00);
		write_cmos_sensor(0x0205,0x20);
		
		
		write_cmos_sensor(0x0202,0x04);
		write_cmos_sensor(0x0203,0xE2);
		write_cmos_sensor(0x0340,0x09);
		write_cmos_sensor(0x0341,0xD0);
		write_cmos_sensor(0x0342,0x0E);
		write_cmos_sensor(0x0343,0x68);
		write_cmos_sensor(0x0344,0x00);
		write_cmos_sensor(0x0345,0x00);
		write_cmos_sensor(0x0346,0x00);
		write_cmos_sensor(0x0347,0x00);
		write_cmos_sensor(0x0348,0x0C);
		write_cmos_sensor(0x0349,0xCF);
		write_cmos_sensor(0x034A,0x09);
		write_cmos_sensor(0x034B,0x9F);
		write_cmos_sensor(0x034C,0x0C);
		write_cmos_sensor(0x034D,0xD0);
		write_cmos_sensor(0x034E,0x09);
		write_cmos_sensor(0x034F,0xA0);
		write_cmos_sensor(0x0390,0x00);
		write_cmos_sensor(0x0391,0x00);
		write_cmos_sensor(0x0940,0x00);
		write_cmos_sensor(0x0381,0x01);
		write_cmos_sensor(0x0383,0x01);
		write_cmos_sensor(0x0385,0x01);
		write_cmos_sensor(0x0387,0x01);
		write_cmos_sensor(0x0301,0x04);
		write_cmos_sensor(0x0303,0x01);
		write_cmos_sensor(0x0305,0x06);
		write_cmos_sensor(0x0306,0x00);
		write_cmos_sensor(0x0307,0x8C);
		write_cmos_sensor(0x0309,0x02);
		write_cmos_sensor(0x030B,0x01);
		write_cmos_sensor(0x3C59,0x00);
		write_cmos_sensor(0x030D,0x06);
		write_cmos_sensor(0x030E,0x00);
		write_cmos_sensor(0x030F,0xAF);
		write_cmos_sensor(0x3C5A,0x00);
		write_cmos_sensor(0x0310,0x01);
		write_cmos_sensor(0x3C50,0x53);
		write_cmos_sensor(0x3C62,0x02);
		write_cmos_sensor(0x3C63,0xBC);
		write_cmos_sensor(0x3C64,0x00);
		write_cmos_sensor(0x3C65,0x00);


        
			
			
			
			
					#ifdef USE_MIPI_2_LANES
		write_cmos_sensor(0x0114,0x01);
		#else
		write_cmos_sensor(0x0114,0x03);
		#endif
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
			
	
	
	write_cmos_sensor(0x3C1E,0x0F); 	
			
			
			
	
	
	
	write_cmos_sensor(0x3500,0x0C); 	
		write_cmos_sensor(0x3C1A,0xA8);
		write_cmos_sensor(0x3B29,0x01);
		write_cmos_sensor(0x3300,0x01);
	
	
	write_cmos_sensor(0x3000,0x07);    
	write_cmos_sensor(0x3001,0x05);    
	write_cmos_sensor(0x3002,0x03);    
		write_cmos_sensor(0x0200,0x0C);
	write_cmos_sensor(0x0201,0xB4);    
	write_cmos_sensor(0x300A,0x03);    
	write_cmos_sensor(0x300C,0x65);    
	write_cmos_sensor(0x300D,0x54);    
	write_cmos_sensor(0x3010,0x00);    
	write_cmos_sensor(0x3012,0x14);    
	write_cmos_sensor(0x3014,0x19);    
	write_cmos_sensor(0x3017,0x0F);    
	write_cmos_sensor(0x3018,0x1A);    
	write_cmos_sensor(0x3019,0x6C);    
	write_cmos_sensor(0x301A,0x78);    
	write_cmos_sensor(0x306F,0x00);    
	write_cmos_sensor(0x3070,0x00);    
	write_cmos_sensor(0x3071,0x00);    
	write_cmos_sensor(0x3072,0x00);    
	write_cmos_sensor(0x3073,0x00);    
	write_cmos_sensor(0x3074,0x00);    
	write_cmos_sensor(0x3075,0x00);    
	write_cmos_sensor(0x3076,0x0A);    
	write_cmos_sensor(0x3077,0x03);    
	write_cmos_sensor(0x3078,0x84);    
	write_cmos_sensor(0x3079,0x00);    
	write_cmos_sensor(0x307A,0x00);    
	write_cmos_sensor(0x307B,0x00);    
	write_cmos_sensor(0x307C,0x00);    
	write_cmos_sensor(0x3085,0x00);    
	write_cmos_sensor(0x3086,0x72);    
	write_cmos_sensor(0x30A6,0x01);    
	write_cmos_sensor(0x30A7,0x0E);    
	write_cmos_sensor(0x3032,0x01);    
	write_cmos_sensor(0x3037,0x02);    
	write_cmos_sensor(0x304A,0x01);    
	write_cmos_sensor(0x3054,0xF0);    
	write_cmos_sensor(0x3044,0x20);    
	write_cmos_sensor(0x3045,0x20);    
	write_cmos_sensor(0x3047,0x04);    
	write_cmos_sensor(0x3048,0x11);    
	write_cmos_sensor(0x303D,0x08);    
	write_cmos_sensor(0x304B,0x31);    
	write_cmos_sensor(0x3063,0x00);    
	write_cmos_sensor(0x303A,0x0B);    
	write_cmos_sensor(0x302D,0x7F);    
	write_cmos_sensor(0x3039,0x45);    
	write_cmos_sensor(0x3038,0x10);    
	write_cmos_sensor(0x3097,0x11);    
	write_cmos_sensor(0x3096,0x01);    
	write_cmos_sensor(0x3042,0x01);    
	write_cmos_sensor(0x3053,0x01);    
	write_cmos_sensor(0x320B,0x40);    
	write_cmos_sensor(0x320C,0x06);    
	write_cmos_sensor(0x320D,0xC0);    
	write_cmos_sensor(0x3202,0x00);    
	write_cmos_sensor(0x3203,0x3D);    
	write_cmos_sensor(0x3204,0x00);    
	write_cmos_sensor(0x3205,0x3D);    
	write_cmos_sensor(0x3206,0x00);    
	write_cmos_sensor(0x3207,0x3D);    
	write_cmos_sensor(0x3208,0x00);    
	write_cmos_sensor(0x3209,0x3D);    
	write_cmos_sensor(0x3211,0x02);    
	write_cmos_sensor(0x3212,0x21);    
	write_cmos_sensor(0x3213,0x02);    
	write_cmos_sensor(0x3214,0x21);    
	write_cmos_sensor(0x3215,0x02);    
	write_cmos_sensor(0x3216,0x21);    
	write_cmos_sensor(0x3217,0x02);    
	write_cmos_sensor(0x3218,0x21);    
		write_cmos_sensor(0x0100,0x01);
	}
}
static void normal_video_setting(kal_uint16 currefps)
	{ 
			LOG_INF("E! currefps:%d\n",currefps);
			write_cmos_sensor(0x0100,0x00);
		write_cmos_sensor(0x0101,0x03);
			write_cmos_sensor(0x0204,0x00);
			write_cmos_sensor(0x0205,0x20);
			write_cmos_sensor(0x0342,0x0E);
			write_cmos_sensor(0x0343,0x68);
			write_cmos_sensor(0x0344,0x00);
			write_cmos_sensor(0x0345,0x00);
			write_cmos_sensor(0x0346,0x00);
			write_cmos_sensor(0x0347,0x00);
			write_cmos_sensor(0x0348,0x0C);
			write_cmos_sensor(0x0349,0xD1);
			write_cmos_sensor(0x034A,0x09);
			write_cmos_sensor(0x034B,0x9F);
			write_cmos_sensor(0x034C,0x06);
			write_cmos_sensor(0x034D,0x68);
			write_cmos_sensor(0x034E,0x04);
			write_cmos_sensor(0x034F,0xD0);
			write_cmos_sensor(0x0390,0x01);
			write_cmos_sensor(0x0391,0x22);
			write_cmos_sensor(0x0940,0x00);
			write_cmos_sensor(0x0381,0x01);
			write_cmos_sensor(0x0383,0x03);
			write_cmos_sensor(0x0385,0x01);
			write_cmos_sensor(0x0387,0x03);
			write_cmos_sensor(0x0301,0x04);
			write_cmos_sensor(0x0303,0x01);
			write_cmos_sensor(0x0305,0x06);
			write_cmos_sensor(0x0306,0x00);
			write_cmos_sensor(0x0307,0x8C);
			write_cmos_sensor(0x0309,0x02);
			write_cmos_sensor(0x030B,0x01);
			write_cmos_sensor(0x3C59,0x00);
			write_cmos_sensor(0x030D,0x06);
			write_cmos_sensor(0x030E,0x00);
			write_cmos_sensor(0x030F,0xAF);
			write_cmos_sensor(0x3C5A,0x00);
			write_cmos_sensor(0x0310,0x01);
			write_cmos_sensor(0x3C50,0x53);
			write_cmos_sensor(0x3C62,0x02);
			write_cmos_sensor(0x3C63,0xBC);
			write_cmos_sensor(0x3C64,0x00);
			write_cmos_sensor(0x3C65,0x00);
		#ifdef USE_MIPI_2_LANES
		write_cmos_sensor(0x0114,0x01);
		#else
		write_cmos_sensor(0x0114,0x03);
		#endif
		write_cmos_sensor(0x3C1E,0x0F);
		write_cmos_sensor(0x3500,0x0C);
		write_cmos_sensor(0x3C1A,0xA8);
	write_cmos_sensor(0x3B29,0x01); 	 
		write_cmos_sensor(0x3300,0x01);
		write_cmos_sensor(0x3000,0x07);
		write_cmos_sensor(0x3001,0x05);
		write_cmos_sensor(0x3002,0x03);
		write_cmos_sensor(0x0200,0x0C);
		write_cmos_sensor(0x0201,0xB4);
		write_cmos_sensor(0x300A,0x03);
		write_cmos_sensor(0x300C,0x65);
		write_cmos_sensor(0x300D,0x54);
		write_cmos_sensor(0x3010,0x00);
		write_cmos_sensor(0x3012,0x14);
		write_cmos_sensor(0x3014,0x19);
		write_cmos_sensor(0x3017,0x0F);
		write_cmos_sensor(0x3018,0x1A);
		write_cmos_sensor(0x3019,0x6C);
		write_cmos_sensor(0x301A,0x78);
		write_cmos_sensor(0x306F,0x00);
		write_cmos_sensor(0x3070,0x00);
		write_cmos_sensor(0x3071,0x00);
		write_cmos_sensor(0x3072,0x00);
		write_cmos_sensor(0x3073,0x00);
		write_cmos_sensor(0x3074,0x00);
		write_cmos_sensor(0x3075,0x00);
		write_cmos_sensor(0x3076,0x0A);
		write_cmos_sensor(0x3077,0x03);
		write_cmos_sensor(0x3078,0x84);
		write_cmos_sensor(0x3079,0x00);
		write_cmos_sensor(0x307A,0x00);
		write_cmos_sensor(0x307B,0x00);
		write_cmos_sensor(0x307C,0x00);
		write_cmos_sensor(0x3085,0x00);
		write_cmos_sensor(0x3086,0x72);
		write_cmos_sensor(0x30A6,0x01);
		write_cmos_sensor(0x30A7,0x0E);
		write_cmos_sensor(0x3032,0x01);
		write_cmos_sensor(0x3037,0x02);
		write_cmos_sensor(0x304A,0x01);
		write_cmos_sensor(0x3054,0xF0);
		write_cmos_sensor(0x3044,0x20);
		write_cmos_sensor(0x3045,0x20);
		write_cmos_sensor(0x3047,0x04);
		write_cmos_sensor(0x3048,0x11);
		write_cmos_sensor(0x303D,0x08);
		write_cmos_sensor(0x304B,0x31);
		write_cmos_sensor(0x3063,0x00);
		write_cmos_sensor(0x303A,0x0B);
		write_cmos_sensor(0x302D,0x7F);
		write_cmos_sensor(0x3039,0x45);
		write_cmos_sensor(0x3038,0x10);
		write_cmos_sensor(0x3097,0x11);
		write_cmos_sensor(0x3096,0x01);
		write_cmos_sensor(0x3042,0x01);
		write_cmos_sensor(0x3053,0x01);
		write_cmos_sensor(0x320B,0x40);
		write_cmos_sensor(0x320C,0x06);
		write_cmos_sensor(0x320D,0xC0);
		write_cmos_sensor(0x3202,0x00);
		write_cmos_sensor(0x3203,0x3D);
		write_cmos_sensor(0x3204,0x00);
		write_cmos_sensor(0x3205,0x3D);
		write_cmos_sensor(0x3206,0x00);
		write_cmos_sensor(0x3207,0x3D);
		write_cmos_sensor(0x3208,0x00);
		write_cmos_sensor(0x3209,0x3D);
		write_cmos_sensor(0x3211,0x02);
		write_cmos_sensor(0x3212,0x21);
		write_cmos_sensor(0x3213,0x02);
		write_cmos_sensor(0x3214,0x21);
		write_cmos_sensor(0x3215,0x02);
		write_cmos_sensor(0x3216,0x21);
		write_cmos_sensor(0x3217,0x02);
		write_cmos_sensor(0x3218,0x21);
			write_cmos_sensor(0x0100,0x01);
		}
static void hs_video_setting() 
{              LOG_INF("E//VGA 120fps");
				
			
			
			
			
			
			
			
			
			
			                                                                                                      
			

			write_cmos_sensor(0x0100,0x00);       
			write_cmos_sensor(0x0101,0x03);       
			                       
			write_cmos_sensor(0x0204,0x00);       
			write_cmos_sensor(0x0205,0x20);     	
			                       
			write_cmos_sensor(0x0200,0x0C);       
			write_cmos_sensor(0x0201,0x98);     	
			write_cmos_sensor(0x0202,0x04);     	
			write_cmos_sensor(0x0203,0xE2);     	
			                  
			write_cmos_sensor(0x0340,0x02);       
			write_cmos_sensor(0x0341,0x80);     	
			write_cmos_sensor(0x0342,0x0E);     	
			write_cmos_sensor(0x0343,0x68);     	
			                  
			write_cmos_sensor(0x0344,0x00);       
			write_cmos_sensor(0x0345,0x00);     	
			write_cmos_sensor(0x0346,0x00);     	
			write_cmos_sensor(0x0347,0x00);     	
			write_cmos_sensor(0x0348,0x0C);     	
			write_cmos_sensor(0x0349,0xCF);     	
			write_cmos_sensor(0x034A,0x09);     	
			write_cmos_sensor(0x034B,0x9F);     	
			                  
			write_cmos_sensor(0x034C,0x03);     	
			write_cmos_sensor(0x034D,0x34);     	
			write_cmos_sensor(0x034E,0x02);     	
			write_cmos_sensor(0x034F,0x68);     	
			                  
			write_cmos_sensor(0x0390,0x01);       
			write_cmos_sensor(0x0391,0x44);     	
			write_cmos_sensor(0x0940,0x00);       
			                  
			write_cmos_sensor(0x0381,0x01);       
			write_cmos_sensor(0x0383,0x07);     	
			write_cmos_sensor(0x0385,0x01);     	
			write_cmos_sensor(0x0387,0x07);     	
			                  
					#ifdef USE_MIPI_2_LANES
		write_cmos_sensor(0x0114,0x01);
		#else
		write_cmos_sensor(0x0114,0x03);
		#endif       
			                  
			write_cmos_sensor(0x0301,0x04);       
			write_cmos_sensor(0x0303,0x01);     	
			write_cmos_sensor(0x0305,0x06);     	
			write_cmos_sensor(0x0306,0x00);     	
			write_cmos_sensor(0x0307,0x8C);     	
			write_cmos_sensor(0x0309,0x02);     	
			write_cmos_sensor(0x030B,0x01);     	
			write_cmos_sensor(0x3C59,0x00);     	
			write_cmos_sensor(0x030D,0x06);     	
			write_cmos_sensor(0x030E,0x00);     	
			write_cmos_sensor(0x030F,0xAF);     	
			write_cmos_sensor(0x3C5A,0x00);     	
			write_cmos_sensor(0x0310,0x01);     	
			write_cmos_sensor(0x3C50,0x53);     	
			write_cmos_sensor(0x3C62,0x02);       
			write_cmos_sensor(0x3C63,0xBC);     	
			write_cmos_sensor(0x3C64,0x00);     	
			write_cmos_sensor(0x3C65,0x00);     	
			                  
			write_cmos_sensor(0x3C1E,0x0F);       
			    	
			    	
			    	
			                  
			write_cmos_sensor(0x3500,0x0C);       
			                  
			write_cmos_sensor(0x3C1A,0xEC);       
			                  
			write_cmos_sensor(0x3B29,0x01);       
			
			                  
			                  
			
			write_cmos_sensor(0x3000,0x07);
			write_cmos_sensor(0x3001,0x05);
			write_cmos_sensor(0x3002,0x03);
			write_cmos_sensor(0x0200,0x0C);
			write_cmos_sensor(0x0201,0xB4);
			write_cmos_sensor(0x300A,0x03);
			write_cmos_sensor(0x300C,0x65);
			write_cmos_sensor(0x300D,0x54);
			write_cmos_sensor(0x3010,0x00);
			write_cmos_sensor(0x3012,0x14);
			write_cmos_sensor(0x3014,0x19);
			write_cmos_sensor(0x3017,0x0F);
			write_cmos_sensor(0x3018,0x1A);
			write_cmos_sensor(0x3019,0x6C);
			write_cmos_sensor(0x301A,0x78);
			write_cmos_sensor(0x306F,0x00);
			write_cmos_sensor(0x3070,0x00);
			write_cmos_sensor(0x3071,0x00);
			write_cmos_sensor(0x3072,0x00);
			write_cmos_sensor(0x3073,0x00);
			write_cmos_sensor(0x3074,0x00);
			write_cmos_sensor(0x3075,0x00);
			write_cmos_sensor(0x3076,0x0A);
			write_cmos_sensor(0x3077,0x03);
			write_cmos_sensor(0x3078,0x84);
			write_cmos_sensor(0x3079,0x00);
			write_cmos_sensor(0x307A,0x00);
			write_cmos_sensor(0x307B,0x00);
			write_cmos_sensor(0x307C,0x00);
			write_cmos_sensor(0x3085,0x00);
			write_cmos_sensor(0x3086,0x72);
			write_cmos_sensor(0x30A6,0x01);
			write_cmos_sensor(0x30A7,0x0E);
			write_cmos_sensor(0x3032,0x01);
			write_cmos_sensor(0x3037,0x02);
			write_cmos_sensor(0x304A,0x01);
			write_cmos_sensor(0x3054,0xF0);
			write_cmos_sensor(0x3044,0x20);
			write_cmos_sensor(0x3045,0x20);
			write_cmos_sensor(0x3047,0x04);
			write_cmos_sensor(0x3048,0x11);
			write_cmos_sensor(0x303D,0x08);
			write_cmos_sensor(0x304B,0x31);
			write_cmos_sensor(0x3063,0x00);
			write_cmos_sensor(0x303A,0x0B);
			write_cmos_sensor(0x302D,0x7F);
			write_cmos_sensor(0x3039,0x45);
			write_cmos_sensor(0x3038,0x10);
			write_cmos_sensor(0x3097,0x11);
			write_cmos_sensor(0x3096,0x01);
			write_cmos_sensor(0x3042,0x01);
			write_cmos_sensor(0x3053,0x01);
			write_cmos_sensor(0x320B,0x40);
			write_cmos_sensor(0x320C,0x06);
			write_cmos_sensor(0x320D,0xC0);
			write_cmos_sensor(0x3202,0x00);
			write_cmos_sensor(0x3203,0x3D);
			write_cmos_sensor(0x3204,0x00);
			write_cmos_sensor(0x3205,0x3D);
			write_cmos_sensor(0x3206,0x00);
			write_cmos_sensor(0x3207,0x3D);
			write_cmos_sensor(0x3208,0x00);
			write_cmos_sensor(0x3209,0x3D);
			write_cmos_sensor(0x3211,0x02);
			write_cmos_sensor(0x3212,0x21);
			write_cmos_sensor(0x3213,0x02);
			write_cmos_sensor(0x3214,0x21);
			write_cmos_sensor(0x3215,0x02);
			write_cmos_sensor(0x3216,0x21);
			write_cmos_sensor(0x3217,0x02);
			write_cmos_sensor(0x3218,0x21);
			                  
			write_cmos_sensor(0x0100,0x01);
	}

static void slim_video_setting()
	{
		LOG_INF("E");
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		write_cmos_sensor(0x0100,0x00); 	
		
		
		write_cmos_sensor(0x0101,0x03); 	
		
		
		write_cmos_sensor(0x0204,0x00); 	
		write_cmos_sensor(0x0205,0x20); 	
		
		
		write_cmos_sensor(0x0202,0x04); 	
		write_cmos_sensor(0x0203,0xE2); 	
		
		
		write_cmos_sensor(0x0340,0x05); 	
		write_cmos_sensor(0x0341,0x00); 	
		write_cmos_sensor(0x0342,0x0E); 	
		write_cmos_sensor(0x0343,0x68); 	
		
		
		write_cmos_sensor(0x0344,0x01); 	
		write_cmos_sensor(0x0345,0x68); 	
		write_cmos_sensor(0x0346,0x02); 	
		write_cmos_sensor(0x0347,0x00); 	
		write_cmos_sensor(0x0348,0x0B); 	
		write_cmos_sensor(0x0349,0x69); 	
		write_cmos_sensor(0x034A,0x07); 	
		write_cmos_sensor(0x034B,0x9F); 	
		write_cmos_sensor(0x034C,0x05); 	
		write_cmos_sensor(0x034D,0x00); 	
		write_cmos_sensor(0x034E,0x02); 	
		write_cmos_sensor(0x034F,0xD0); 	
		
		
		write_cmos_sensor(0x0390,0x01); 	
		write_cmos_sensor(0x0391,0x22); 	
		
		
		write_cmos_sensor(0x0940,0x00); 	
		
		
		write_cmos_sensor(0x0381,0x01); 	
		write_cmos_sensor(0x0383,0x03); 	
		write_cmos_sensor(0x0385,0x01); 	
		write_cmos_sensor(0x0387,0x03); 	
					#ifdef USE_MIPI_2_LANES
		write_cmos_sensor(0x0114,0x01);
		#else
		write_cmos_sensor(0x0114,0x03);
		#endif       
		
		
		write_cmos_sensor(0x0301,0x04); 	
		write_cmos_sensor(0x0303,0x01); 	
		write_cmos_sensor(0x0305,0x06); 	
		write_cmos_sensor(0x0306,0x00); 	
		write_cmos_sensor(0x0307,0x8C); 	
		write_cmos_sensor(0x0309,0x02); 	
		write_cmos_sensor(0x030B,0x01); 	
		write_cmos_sensor(0x3C59,0x00); 	
		write_cmos_sensor(0x030D,0x06); 	
		write_cmos_sensor(0x030E,0x00); 	
		write_cmos_sensor(0x030F,0xAF); 	
		write_cmos_sensor(0x3C5A,0x00); 	
		write_cmos_sensor(0x0310,0x01); 	
		write_cmos_sensor(0x3C50,0x53); 	
		
		write_cmos_sensor(0x3C62,0x02); 	
		write_cmos_sensor(0x3C63,0xBC); 	
		write_cmos_sensor(0x3C64,0x00); 	
		write_cmos_sensor(0x3C65,0x00); 	
		
		
		write_cmos_sensor(0x3C1E,0x0F); 	
		
		
		
		
		
		
		write_cmos_sensor(0x3500,0x0C); 	
		
		
		write_cmos_sensor(0x3C1A,0xEC); 	
		
		
		write_cmos_sensor(0x3000,0x07);    
		write_cmos_sensor(0x3001,0x05);    
		write_cmos_sensor(0x3002,0x03);    
		write_cmos_sensor(0x0200,0x0c);    
		write_cmos_sensor(0x0201,0xB4);    
		write_cmos_sensor(0x300A,0x03);    
		write_cmos_sensor(0x300C,0x65);    
		write_cmos_sensor(0x300D,0x54);    
		write_cmos_sensor(0x3010,0x00);    
		write_cmos_sensor(0x3012,0x14);    
		write_cmos_sensor(0x3014,0x19);    
		write_cmos_sensor(0x3017,0x0F);    
		write_cmos_sensor(0x3018,0x1A);    
		write_cmos_sensor(0x3019,0x6C);    
		write_cmos_sensor(0x301A,0x78);    
		write_cmos_sensor(0x306F,0x00);    
		write_cmos_sensor(0x3070,0x00);    
		write_cmos_sensor(0x3071,0x00);    
		write_cmos_sensor(0x3072,0x00);    
		write_cmos_sensor(0x3073,0x00);    
		write_cmos_sensor(0x3074,0x00);    
		write_cmos_sensor(0x3075,0x00);    
		write_cmos_sensor(0x3076,0x0A);    
		write_cmos_sensor(0x3077,0x03);    
		write_cmos_sensor(0x3078,0x84);    
		write_cmos_sensor(0x3079,0x00);    
		write_cmos_sensor(0x307A,0x00);    
		write_cmos_sensor(0x307B,0x00);    
		write_cmos_sensor(0x307C,0x00);    
		write_cmos_sensor(0x3085,0x00);    
		write_cmos_sensor(0x3086,0x72);    
		write_cmos_sensor(0x30A6,0x01);    
		write_cmos_sensor(0x30A7,0x0E);    
		write_cmos_sensor(0x3032,0x01);    
		write_cmos_sensor(0x3037,0x02);    
		write_cmos_sensor(0x304A,0x01);    
		write_cmos_sensor(0x3054,0xF0);    
		write_cmos_sensor(0x3044,0x20);    
		write_cmos_sensor(0x3045,0x20);    
		write_cmos_sensor(0x3047,0x04);    
		write_cmos_sensor(0x3048,0x11);    
		write_cmos_sensor(0x303D,0x08);    
		write_cmos_sensor(0x304B,0x31);    
		write_cmos_sensor(0x3063,0x00);    
		write_cmos_sensor(0x303A,0x0B);    
		write_cmos_sensor(0x302D,0x7F);    
		write_cmos_sensor(0x3039,0x45);    
		write_cmos_sensor(0x3038,0x10);    
		write_cmos_sensor(0x3097,0x11);    
		write_cmos_sensor(0x3096,0x01);    
		write_cmos_sensor(0x3042,0x01);    
		write_cmos_sensor(0x3053,0x01);    
		write_cmos_sensor(0x320B,0x40);    
		write_cmos_sensor(0x320C,0x06);    
		write_cmos_sensor(0x320D,0xC0);    
		write_cmos_sensor(0x3202,0x00);    
		write_cmos_sensor(0x3203,0x3D);    
		write_cmos_sensor(0x3204,0x00);    
		write_cmos_sensor(0x3205,0x3D);    
		write_cmos_sensor(0x3206,0x00);    
		write_cmos_sensor(0x3207,0x3D);    
		write_cmos_sensor(0x3208,0x00);    
		write_cmos_sensor(0x3209,0x3D);    
		write_cmos_sensor(0x3211,0x02);    
		write_cmos_sensor(0x3212,0x21);    
		write_cmos_sensor(0x3213,0x02);    
		write_cmos_sensor(0x3214,0x21);    
		write_cmos_sensor(0x3215,0x02);    
		write_cmos_sensor(0x3216,0x21);    
		write_cmos_sensor(0x3217,0x02);    
		write_cmos_sensor(0x3218,0x21);    
		write_cmos_sensor(0x3B29,0x01); 	 
		
		
		
		write_cmos_sensor(0x0100,0x01);    
		
		}


static const char *s5k4h5Vendor = "Samsung";
static const char *s5k4h5NAME = "s5k4h5";
static const char *s5k4h5Size = "5.0M";

static ssize_t sensor_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;

	sprintf(buf, "%s %s %s\n", s5k4h5Vendor, s5k4h5NAME, s5k4h5Size);
	ret = strlen(buf) + 1;

	return ret;
}

static ssize_t sensor_otp_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
    unsigned short inf_val = 0;
    unsigned short mac_val = 0;
	sprintf(buf, "lenc:%u:%u\n",inf_val,mac_val);
	ret = strlen(buf) + 1;

	return ret;
}


static DEVICE_ATTR(sensor, 0444, sensor_vendor_show, NULL);
static DEVICE_ATTR(otp, 0444, sensor_otp_show, NULL);
static struct kobject *android_s5k4h5;

static int s5k4h5_sysfs_init(void)
{
	int ret ;
	LOG_INF("kobject creat and add\n");
    if(android_s5k4h5 == NULL){
        android_s5k4h5 = kobject_create_and_add("android_camera2", NULL);
        if (android_s5k4h5 == NULL) {
            LOG_INF("subsystem_register failed\n");
            ret = -ENOMEM;
            return ret ;
        }
        LOG_INF("sysfs_create_file\n");
        ret = sysfs_create_file(android_s5k4h5, &dev_attr_sensor.attr);
        if (ret) {
            LOG_INF("sysfs_create_file " \
                    "failed\n");
            kobject_del(android_s5k4h5);
            android_s5k4h5 = NULL;
        }

        ret = sysfs_create_file(android_s5k4h5, &dev_attr_otp.attr);
        if (ret) {
            LOG_INF("sysfs_create_file " \
                    "failed\n");
            kobject_del(android_s5k4h5);
            android_s5k4h5 = NULL;
        }
    }
	return 0 ;
}

static kal_uint32 get_imgsensor_id(UINT32 *sensor_id) 
{
	kal_uint8 i = 0;
    int  retry = 2;

#ifdef SLT_DEVINFO_CMM 
 	s_DEVINFO_ccm =(struct devinfo_struct*) kmalloc(sizeof(struct devinfo_struct), GFP_KERNEL);	
	s_DEVINFO_ccm->device_type = "CCM-S";
	s_DEVINFO_ccm->device_module = "CM9857-A800BF";
	s_DEVINFO_ccm->device_vendor = "Truly";
	s_DEVINFO_ccm->device_ic = "S5K4H5YC";
	s_DEVINFO_ccm->device_version = "Samsung";
	s_DEVINFO_ccm->device_info = "800W";
#endif

	
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			*sensor_id = ((read_cmos_sensor(0x0000) << 8) | read_cmos_sensor(0x0001));
            if (*sensor_id == imgsensor_info.sensor_id){
                LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id,*sensor_id);
                s5k4h5_sysfs_init();
                break;
            }
            LOG_INF("Read sensor id fail, id: 0x%x,0x%x\n", imgsensor.i2c_write_id,*sensor_id);
            retry--;
        } while(retry > 0);
        if (*sensor_id == imgsensor_info.sensor_id){
            LOG_INF("get sensor id successfully  id = 0x%x ",*sensor_id);
            break;
        }else{
            i++;
            retry = 2;
        }
    }

	if (*sensor_id != imgsensor_info.sensor_id) {
		
		*sensor_id = 0xFFFFFFFF;
#ifdef SLT_DEVINFO_CMM 
	s_DEVINFO_ccm->device_used = DEVINFO_UNUSED;
	devinfo_check_add_device(s_DEVINFO_ccm);
#endif
		return ERROR_SENSOR_CONNECT_FAIL;
	}

#ifdef SLT_DEVINFO_CMM
	s_DEVINFO_ccm->device_used = DEVINFO_USED;
	devinfo_check_add_device(s_DEVINFO_ccm);
#endif
	return ERROR_NONE;
}



static kal_uint32 open(void)
{
	
	kal_uint8 i = 0;
	kal_uint8 retry = 1;
	kal_uint16 sensor_id = 0; 
	LOG_INF("PLATFORM:MT6595,MIPI 2LANE\n");
	LOG_INF("preview 1280*960@30fps,864Mbps/lane; video 1280*960@30fps,864Mbps/lane; capture 5M@30fps,864Mbps/lane\n");
	
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = 0x30;
		spin_unlock(&imgsensor_drv_lock);
		do {
			sensor_id = ((read_cmos_sensor(0x0000) << 8) | read_cmos_sensor(0x0001));
			if (sensor_id == imgsensor_info.sensor_id){
                LOG_INF("i2c write id: 0x%x, sensor id: 0x%x,0x%x\n", imgsensor.i2c_write_id,sensor_id);
                break;
			}	
			LOG_INF("Read sensor id fail, id: 0x%x,0x%x\n", imgsensor.i2c_write_id,sensor_id);
            retry--;
		} while(retry > 0);
        if (sensor_id == imgsensor_info.sensor_id){
            LOG_INF("get sensor id successfully  id = 0x%x ",sensor_id);
            break;
        }else{
            i++;
            retry = 2;
        }
    }

	if (imgsensor_info.sensor_id != sensor_id){
        LOG_INF("xxxx get sensor id failed \n");
		return ERROR_SENSOR_CONNECT_FAIL;
    }
	
	sensor_init();

	spin_lock(&imgsensor_drv_lock);

	imgsensor.autoflicker_en= KAL_FALSE;
	imgsensor.sensor_mode = IMGSENSOR_MODE_INIT;
	imgsensor.shutter = 0x3D0;
	imgsensor.gain = 0x100;
	imgsensor.pclk = imgsensor_info.pre.pclk;
	imgsensor.frame_length = imgsensor_info.pre.framelength;
	imgsensor.line_length = imgsensor_info.pre.linelength;
	imgsensor.min_frame_length = imgsensor_info.pre.framelength;
	imgsensor.dummy_pixel = 0;
	imgsensor.dummy_line = 0;
	imgsensor.ihdr_en = 0;
	imgsensor.test_pattern = KAL_FALSE;
	imgsensor.current_fps = imgsensor_info.pre.max_framerate;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}	



static kal_uint32 close(void)
{
	LOG_INF("E\n");

	 
	
	return ERROR_NONE;
}	


static kal_uint32 preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_PREVIEW;
	imgsensor.pclk = imgsensor_info.pre.pclk;
	
	imgsensor.line_length = imgsensor_info.pre.linelength;
	imgsensor.frame_length = imgsensor_info.pre.framelength; 
	imgsensor.min_frame_length = imgsensor_info.pre.framelength;
	imgsensor.current_fps = imgsensor.current_fps;
	
	spin_unlock(&imgsensor_drv_lock);
	preview_setting();
#if defined(OPEN_OTP_FUNCTION)
	mdelay(20);
	s5k4h5yc_otp_update();
#endif
	return ERROR_NONE;
}   

static kal_uint32 capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
						  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CAPTURE;
	if (0) {
		imgsensor.pclk = imgsensor_info.cap1.pclk;
		imgsensor.line_length = imgsensor_info.cap1.linelength;
		imgsensor.frame_length = imgsensor_info.cap1.framelength;  
		imgsensor.min_frame_length = imgsensor_info.cap1.framelength;
		
	} else {
		if (imgsensor.current_fps != imgsensor_info.cap.max_framerate)
			LOG_INF("Warning: current_fps %d fps is not support, so use cap1's setting: %d fps!\n",imgsensor_info.cap1.max_framerate/10);
		imgsensor.pclk = imgsensor_info.cap.pclk;
		imgsensor.line_length = imgsensor_info.cap.linelength;
		imgsensor.frame_length = imgsensor_info.cap.framelength;  
		imgsensor.min_frame_length = imgsensor_info.cap.framelength;
		
	}

	spin_unlock(&imgsensor_drv_lock);

	capture_setting(imgsensor.current_fps); 
#if defined(OPEN_OTP_FUNCTION)
	mdelay(20);
	s5k4h5yc_otp_update();
#endif
	
	return ERROR_NONE;
}	
static kal_uint32 normal_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");
	
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_VIDEO;
	imgsensor.pclk = imgsensor_info.normal_video.pclk;
	imgsensor.line_length = imgsensor_info.normal_video.linelength;
	imgsensor.frame_length = imgsensor_info.normal_video.framelength;  
	imgsensor.min_frame_length = imgsensor_info.normal_video.framelength;
	
	
	spin_unlock(&imgsensor_drv_lock);
	normal_video_setting(imgsensor.current_fps);
#if defined(OPEN_OTP_FUNCTION)
	mdelay(20);
	s5k4h5yc_otp_update();
#endif
	
	return ERROR_NONE;
}   

static kal_uint32 hs_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");
	
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_HIGH_SPEED_VIDEO;
	imgsensor.pclk = imgsensor_info.hs_video.pclk;
	
	imgsensor.line_length = imgsensor_info.hs_video.linelength;
	imgsensor.frame_length = imgsensor_info.hs_video.framelength; 
	imgsensor.min_frame_length = imgsensor_info.hs_video.framelength;
	imgsensor.dummy_line = 0;
	imgsensor.dummy_pixel = 0;
	
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	hs_video_setting();
	return ERROR_NONE;
}   

static kal_uint32 slim_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");
	
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_SLIM_VIDEO;
	imgsensor.pclk = imgsensor_info.slim_video.pclk;
	
	imgsensor.line_length = imgsensor_info.slim_video.linelength;
	imgsensor.frame_length = imgsensor_info.slim_video.framelength; 
	imgsensor.min_frame_length = imgsensor_info.slim_video.framelength;
	imgsensor.dummy_line = 0;
	imgsensor.dummy_pixel = 0;
	
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	slim_video_setting();
	return ERROR_NONE;
}   



static kal_uint32 get_resolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *sensor_resolution)
{
	LOG_INF("E\n");
	sensor_resolution->SensorFullWidth = imgsensor_info.cap.grabwindow_width;
	sensor_resolution->SensorFullHeight = imgsensor_info.cap.grabwindow_height;
	
	sensor_resolution->SensorPreviewWidth = imgsensor_info.pre.grabwindow_width;
	sensor_resolution->SensorPreviewHeight = imgsensor_info.pre.grabwindow_height;

	sensor_resolution->SensorVideoWidth = imgsensor_info.normal_video.grabwindow_width;
	sensor_resolution->SensorVideoHeight = imgsensor_info.normal_video.grabwindow_height;		

	
	sensor_resolution->SensorHighSpeedVideoWidth	 = imgsensor_info.hs_video.grabwindow_width;
	sensor_resolution->SensorHighSpeedVideoHeight	 = imgsensor_info.hs_video.grabwindow_height;
	
	return ERROR_NONE;
}	

static kal_uint32 get_info(MSDK_SCENARIO_ID_ENUM scenario_id,
					  MSDK_SENSOR_INFO_STRUCT *sensor_info,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("scenario_id = %d\n", scenario_id);

	
	
	
	

	sensor_info->SensorClockPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorClockFallingPolarity = SENSOR_CLOCK_POLARITY_LOW; 
	sensor_info->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW; 
	sensor_info->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorInterruptDelayLines = 4; 
	sensor_info->SensorResetActiveHigh = FALSE; 
	sensor_info->SensorResetDelayCount = 5; 

	sensor_info->SensroInterfaceType = imgsensor_info.sensor_interface_type;

	sensor_info->SensorOutputDataFormat = imgsensor_info.sensor_output_dataformat;

	sensor_info->CaptureDelayFrame = imgsensor_info.cap_delay_frame; 
	sensor_info->PreviewDelayFrame = imgsensor_info.pre_delay_frame; 
	sensor_info->VideoDelayFrame = imgsensor_info.video_delay_frame;

	sensor_info->SensorMasterClockSwitch = 0; 
	sensor_info->SensorDrivingCurrent = imgsensor_info.isp_driving_current;
	
	sensor_info->AEShutDelayFrame = imgsensor_info.ae_shut_delay_frame; 		 
	sensor_info->AESensorGainDelayFrame = imgsensor_info.ae_sensor_gain_delay_frame;	
	sensor_info->AEISPGainDelayFrame = imgsensor_info.ae_ispGain_delay_frame;	
	sensor_info->IHDR_Support = imgsensor_info.ihdr_support;
	sensor_info->IHDR_LE_FirstLine = imgsensor_info.ihdr_le_firstline;
	sensor_info->SensorModeNum = imgsensor_info.sensor_mode_num;
	
	sensor_info->SensorMIPILaneNumber = imgsensor_info.mipi_lane_num; 
	sensor_info->SensorClockFreq = imgsensor_info.mclk;
	sensor_info->SensorClockDividCount = 3; 
	sensor_info->SensorClockRisingCount = 0;
	sensor_info->SensorClockFallingCount = 2; 
	sensor_info->SensorPixelClockCount = 3; 
	sensor_info->SensorDataLatchCount = 2; 
	
	sensor_info->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
	sensor_info->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
	sensor_info->SensorWidthSampling = 0;  
	sensor_info->SensorHightSampling = 0;	
	sensor_info->SensorPacketECCOrder = 1;

	switch (scenario_id) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			sensor_info->SensorGrabStartX = imgsensor_info.pre.startx; 
			sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;		
			
			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.pre.mipi_data_lp2hs_settle_dc;
			
			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			sensor_info->SensorGrabStartX = imgsensor_info.cap.startx; 
			sensor_info->SensorGrabStartY = imgsensor_info.cap.starty;
				  
			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.cap.mipi_data_lp2hs_settle_dc; 

			break;	 
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			
			sensor_info->SensorGrabStartX = imgsensor_info.normal_video.startx; 
			sensor_info->SensorGrabStartY = imgsensor_info.normal_video.starty;
	   
			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.normal_video.mipi_data_lp2hs_settle_dc; 

			break;	  
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:			
			sensor_info->SensorGrabStartX = imgsensor_info.hs_video.startx; 
			sensor_info->SensorGrabStartY = imgsensor_info.hs_video.starty;
				  
			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.hs_video.mipi_data_lp2hs_settle_dc; 

			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			sensor_info->SensorGrabStartX = imgsensor_info.slim_video.startx; 
			sensor_info->SensorGrabStartY = imgsensor_info.slim_video.starty;
				  
			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.slim_video.mipi_data_lp2hs_settle_dc; 

			break;
		default:			
			sensor_info->SensorGrabStartX = imgsensor_info.pre.startx; 
			sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;		
			
			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.pre.mipi_data_lp2hs_settle_dc;
			break;
	}
	
	return ERROR_NONE;
}	


static kal_uint32 control(MSDK_SCENARIO_ID_ENUM scenario_id, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("scenario_id = %d\n", scenario_id);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.current_scenario_id = scenario_id;
	spin_unlock(&imgsensor_drv_lock);
	switch (scenario_id) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			preview(image_window, sensor_config_data);
			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			capture(image_window, sensor_config_data);
			break;	
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			normal_video(image_window, sensor_config_data);
			break;	  
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			hs_video(image_window, sensor_config_data);
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			slim_video(image_window, sensor_config_data);
			break;	  
		default:
			LOG_INF("Error ScenarioId setting");
			preview(image_window, sensor_config_data);
			return ERROR_INVALID_SCENARIO_ID;
	}
	return ERROR_NONE;
}	



static kal_uint32 set_video_mode(UINT16 framerate)
	{
		LOG_INF("framerate = %d\n ", framerate);
		
		if (framerate == 0)
			
			return ERROR_NONE;
		spin_lock(&imgsensor_drv_lock);
		if ((framerate == 300) && (imgsensor.autoflicker_en == KAL_TRUE))
			imgsensor.current_fps = 296;
		else if ((framerate == 150) && (imgsensor.autoflicker_en == KAL_TRUE))
			imgsensor.current_fps = 146;
		else
			imgsensor.current_fps = framerate;
		spin_unlock(&imgsensor_drv_lock);
		set_max_framerate(imgsensor.current_fps,1);
	
		return ERROR_NONE;
	}


static kal_uint32 set_auto_flicker_mode(kal_bool enable, UINT16 framerate)
{
	LOG_INF("enable = %d, framerate = %d \n", enable, framerate);
	spin_lock(&imgsensor_drv_lock);
	if (enable) 
		imgsensor.autoflicker_en = KAL_TRUE;
	else 
		imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}


static kal_uint32 set_max_framerate_by_scenario(MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 framerate) 
{
	kal_int16 dummyLine;
	kal_uint32 lineLength,frame_length;
  
	LOG_INF("scenario_id = %d, framerate = %d\n", scenario_id, framerate);

	switch (scenario_id) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			frame_length = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.pre.framelength) ? (frame_length - imgsensor_info.pre.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			 LOG_INF("[ylf_preview]imgsensor.frame_length = %d, imgsensor.min_frame_length = %d\n", imgsensor.frame_length, imgsensor.min_frame_length);
			
			break;			
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			if(framerate == 0)
				return ERROR_NONE;
			frame_length = imgsensor_info.normal_video.pclk / framerate * 10 / imgsensor_info.normal_video.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.normal_video.framelength) ? (frame_length - imgsensor_info.normal_video.framelength) : 0;			
			imgsensor.frame_length = imgsensor_info.normal_video.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			LOG_INF("[ylf_video]imgsensor.frame_length = %d, imgsensor.min_frame_length = %d\n", imgsensor.frame_length, imgsensor.min_frame_length);
			
			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			if(framerate==300)
			{
				frame_length = imgsensor_info.cap.pclk / framerate * 10 / imgsensor_info.cap.linelength;
				spin_lock(&imgsensor_drv_lock);
				imgsensor.dummy_line = (frame_length > imgsensor_info.cap.framelength) ? (frame_length - imgsensor_info.cap.framelength) : 0;
				imgsensor.frame_length = imgsensor_info.cap.framelength + imgsensor.dummy_line;
				imgsensor.min_frame_length = imgsensor.frame_length;
				spin_unlock(&imgsensor_drv_lock);
			}
			else
			{
				frame_length = imgsensor_info.cap1.pclk / framerate * 10 / imgsensor_info.cap1.linelength;
				spin_lock(&imgsensor_drv_lock);
				imgsensor.dummy_line = (frame_length > imgsensor_info.cap1.framelength) ? (frame_length - imgsensor_info.cap1.framelength) : 0;
				imgsensor.frame_length = imgsensor_info.cap1.framelength + imgsensor.dummy_line;
				imgsensor.min_frame_length = imgsensor.frame_length;
				spin_unlock(&imgsensor_drv_lock);
			}
			LOG_INF("[ylf_CAPTURE]framerate = %d, imgsensor.dummy_line = %d,imgsensor.min_frame_length = %d\n", framerate,imgsensor.dummy_line, imgsensor.min_frame_length);
			
			break;	
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			frame_length = imgsensor_info.hs_video.pclk / framerate * 10 / imgsensor_info.hs_video.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.hs_video.framelength) ? (frame_length - imgsensor_info.hs_video.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.hs_video.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			LOG_INF("[ylf_SPEED_VIDEO]imgsensor.frame_length = %d, imgsensor.min_frame_length = %d\n", imgsensor.frame_length, imgsensor.min_frame_length);
			
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			frame_length = imgsensor_info.slim_video.pclk / framerate * 10 / imgsensor_info.slim_video.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.slim_video.framelength) ? (frame_length - imgsensor_info.slim_video.framelength): 0;	
			imgsensor.frame_length = imgsensor_info.slim_video.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			LOG_INF("[ylf_slim_VIDEO]imgsensor.frame_length = %d, imgsensor.min_frame_length = %d\n", imgsensor.frame_length, imgsensor.min_frame_length);
			
		default:  
			frame_length = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = frame_length - imgsensor_info.pre.framelength;
			if (imgsensor.dummy_line < 0)
				imgsensor.dummy_line = 0;
			imgsensor.frame_length += imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			
			LOG_INF("error scenario_id = %d, we use preview scenario \n", scenario_id);
			break;
	}	
	return ERROR_NONE;
}


static kal_uint32 get_default_framerate_by_scenario(MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 *framerate) 
{
	LOG_INF("scenario_id = %d\n", scenario_id);

	switch (scenario_id) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			*framerate = imgsensor_info.pre.max_framerate;
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			*framerate = imgsensor_info.normal_video.max_framerate;
			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*framerate = imgsensor_info.cap.max_framerate;
			break;		
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			*framerate = imgsensor_info.hs_video.max_framerate;
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO: 
			*framerate = imgsensor_info.slim_video.max_framerate;
			break;
		default:
			break;
	}

	return ERROR_NONE;
}

static kal_uint32 set_test_pattern_mode(kal_bool enable)
{
	LOG_INF("enable: %d\n", enable);

	if(enable) 	 
		write_cmos_sensor(0x0601, 0x02);
	else		  
		write_cmos_sensor(0x0601, 0x00);  

	spin_lock(&imgsensor_drv_lock);
	imgsensor.test_pattern = enable;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}

static kal_uint32 feature_control(MSDK_SENSOR_FEATURE_ENUM feature_id,
							 UINT8 *feature_para,UINT32 *feature_para_len)
{
	UINT16 *feature_return_para_16=(UINT16 *) feature_para;
	UINT16 *feature_data_16=(UINT16 *) feature_para;
	UINT32 *feature_return_para_32=(UINT32 *) feature_para;
	UINT32 *feature_data_32=(UINT32 *) feature_para;

    unsigned long long *feature_data=(unsigned long long *) feature_para;
    unsigned long long *feature_return_para=(unsigned long long *) feature_para;
	
	SENSOR_WINSIZE_INFO_STRUCT *wininfo;	
	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data=(MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;
 
	
	switch (feature_id) {
		case SENSOR_FEATURE_GET_PERIOD:
			*feature_return_para_16++ = imgsensor.line_length;
			*feature_return_para_16 = imgsensor.frame_length;
			*feature_para_len=4;
			break;
		case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:	 
			*feature_return_para_32 = imgsensor.pclk;
			*feature_para_len=4;
			break;		   
		case SENSOR_FEATURE_SET_ESHUTTER:
			set_shutter(*feature_data_16);
			break;
		case SENSOR_FEATURE_SET_NIGHTMODE:
			night_mode((BOOL) *feature_data_16);
			break;
		case SENSOR_FEATURE_SET_GAIN:		
			set_gain((UINT16) *feature_data_16);
			break;
		case SENSOR_FEATURE_SET_FLASHLIGHT:
			break;
		case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
			break;
		case SENSOR_FEATURE_SET_REGISTER:
			write_cmos_sensor(sensor_reg_data->RegAddr, sensor_reg_data->RegData);
			break;
		case SENSOR_FEATURE_GET_REGISTER:
			sensor_reg_data->RegData = read_cmos_sensor(sensor_reg_data->RegAddr);
			break;
		case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
			
			
			*feature_return_para_32=LENS_DRIVER_ID_DO_NOT_CARE;
			*feature_para_len=4;
			break;
		case SENSOR_FEATURE_SET_VIDEO_MODE:
			set_video_mode(*feature_data_16);
			break; 
		case SENSOR_FEATURE_CHECK_SENSOR_ID:
            get_imgsensor_id(feature_return_para_32); 
			break; 
		case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
			set_auto_flicker_mode((BOOL)*feature_data_16,*(feature_data_16+1));
			break;
		case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
			set_max_framerate_by_scenario((MSDK_SCENARIO_ID_ENUM)*feature_data_32, *(feature_data+1));
			break;
		case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
			get_default_framerate_by_scenario((MSDK_SCENARIO_ID_ENUM)*feature_data_32, (MUINT32 *)(*(feature_data+1)));
			break;
		case SENSOR_FEATURE_SET_TEST_PATTERN:
			set_test_pattern_mode((BOOL)*feature_data_16);
			break;
		case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE: 
			*feature_return_para_32 = imgsensor_info.checksum_value;
			*feature_para_len=4;							 
			break;				
		case SENSOR_FEATURE_SET_FRAMERATE:
			LOG_INF("current fps :%d\n", *feature_data_16);
			spin_lock(&imgsensor_drv_lock);
			imgsensor.current_fps = *feature_data_16;
			spin_unlock(&imgsensor_drv_lock);		
			break;
	
	
	
		case SENSOR_FEATURE_GET_CROP_INFO:
			LOG_INF("SENSOR_FEATURE_GET_CROP_INFO scenarioId:%d\n", *feature_data_32);
			wininfo = (SENSOR_WINSIZE_INFO_STRUCT *)(*(feature_data+1));
		
			switch (*feature_data_32) {
				case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
					memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[1],sizeof(SENSOR_WINSIZE_INFO_STRUCT));
					break;	  
				case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
					memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[2],sizeof(SENSOR_WINSIZE_INFO_STRUCT));
					break;
				case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
					memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[3],sizeof(SENSOR_WINSIZE_INFO_STRUCT));
					break;
				case MSDK_SCENARIO_ID_SLIM_VIDEO:
					memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[4],sizeof(SENSOR_WINSIZE_INFO_STRUCT));
					break;
				case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
				default:
					memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[0],sizeof(SENSOR_WINSIZE_INFO_STRUCT));
					break;
			}
		case SENSOR_FEATURE_SET_IHDR_SHUTTER_GAIN:
			LOG_INF("SENSOR_SET_SENSOR_IHDR LE=%d, SE=%d, Gain=%d\n",*feature_data_32,*(feature_data_32+1),*(feature_data_32+2)); 
			ihdr_write_shutter_gain(*feature_data_32,*(feature_data_32+1),*(feature_data_32+2));	
			break;
		default:
			break;
	}
  
	return ERROR_NONE;
}	

static SENSOR_FUNCTION_STRUCT sensor_func = {
	open,
	get_info,
	get_resolution,
	feature_control,
	control,
	close
};

UINT32 S5K4H5YC_MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	
	if (pfFunc!=NULL)
		*pfFunc=&sensor_func;
	return ERROR_NONE;
}	
