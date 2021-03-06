#include <mach/charging.h>
#include "fan5405.h"
#include <mach/upmu_common.h>
#include <mach/mt_gpio.h>
#include <cust_gpio_usage.h>
#include <mach/upmu_hw.h>
#include <linux/xlog.h>
#include <linux/delay.h>
#include <mach/mt_sleep.h>
#include <mach/mt_boot.h>
#include <mach/system.h>
#include <mach/upmu_sw.h>
#include <cust_charging.h>
#include <mach/mt6311.h>

 
 
 
#define STATUS_OK	0
#define STATUS_UNSUPPORTED	-1
#define GETARRAYNUM(array) (sizeof(array)/sizeof(array[0]))

#define upmu_set_rg_bc11_vsrc_en mt6325_upmu_set_rg_bc11_vsrc_en
#define upmu_set_rg_bc11_vref_vth mt6325_upmu_set_rg_bc11_vref_vth 
#define upmu_set_rg_bc11_cmp_en  mt6325_upmu_set_rg_bc11_cmp_en
#define upmu_set_rg_bc11_ipu_en  mt6325_upmu_set_rg_bc11_ipu_en 
#define upmu_set_rg_bc11_ipd_en  mt6325_upmu_set_rg_bc11_ipd_en
#define upmu_set_rg_bc11_rst     mt6325_upmu_set_rg_bc11_rst
#define upmu_set_rg_bc11_bb_ctrl   mt6325_upmu_set_rg_bc11_bb_ctrl
#define upmu_get_rgs_bc11_cmp_out  mt6325_upmu_get_rgs_bc11_cmp_out
#define upmu_set_rg_usbdl_set      mt6325_upmu_set_rg_usbdl_set
#define upmu_set_rg_usbdl_rst      mt6325_upmu_set_rg_usbdl_rst
#define upmu_set_rg_vcdt_hv_vth    mt6325_upmu_set_rg_vcdt_hv_vth
#define upmu_get_rgs_vcdt_hv_det   mt6325_upmu_get_rgs_vcdt_hv_det 
#define upmu_set_baton_tdet_en     mt6325_upmu_set_baton_tdet_en
#define upmu_set_rg_baton_en       mt6325_upmu_set_rg_baton_en
#define upmu_get_rgs_baton_undet   mt6325_upmu_get_rgs_baton_undet
#define upmu_get_rgs_chrdet        mt6325_upmu_get_rgs_chrdet
#define upmu_set_rg_bc11_bias_en   mt6325_upmu_set_rg_bc11_bias_en 


 
 
 
#if 1
#include <cust_gpio_usage.h>
int gpio_number   = GPIO_SWCHARGER_EN_PIN; 
int gpio_off_mode = GPIO_SWCHARGER_EN_PIN_M_GPIO;
int gpio_on_mode  = GPIO_SWCHARGER_EN_PIN_M_GPIO;
#else
int gpio_number   = (19 | 0x80000000);
int gpio_off_mode = 0;
int gpio_on_mode  = 0;
#endif
int gpio_off_dir  = GPIO_DIR_OUT;
int gpio_off_out  = GPIO_OUT_ONE;
int gpio_on_dir   = GPIO_DIR_OUT;
int gpio_on_out   = GPIO_OUT_ZERO;

kal_bool charging_type_det_done = KAL_TRUE;
static CHARGER_TYPE g_charger_type = CHARGER_UNKNOWN;

const kal_uint32 VBAT_CV_VTH[]=
{
	BATTERY_VOLT_03_500000_V,   BATTERY_VOLT_03_520000_V,	BATTERY_VOLT_03_540000_V,   BATTERY_VOLT_03_560000_V,
	BATTERY_VOLT_03_580000_V,   BATTERY_VOLT_03_600000_V,	BATTERY_VOLT_03_620000_V,   BATTERY_VOLT_03_640000_V,
	BATTERY_VOLT_03_660000_V,	BATTERY_VOLT_03_680000_V,	BATTERY_VOLT_03_700000_V,	BATTERY_VOLT_03_720000_V,
	BATTERY_VOLT_03_740000_V,	BATTERY_VOLT_03_760000_V,	BATTERY_VOLT_03_780000_V,	BATTERY_VOLT_03_800000_V,
	BATTERY_VOLT_03_820000_V,	BATTERY_VOLT_03_840000_V,	BATTERY_VOLT_03_860000_V,	BATTERY_VOLT_03_880000_V,
	BATTERY_VOLT_03_900000_V,	BATTERY_VOLT_03_920000_V,	BATTERY_VOLT_03_940000_V,	BATTERY_VOLT_03_960000_V,
	BATTERY_VOLT_03_980000_V,	BATTERY_VOLT_04_000000_V,	BATTERY_VOLT_04_020000_V,	BATTERY_VOLT_04_040000_V,
	BATTERY_VOLT_04_060000_V,	BATTERY_VOLT_04_080000_V,	BATTERY_VOLT_04_100000_V,	BATTERY_VOLT_04_120000_V,
	BATTERY_VOLT_04_140000_V,   BATTERY_VOLT_04_160000_V,	BATTERY_VOLT_04_180000_V,   BATTERY_VOLT_04_200000_V,
	BATTERY_VOLT_04_220000_V,   BATTERY_VOLT_04_240000_V,	BATTERY_VOLT_04_260000_V,   BATTERY_VOLT_04_280000_V,
	BATTERY_VOLT_04_300000_V,   BATTERY_VOLT_04_320000_V,	BATTERY_VOLT_04_340000_V,   BATTERY_VOLT_04_360000_V,	
	BATTERY_VOLT_04_380000_V,   BATTERY_VOLT_04_400000_V,	BATTERY_VOLT_04_420000_V,   BATTERY_VOLT_04_440000_V	
	
};

const kal_uint32 CS_VTH[]=
{
	CHARGE_CURRENT_550_00_MA,   CHARGE_CURRENT_650_00_MA,	CHARGE_CURRENT_750_00_MA, CHARGE_CURRENT_850_00_MA,
	CHARGE_CURRENT_950_00_MA,   CHARGE_CURRENT_1050_00_MA,	CHARGE_CURRENT_1150_00_MA, CHARGE_CURRENT_1250_00_MA
}; 

 const kal_uint32 INPUT_CS_VTH[]=
 {
	 CHARGE_CURRENT_100_00_MA,	 CHARGE_CURRENT_500_00_MA,	 CHARGE_CURRENT_800_00_MA, CHARGE_CURRENT_MAX
 }; 

 const kal_uint32 VCDT_HV_VTH[]=
 {
	  BATTERY_VOLT_04_200000_V, BATTERY_VOLT_04_250000_V,	  BATTERY_VOLT_04_300000_V,   BATTERY_VOLT_04_350000_V,
	  BATTERY_VOLT_04_400000_V, BATTERY_VOLT_04_450000_V,	  BATTERY_VOLT_04_500000_V,   BATTERY_VOLT_04_550000_V,
	  BATTERY_VOLT_04_600000_V, BATTERY_VOLT_06_000000_V,	  BATTERY_VOLT_06_500000_V,   BATTERY_VOLT_07_000000_V,
	  BATTERY_VOLT_07_500000_V, BATTERY_VOLT_08_500000_V,	  BATTERY_VOLT_09_500000_V,   BATTERY_VOLT_10_500000_V		  
 };

 
 
 


 
 
 

 
 
 
 extern kal_uint32 upmu_get_reg_value(kal_uint32 reg);
 extern bool mt_usb_is_device(void);
 extern void Charger_Detect_Init(void);
 extern void Charger_Detect_Release(void);
 extern int hw_charging_get_charger_type(void);
 extern void mt_power_off(void);

 
 kal_uint32 charging_value_to_parameter(const kal_uint32 *parameter, const kal_uint32 array_size, const kal_uint32 val)
{
	if (val < array_size)
	{
		return parameter[val];
	}
	else
	{
		battery_xlog_printk(BAT_LOG_CRTI, "Can't find the parameter \r\n");
		return parameter[0];
	}
}

 
 kal_uint32 charging_parameter_to_value(const kal_uint32 *parameter, const kal_uint32 array_size, const kal_uint32 val)
{
	kal_uint32 i;

	for(i=0;i<array_size;i++)
	{
		if (val >= *(parameter + i)&&val < *(parameter+i+1))
		{
				return i;
		}
	}

    battery_xlog_printk(BAT_LOG_CRTI, "NO register value match \r\n");
	
	return 0;
}


 static kal_uint32 bmt_find_closest_level(const kal_uint32 *pList,kal_uint32 number,kal_uint32 level)
 {
	 kal_uint32 i;
	 kal_uint32 max_value_in_last_element;
 
	 if(pList[0] < pList[1])
		 max_value_in_last_element = KAL_TRUE;
	 else
		 max_value_in_last_element = KAL_FALSE;
 
	 if(max_value_in_last_element == KAL_TRUE)
	 {
		 for(i = (number-1); i != 0; i--)	 
		 {
			 if(pList[i] <= level)
			 {
				 return pList[i];
			 }	  
		 }

 		 battery_xlog_printk(BAT_LOG_CRTI, "Can't find closest level, small value first \r\n");
		 return pList[0];
		 
	 }
	 else
	 {
		 for(i = 0; i< number; i++)  
		 {
			 if(pList[i] <= level)
			 {
				 return pList[i];
			 }
		 }

		 battery_xlog_printk(BAT_LOG_CRTI, "Can't find closest level, large value first \r\n"); 	 
		 return pList[number -1];
  		 
	 }
 }

static kal_uint32 is_chr_det(void)
{
    kal_uint32 val=0;

    val = mt6325_upmu_get_rgs_chrdet();

    battery_xlog_printk(BAT_LOG_CRTI,"[is_chr_det] %d\n", val);

    return val;
}

 static kal_uint32 charging_hw_init(void *data)
 {
 	kal_uint32 status = STATUS_OK;
	static bool charging_init_flag = KAL_FALSE;

	mt_set_gpio_mode(gpio_number,gpio_on_mode);
	mt_set_gpio_dir(gpio_number,gpio_on_dir);
	mt_set_gpio_out(gpio_number,gpio_on_out);

	battery_xlog_printk(BAT_LOG_FULL, "gpio_number=0x%x,gpio_on_mode=%d,gpio_off_mode=%d\n", gpio_number, gpio_on_mode, gpio_off_mode);

	upmu_set_rg_usbdl_set(0);       
	upmu_set_rg_usbdl_rst(1);		

	#if defined(HIGH_BATTERY_VOLTAGE_SUPPORT)
	fan5405_reg_config_interface(0x06,0x77); 
	#else
	fan5405_reg_config_interface(0x06,0x70);
	#endif
	    
	fan5405_reg_config_interface(0x00,0xC0);	
	fan5405_reg_config_interface(0x01,0xb8);	
	fan5405_reg_config_interface(0x05,0x03);
	if ( !charging_init_flag ) {   
		fan5405_reg_config_interface(0x04,0x1A); 
		charging_init_flag = KAL_TRUE;
	}        
 	return status;
 }


 static kal_uint32 charging_dump_register(void *data)
 {
 	kal_uint32 status = STATUS_OK;

	fan5405_dump_register();
   	
	return status;
 }	

 static kal_uint32 charging_dump_register_htc(void *data)
 {
	int len = 0;
	pCHR_REG_DUMP pReg_dump = (pCHR_REG_DUMP)data;
	char *buf = pReg_dump->buf;
	int size = pReg_dump->size;
	len = fan5405_dump_register_htc(buf, size);
	pReg_dump->size = len;
	return STATUS_OK;
 }

 static kal_uint32 charging_enable(void *data)
 {
 	kal_uint32 status = STATUS_OK;
	kal_uint32 enable = *(kal_uint32*)(data);

	if(KAL_TRUE == enable)
	{
		fan5405_set_ce(0);
		fan5405_set_hz_mode(0);
		fan5405_set_opa_mode(0);
	}
	else
	{

#if defined(CONFIG_USB_MTK_HDRC_HCD)
   		if(mt_usb_is_device())
#endif 			
    	{
	        mt_set_gpio_mode(gpio_number,gpio_off_mode);  
	        mt_set_gpio_dir(gpio_number,gpio_off_dir);
	        mt_set_gpio_out(gpio_number,gpio_off_out);

	        fan5405_set_ce(1);
    	}
	}
		
	return status;
 }
 
  extern int is_mt6311_exist(void);
  extern kal_uint32 mt6311_get_chip_id(void);
  


 static kal_uint32 charging_set_cv_voltage(void *data)
 {
 	kal_uint32 status = STATUS_OK;
	kal_uint16 register_value;
	
	register_value = charging_parameter_to_value(VBAT_CV_VTH, GETARRAYNUM(VBAT_CV_VTH) ,*(kal_uint32 *)(data));
    #if 0 
	fan5405_set_oreg(register_value); 
    #else
    
    if(mt6325_upmu_get_swcid() == PMIC6325_E1_CID_CODE)
    {
        #if defined(CV_E1_INTERNAL)
		fan5405_set_oreg(0x19); 
        #else
		fan5405_set_oreg(0xf); 
        #endif
        battery_xlog_printk(BAT_LOG_CRTI,"[charging_set_cv_voltage] set low CV by 6325 E1\n");
    }
    else
    {
        if(is_mt6311_exist())
        {
            if(mt6311_get_chip_id()==PMIC6311_E1_CID_CODE)
            {
                #if defined(CV_E1_INTERNAL)
				fan5405_set_oreg(0x19); 
                #else
				fan5405_set_oreg(0xf); 
                #endif
                battery_xlog_printk(BAT_LOG_CRTI,"[charging_set_cv_voltage] set low CV by 6311 E1\n");
            }
            else
            {
				fan5405_set_oreg(register_value); 
            }
        }
        else
        {
				fan5405_set_oreg(register_value); 
        } 
    }  
    #endif

	return status;
 } 	


 static kal_uint32 charging_get_current(void *data)
 {
    kal_uint32 status = STATUS_OK;
    kal_uint32 array_size;
    kal_uint8 reg_value;
	
    
    array_size = GETARRAYNUM(CS_VTH);
    fan5405_read_interface(0x1, &reg_value, 0x3, 0x6);	
    *(kal_uint32 *)data = charging_value_to_parameter(CS_VTH,array_size,reg_value);
	
    return status;
 }  
  


 static kal_uint32 charging_set_current(void *data)
 {
 	kal_uint32 status = STATUS_OK;
	kal_uint32 set_chr_current;
	kal_uint32 array_size;
	kal_uint32 register_value;
	kal_uint32 current_value = *(kal_uint32 *)data;

	if(current_value <= CHARGE_CURRENT_350_00_MA)
	{
		fan5405_set_io_level(1);
	}
	else
	{
		fan5405_set_io_level(0);
		array_size = GETARRAYNUM(CS_VTH);
		set_chr_current = bmt_find_closest_level(CS_VTH, array_size, current_value);
		register_value = charging_parameter_to_value(CS_VTH, array_size ,set_chr_current);
		fan5405_set_iocharge(register_value);
	}
	return status;
 } 	
 

 static kal_uint32 charging_set_input_current(void *data)
 {
 	kal_uint32 status = STATUS_OK;
	kal_uint32 set_chr_current;
	kal_uint32 array_size;
	kal_uint32 register_value;

    if(*(kal_uint32 *)data > CHARGE_CURRENT_500_00_MA)
    {
        register_value = 0x3;
    }
    else
    {
    	array_size = GETARRAYNUM(INPUT_CS_VTH);
    	set_chr_current = bmt_find_closest_level(INPUT_CS_VTH, array_size, *(kal_uint32 *)data);
    	register_value = charging_parameter_to_value(INPUT_CS_VTH, array_size ,set_chr_current);	
    }
    
    fan5405_set_input_charging_current(register_value);

	return status;
 } 	


 static kal_uint32 charging_get_charging_status(void *data)
 {
 	kal_uint32 status = STATUS_OK;
	kal_uint32 ret_val;

	ret_val = fan5405_get_chip_status();
	
	if(ret_val == 0x2)
		*(kal_uint32 *)data = KAL_TRUE;
	else
		*(kal_uint32 *)data = KAL_FALSE;
	
	return status;
 } 	


 static kal_uint32 charging_reset_watch_dog_timer(void *data)
 {
	 kal_uint32 status = STATUS_OK;
 
	 fan5405_set_tmr_rst(1);
	 
	 return status;
 }
 
 
  static kal_uint32 charging_set_hv_threshold(void *data)
  {
	 kal_uint32 status = STATUS_OK;
 
	 kal_uint32 set_hv_voltage;
	 kal_uint32 array_size;
	 kal_uint16 register_value;
	 kal_uint32 voltage = *(kal_uint32*)(data);
	 
	 array_size = GETARRAYNUM(VCDT_HV_VTH);
	 set_hv_voltage = bmt_find_closest_level(VCDT_HV_VTH, array_size, voltage);
	 register_value = charging_parameter_to_value(VCDT_HV_VTH, array_size ,set_hv_voltage);
	 upmu_set_rg_vcdt_hv_vth(register_value);
 
	 return status;
  }
 
 
  static kal_uint32 charging_get_hv_status(void *data)
  {
	   kal_uint32 status = STATUS_OK;
 
	   *(kal_bool*)(data) = upmu_get_rgs_vcdt_hv_det();
	   
	   return status;
  }


 static kal_uint32 charging_get_battery_status(void *data)
 {
	   kal_uint32 status = STATUS_OK;
 
 	   upmu_set_baton_tdet_en(1);	
	   upmu_set_rg_baton_en(1);
	   *(kal_bool*)(data) = upmu_get_rgs_baton_undet();
	   
	   return status;
 }


 static kal_uint32 charging_get_charger_det_status(void *data)
 {
	   kal_uint32 status = STATUS_OK;
 
	   *(kal_bool*)(data) = upmu_get_rgs_chrdet();
	   
	   return status;
 }


kal_bool charging_type_detection_done(void)
{
	 return charging_type_det_done;
}


 static kal_uint32 charging_get_charger_type(void *data)
 {
	 kal_uint32 status = STATUS_OK;

#if defined(CONFIG_POWER_EXT) || defined(CONFIG_MTK_FPGA)
    *(CHARGER_TYPE*)(data) = STANDARD_HOST;
#else

    if(is_chr_det()==0)
    {
        g_charger_type = CHARGER_UNKNOWN;
        *(CHARGER_TYPE*)(data) = CHARGER_UNKNOWN;
        battery_xlog_printk(BAT_LOG_CRTI, "[charging_get_charger_type] return CHARGER_UNKNOWN\n");
        return status;
    }

    charging_type_det_done = KAL_FALSE;

    *(CHARGER_TYPE*)(data) = hw_charging_get_charger_type();

    charging_type_det_done = KAL_TRUE;

    g_charger_type = *(CHARGER_TYPE*)(data);

#endif

	return status;
}

static kal_uint32 charging_get_is_pcm_timer_trigger(void *data)
{
    kal_uint32 status = STATUS_OK;

    if(slp_get_wake_reason() == WR_PCM_TIMER)
        *(kal_bool*)(data) = KAL_TRUE;
    else
        *(kal_bool*)(data) = KAL_FALSE;

    battery_xlog_printk(BAT_LOG_CRTI, "slp_get_wake_reason=%d\n", slp_get_wake_reason());
       
    return status;
}

static kal_uint32 charging_set_platform_reset(void *data)
{
    kal_uint32 status = STATUS_OK;

    battery_xlog_printk(BAT_LOG_CRTI, "charging_set_platform_reset\n");
 
    arch_reset(0,NULL);
        
    return status;
}

static kal_uint32 charging_get_platfrom_boot_mode(void *data)
{
    kal_uint32 status = STATUS_OK;
  
    *(kal_uint32*)(data) = get_boot_mode();

    battery_xlog_printk(BAT_LOG_CRTI, "get_boot_mode=%d\n", get_boot_mode());
         
    return status;
}

static kal_uint32 charging_set_power_off(void *data)
{
    kal_uint32 status = STATUS_OK;
  
    battery_xlog_printk(BAT_LOG_CRTI, "charging_set_power_off\n");
    mt_power_off();
         
    return status;
}

 static kal_uint32 (* const charging_func[CHARGING_CMD_NUMBER])(void *data)=
 {
 	  charging_hw_init
	,charging_dump_register  	
	,charging_enable
	,charging_set_cv_voltage
	,charging_get_current
	,charging_set_current
	,charging_set_input_current
	,charging_get_charging_status
	,charging_reset_watch_dog_timer
	,charging_set_hv_threshold
	,charging_get_hv_status
	,charging_get_battery_status
	,charging_get_charger_det_status
	,charging_get_charger_type
	,charging_get_is_pcm_timer_trigger
	,charging_set_platform_reset
	,charging_get_platfrom_boot_mode
	,charging_set_power_off
	,charging_dump_register_htc
 };

 
 kal_int32 chr_control_interface(CHARGING_CTRL_CMD cmd, void *data)
 {
	 kal_int32 status;
	 if(cmd < CHARGING_CMD_NUMBER)
		 status = charging_func[cmd](data);
	 else
		 return STATUS_UNSUPPORTED;
 
	 return status;
 }
