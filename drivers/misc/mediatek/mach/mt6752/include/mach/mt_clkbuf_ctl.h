#ifndef __MT_CLK_BUF_CTL_H__
#define __MT_CLK_BUF_CTL_H__

#include <linux/kernel.h>
#include <linux/mutex.h>
#include <cust_clk_buf.h>
#if 0
#define GPIO53 53
#define GPIO54 54
#define GPIO55 55
#define GPIO56 56
#define GPIO57 57
#endif

#if 1
#ifndef GPIO_RFIC0_BSI_CS
#define GPIO_RFIC0_BSI_CS         (GPIO53|0x80000000)    
#endif
#ifndef GPIO_RFIC0_BSI_CK
#define GPIO_RFIC0_BSI_CK         (GPIO54|0x80000000)    
#endif
#ifndef GPIO_RFIC0_BSI_D0
#define GPIO_RFIC0_BSI_D0         (GPIO55|0x80000000)    
#endif
#ifndef GPIO_RFIC0_BSI_D1
#define GPIO_RFIC0_BSI_D1         (GPIO56|0x80000000)    
#endif
#ifndef GPIO_RFIC0_BSI_D2
#define GPIO_RFIC0_BSI_D2         (GPIO57|0x80000000)    
#endif
#endif


enum clk_buf_id{
    CLK_BUF_BB			= 0,
    CLK_BUF_6605		= 1,
    CLK_BUF_5193		= 2,
    CLK_BUF_AUDIO		= 3,
    CLK_BUF_INVALID		= 4,
};
typedef enum
{
   CLK_BUF_SW_DISABLE = 0,
   CLK_BUF_SW_ENABLE  = 1,
}CLK_BUF_SWCTRL_STATUS_T;
#define CLKBUF_NUM         4 

#define STA_CLK_ON      1
#define STA_CLK_OFF     0

bool clk_buf_ctrl(enum clk_buf_id id,bool onoff);
void clk_buf_get_swctrl_status(CLK_BUF_SWCTRL_STATUS_T *status);
bool clk_buf_init(void);

extern struct mutex clk_buf_ctrl_lock;
#endif

