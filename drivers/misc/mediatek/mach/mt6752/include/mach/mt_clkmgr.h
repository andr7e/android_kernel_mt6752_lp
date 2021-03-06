#ifndef _MT_CLKMGR_H
#define _MT_CLKMGR_H

#include <linux/list.h>
#include "mach/mt_typedefs.h"

#define CONFIG_CLKMGR_STAT

#ifdef CONFIG_OF
extern void __iomem  *clk_apmixed_base;
extern void __iomem  *clk_cksys_base;
extern void __iomem  *clk_infracfg_ao_base;
extern void __iomem  *clk_audio_base;
extern void __iomem  *clk_mfgcfg_base;
extern void __iomem  *clk_mmsys_config_base;
extern void __iomem  *clk_imgsys_base;
extern void __iomem  *clk_vdec_gcon_base;
extern void __iomem  *clk_mjc_config_base;
extern void __iomem  *clk_venc_gcon_base;
#endif


#define AP_PLL_CON0             (clk_apmixed_base + 0x00)
#define AP_PLL_CON1             (clk_apmixed_base + 0x04)
#define AP_PLL_CON2             (clk_apmixed_base + 0x08)
#define AP_PLL_CON7             (clk_apmixed_base + 0x1C)

#define ARMCA7PLL_CON0          (clk_apmixed_base + 0x210)
#define ARMCA7PLL_CON1          (clk_apmixed_base + 0x214)
#define ARMCA7PLL_CON2          (clk_apmixed_base + 0x218)
#define ARMCA7PLL_PWR_CON0      (clk_apmixed_base + 0x21C)

#define MAINPLL_CON0            (clk_apmixed_base + 0x220)
#define MAINPLL_CON1            (clk_apmixed_base + 0x224)
#define MAINPLL_PWR_CON0        (clk_apmixed_base + 0x22C)

#define UNIVPLL_CON0            (clk_apmixed_base + 0x230)
#define UNIVPLL_CON1            (clk_apmixed_base + 0x234)
#define UNIVPLL_PWR_CON0        (clk_apmixed_base + 0x23C)

#define MMPLL_CON0              (clk_apmixed_base + 0x240)
#define MMPLL_CON1              (clk_apmixed_base + 0x244)
#define MMPLL_CON2              (clk_apmixed_base + 0x248)
#define MMPLL_PWR_CON0          (clk_apmixed_base + 0x24C)

#define MSDCPLL_CON0            (clk_apmixed_base + 0x250)
#define MSDCPLL_CON1            (clk_apmixed_base + 0x254)
#define MSDCPLL_PWR_CON0        (clk_apmixed_base + 0x25C)

#define VENCPLL_CON0            (clk_apmixed_base + 0x260)
#define VENCPLL_CON1            (clk_apmixed_base + 0x264)
#define VENCPLL_PWR_CON0        (clk_apmixed_base + 0x26C)

#define TVDPLL_CON0             (clk_apmixed_base + 0x270)
#define TVDPLL_CON1             (clk_apmixed_base + 0x274)
#define TVDPLL_PWR_CON0         (clk_apmixed_base + 0x27C)

#define MPLL_CON0               (clk_apmixed_base + 0x280)
#define MPLL_CON1               (clk_apmixed_base + 0x284)
#define MPLL_PWR_CON0           (clk_apmixed_base + 0x28C)

#define APLL1_CON0              (clk_apmixed_base + 0x2A0)
#define APLL1_CON1              (clk_apmixed_base + 0x2A4)
#define APLL1_CON2              (clk_apmixed_base + 0x2A8)
#define APLL1_CON3              (clk_apmixed_base + 0x2AC)
#define APLL1_PWR_CON0          (clk_apmixed_base + 0x2B0)

#define APLL2_CON0              (clk_apmixed_base + 0x2B4)
#define APLL2_CON1              (clk_apmixed_base + 0x2B8)
#define APLL2_CON2              (clk_apmixed_base + 0x2BC)
#define APLL2_CON3              (clk_apmixed_base + 0x2C0)
#define APLL2_PWR_CON0          (clk_apmixed_base + 0x2C4)

#define CLK_MODE                (clk_cksys_base + 0x000)
#define CLK_CFG_UPDATE          (clk_cksys_base + 0x004)
#define TST_SEL_0               (clk_cksys_base + 0x020)
#define TST_SEL_1               (clk_cksys_base + 0x024)
#define TST_SEL_2               (clk_cksys_base + 0x028)
#define CLK_CFG_0               (clk_cksys_base + 0x040)
#define CLK_CFG_1               (clk_cksys_base + 0x050)
#define CLK_CFG_2               (clk_cksys_base + 0x060)
#define CLK_CFG_3               (clk_cksys_base + 0x070)
#define CLK_CFG_4               (clk_cksys_base + 0x080)
#define CLK_CFG_5               (clk_cksys_base + 0x090)
#define CLK_CFG_6               (clk_cksys_base + 0x0A0) 
#define CLK_MISC_CFG_0          (clk_cksys_base + 0x104)
#define CLK_DBG_CFG             (clk_cksys_base + 0x10C)
#define CLK_SCP_CFG_0           (clk_cksys_base + 0x200)
#define CLK_SCP_CFG_1           (clk_cksys_base + 0x204)
#define CLK26CALI_0             (clk_cksys_base + 0x220)
#define CLK26CALI_1             (clk_cksys_base + 0x224)
#define CKSTA_REG               (clk_cksys_base + 0x22C)


#define TOP_CKMUXSEL            (clk_infracfg_ao_base + 0x00)
#define TOP_CKDIV1              (clk_infracfg_ao_base + 0x08)

#define INFRA_PDN_SET0          (clk_infracfg_ao_base + 0x0080)
#define INFRA_PDN_CLR0          (clk_infracfg_ao_base + 0x0084)
#define INFRA_PDN_SET1          (clk_infracfg_ao_base + 0x0088)
#define INFRA_PDN_CLR1          (clk_infracfg_ao_base + 0x008C)
#define INFRA_PDN_STA0          (clk_infracfg_ao_base + 0x0090)
#define INFRA_PDN_STA1          (clk_infracfg_ao_base + 0x0094)

#define TOPAXI_PROT_EN          (clk_infracfg_ao_base + 0x0220)
#define TOPAXI_PROT_STA1        (clk_infracfg_ao_base + 0x0228)

#define AUDIO_TOP_CON0          (clk_audio_base + 0x0000)
                                
            
#define MFG_CG_CON              (clk_mfgcfg_base + 0)
#define MFG_CG_SET              (clk_mfgcfg_base + 4)
#define MFG_CG_CLR              (clk_mfgcfg_base + 8)

             
#define DISP_CG_CON0            (clk_mmsys_config_base + 0x100)
#define DISP_CG_SET0            (clk_mmsys_config_base + 0x104)
#define DISP_CG_CLR0            (clk_mmsys_config_base + 0x108)
#define DISP_CG_CON1            (clk_mmsys_config_base + 0x110)
#define DISP_CG_SET1            (clk_mmsys_config_base + 0x114)
#define DISP_CG_CLR1            (clk_mmsys_config_base + 0x118)

#define MMSYS_DUMMY             (clk_mmsys_config_base + 0x890)
#define	SMI_LARB_BWL_EN_REG     (clk_mmsys_config_base + 0x21050)

#define IMG_CG_CON              (clk_imgsys_base + 0x0000)
#define IMG_CG_SET              (clk_imgsys_base + 0x0004)
#define IMG_CG_CLR              (clk_imgsys_base + 0x0008)

                                
#define VDEC_CKEN_SET           (clk_vdec_gcon_base + 0x0000)
#define VDEC_CKEN_CLR           (clk_vdec_gcon_base + 0x0004)
#define LARB_CKEN_SET           (clk_vdec_gcon_base + 0x0008)
#define LARB_CKEN_CLR           (clk_vdec_gcon_base + 0x000C)

#define MJC_CG_CON              (clk_mjc_config_base + 0x0000)
#define MJC_CG_SET              (clk_mjc_config_base + 0x0004)
#define MJC_CG_CLR              (clk_mjc_config_base + 0x0008)

#define VENC_CG_CON             (clk_venc_gcon_base + 0x0)
#define VENC_CG_SET             (clk_venc_gcon_base + 0x4)
#define VENC_CG_CLR             (clk_venc_gcon_base + 0x8)

           



enum {
    CG_INFRA0  = 0,
    CG_INFRA1  = 1,
    CG_DISP0   = 2,
    CG_DISP1   = 3,
    CG_IMAGE   = 4,
    CG_MFG     = 5,
    CG_AUDIO   = 6,
    CG_VDEC0   = 7,
    CG_VDEC1   = 8,
    CG_MJC     = 9,
    CG_VENC    = 10,
    NR_GRPS    = 11,
};

enum cg_clk_id{                                 
    MT_CG_INFRA_PMIC_TMR            = 0,        
    MT_CG_INFRA_PMIC_AP             = 1,        
    MT_CG_INFRA_PMIC_MD             = 2,        
    MT_CG_INFRA_PMIC_CONN           = 3,        
    MT_CG_INFRA_SCPSYS              = 4,        
    MT_CG_INFRA_SEJ                 = 5,        
    MT_CG_INFRA_APXGPT              = 6,        
    MT_CG_INFRA_USB                 = 7,        
    MT_CG_INFRA_ICUSB               = 8,        
    MT_CG_INFRA_GCE                 = 9,        
    MT_CG_INFRA_THERM               = 10,       
    MT_CG_INFRA_I2C0                = 11,       
    MT_CG_INFRA_I2C1                = 12,       
    MT_CG_INFRA_I2C2                = 13,       
    MT_CG_INFRA_PWM1                = 16,       
    MT_CG_INFRA_PWM2                = 17,       
    MT_CG_INFRA_PWM3                = 18,       
    MT_CG_INFRA_PWM                 = 21,       
    MT_CG_INFRA_UART0               = 22,       
    MT_CG_INFRA_UART1               = 23,       
    MT_CG_INFRA_UART2               = 24,       
    MT_CG_INFRA_UART3               = 25,       
    MT_CG_INFRA_USB_MCU             = 26,       
    MT_CG_INFRA_MD2MD_CCIF_0        = 27,       
    MT_CG_INFRA_MD2MD_CCIF_1        = 28,       
    MT_CG_INFRA_MD2MD_CCIF_2        = 29,       
    MT_CG_INFRA_BTIF                = 31,       
   
    MT_CG_INFRA_MD2MD_CCIF_3        = 0  +32,       
    MT_CG_INFRA_SPI                 = 1  +32,       
    MT_CG_INFRA_MSDC_0              = 2  +32,       
    MT_CG_INFRA_MD2MD_CCIF_4        = 3  +32,       
    MT_CG_INFRA_MSDC_1              = 4  +32,       
    MT_CG_INFRA_MSDC_2              = 5  +32,       
    MT_CG_INFRA_MSDC_3              = 6  +32,       
    MT_CG_INFRA_MD2MD_CCIF_5        = 7  +32,       
    MT_CG_INFRA_GCPU                = 8  +32,       
    MT_CG_INFRA_AUXADC              = 10 +32,       
    MT_CG_INFRA_CPUM				= 11 +32,       
    MT_CG_INFRA_APDMA               = 18 +32,       
    MT_CG_INFRA_DEVICE_APC          = 20 +32,       
    MT_CG_INFRA_L2C_SRAM            = 22 +32,       
    MT_CG_INFRA_CCIF_AP             = 23 +32,       
    MT_CG_INFRA_DEBUGSYS            = 24 +32,       
    MT_CG_INFRA_AUDIO               = 25 +32,       
    MT_CG_INFRA_CCIF_MD             = 26 +32,       
    MT_CG_INFRA_DRAMC_F26M          = 31 +32,       

    MT_CG_DISP0_SMI_COMMON          = 0  +64,       
    MT_CG_DISP0_SMI_LARB0           = 1  +64,       
    MT_CG_DISP0_CAM_MDP             = 2  +64,       
    MT_CG_DISP0_MDP_RDMA            = 3  +64,       
    MT_CG_DISP0_MDP_RSZ0            = 4  +64,       
    MT_CG_DISP0_MDP_RSZ1            = 5  +64,       
    MT_CG_DISP0_MDP_TDSHP           = 6  +64,       
    MT_CG_DISP0_MDP_WDMA            = 7  +64,       
    MT_CG_DISP0_MDP_WROT            = 8  +64,       
    MT_CG_DISP0_FAKE_ENG            = 9  +64,       
    MT_CG_DISP0_DISP_OVL0           = 10 +64,       
    MT_CG_DISP0_DISP_OVL1           = 11 +64,       
    MT_CG_DISP0_DISP_RDMA0          = 12 +64,       
    MT_CG_DISP0_DISP_RDMA1          = 13 +64,       
    MT_CG_DISP0_DISP_WDMA0          = 14 +64,       
    MT_CG_DISP0_DISP_COLOR          = 15 +64,       
    MT_CG_DISP0_DISP_CCORR          = 16 +64,       
    MT_CG_DISP0_DISP_AAL            = 17 +64,       
    MT_CG_DISP0_DISP_GAMMA          = 18 +64,       
    MT_CG_DISP0_DISP_DITHER         = 19 +64,       
    MT_CG_DISP0_DISP_UFOE           = 20 +64,       
    MT_CG_DISP0_LARB4_AXI_ASIF_MM   = 21 +64,       
    MT_CG_DISP0_LARB4_AXI_ASIF_MJC  = 22 +64,       
    MT_CG_DISP0_DISP_WDMA1          = 23 +64,       
    MT_CG_DISP0_UFOD_RDMA0_L0       = 24 +64,       
    MT_CG_DISP0_UFOD_RDMA0_L1       = 25 +64,       
    MT_CG_DISP0_UFOD_RDMA0_L2       = 26 +64,       
    MT_CG_DISP0_UFOD_RDMA0_L3       = 27 +64,       
    MT_CG_DISP0_UFOD_RDMA1_L0       = 28 +64,       
    MT_CG_DISP0_UFOD_RDMA1_L1       = 29 +64,       
    MT_CG_DISP0_UFOD_RDMA1_L2       = 30 +64,       
    MT_CG_DISP0_UFOD_RDMA1_L3       = 31 +64,       
    
    MT_CG_DISP1_DISP_PWM_MM         = 0  +96,      
    MT_CG_DISP1_DISP_PWM_26M        = 1  +96,      
    MT_CG_DISP1_DSI_ENGINE          = 2  +96,      
    MT_CG_DISP1_DSI_DIGITAL         = 3  +96,      
    MT_CG_DISP1_DPI_PIXEL           = 4  +96,      
    MT_CG_DISP1_DPI_ENGINE          = 5  +96,      
    
    MT_CG_IMAGE_LARB2_SMI           = 0  +128,      
    MT_CG_IMAGE_CAM_SMI             = 5  +128,      
    MT_CG_IMAGE_CAM_CAM             = 6  +128,      
    MT_CG_IMAGE_SEN_TG              = 7  +128,      
    MT_CG_IMAGE_SEN_CAM             = 8  +128,      
    MT_CG_IMAGE_CAM_SV              = 9  +128,      
    MT_CG_IMAGE_FD                  = 11 +128,      
    
    MT_CG_MFG_BG3D					= 0  +160,        
    
    MT_CG_AUDIO_AFE                 = 2  +192,      
    MT_CG_AUDIO_I2S                 = 6  +192,      
    MT_CG_AUDIO_ADDA4               = 7  +192,      
    MT_CG_AUDIO_22M                 = 8  +192,      
    MT_CG_AUDIO_24M                 = 9  +192,      
    MT_CG_AUDIO_APLL2_TUNER         = 18 +192,      
    MT_CG_AUDIO_APLL_TUNER          = 19 +192,      
    
    MT_CG_VDEC0_VDEC				= 0  +224,      
                                                
    MT_CG_VDEC1_LARB				= 0  +256,      
                                                
    MT_CG_MJC_SMI_LARB              = 0  +288,      
    MT_CG_MJC_TOP_GROUP0            = 1  +288,      
    MT_CG_MJC_TOP_GROUP1            = 2  +288,      
    MT_CG_MJC_TOP_GROUP2            = 3  +288,      
    MT_CG_MJC_LARB4_AXI_ASIF        = 5  +288,      
                                                
    MT_CG_VENC_LARB                 = 0  +320,      
    MT_CG_VENC_VENC                 = 4  +320,      
    MT_CG_VENC_JPGENC               = 8  +320,      
    MT_CG_VENC_JPGDEC               = 12 +320,      


    CG_INFRA0_FROM                   = MT_CG_INFRA_PMIC_TMR,
    CG_INFRA0_TO                     = MT_CG_INFRA_BTIF,
    NR_INFRA0_CLKS                   = 32,
    
    CG_INFRA1_FROM                   = MT_CG_INFRA_MD2MD_CCIF_3,
    CG_INFRA1_TO                     = MT_CG_INFRA_DRAMC_F26M,
    NR_INFRA1_CLKS                   = 32,
    
    CG_DISP0_FROM                   = MT_CG_DISP0_SMI_COMMON,
    CG_DISP0_TO                     = MT_CG_DISP0_UFOD_RDMA1_L3,
    NR_DISP0_CLKS                   = 32,

    CG_DISP1_FROM                   = MT_CG_DISP1_DISP_PWM_MM,
    CG_DISP1_TO                     = MT_CG_DISP1_DPI_ENGINE,
    NR_DISP1_CLKS                   = 10,
    
    CG_IMAGE_FROM                   = MT_CG_IMAGE_LARB2_SMI,
    CG_IMAGE_TO                     = MT_CG_IMAGE_FD,
    NR_IMAGE_CLKS                   = 7,
    
    CG_MFG_FROM                     = MT_CG_MFG_BG3D,
    CG_MFG_TO                       = MT_CG_MFG_BG3D,
    NR_MFG_CLKS                     = 1,
    
    CG_AUDIO_FROM                   = MT_CG_AUDIO_AFE,
    CG_AUDIO_TO                     = MT_CG_AUDIO_APLL_TUNER,
    NR_AUDIO_CLKS                   = 9,
    
    CG_VDEC0_FROM                   = MT_CG_VDEC0_VDEC,
    CG_VDEC0_TO                     = MT_CG_VDEC0_VDEC,
    NR_VDEC0_CLKS                   = 1,
    
    CG_VDEC1_FROM                   = MT_CG_VDEC1_LARB,
    CG_VDEC1_TO                     = MT_CG_VDEC1_LARB,
    NR_VDEC1_CLKS                   = 1,
    
    CG_MJC_FROM                     = MT_CG_MJC_SMI_LARB,
    CG_MJC_TO                       = MT_CG_MJC_LARB4_AXI_ASIF,
    NR_MJC_CLKS                     = 5,
    
    CG_VENC_FROM                    = MT_CG_VENC_LARB,
    CG_VENC_TO                      = MT_CG_VENC_JPGDEC,
    NR_VENC_CLKS                    = 4,
    
    NR_CLKS                         = 333,
    
    MT_CG_PERI_UART0                = 0,
    MT_CG_PERI_UART1                = 0,
    MT_CG_PERI_UART2                = 0,
    MT_CG_PERI_UART3                = 0,
    MT_CG_PERI_I2C0                 = 0,
    MT_CG_PERI_I2C1                 = 0,
    MT_CG_PERI_I2C2                 = 0,
    MT_CG_PERI_I2C3                 = 0,
    MT_CG_PERI_I2C4                 = 0,
    MT_CG_PERI_MSDC30_0             = 0,
};

enum {
	
    MT_MUX_MM           = 0,
    MT_MUX_DDRPHY       = 1,
    MT_MUX_MEM          = 2,
    MT_MUX_AXI          = 3,

    
    MT_MUX_MFG          = 4,
    MT_MUX_VDEC         = 5,
    MT_MUX_PWM          = 6,

    
    MT_MUX_SPI          = 7,
    MT_MUX_UART         = 8,
    MT_MUX_CAMTG        = 9,

    
    MT_MUX_MSDC30_1     = 10,
    MT_MUX_MSDC50_0     = 11,
    MT_MUX_MSDC50_0_hclk = 12,

    
    MT_MUX_AUDINTBUS    = 13,
    MT_MUX_AUDIO        = 14,
    MT_MUX_MSDC30_3     = 15,
    MT_MUX_MSDC30_2     = 16,
    
    
    MT_MUX_MJC          = 17,
    MT_MUX_SCP          = 18,
    MT_MUX_PMICSPI      = 19,
    
    
    MT_MUX_AUD2         = 20,
    MT_MUX_AUD1         = 21,
    MT_MUX_SCAM         = 22,
    MT_MUX_DPI0         = 23,

    
    MT_MUX_USB          = 24,
    

    NR_MUXS             = 25,
};

enum {
    ARMCA7PLL  = 0,
    MAINPLL    = 1,
    MSDCPLL    = 2,
    UNIVPLL    = 3,
    MMPLL      = 4,
    VENCPLL    = 5,
    TVDPLL     = 6,
    MPLL       = 7,
    APLL1      = 8,
    APLL2      = 9,
    NR_PLLS    = 10,
};

enum {
    SYS_MD1       = 0,
    SYS_MD2       = 1,
    SYS_CONN      = 2,
    SYS_DIS       = 3,
    SYS_MFG       = 4,
    SYS_ISP       = 5,
    SYS_VDE       = 6,
    SYS_MJC       = 7,
    SYS_VEN       = 8,
    SYS_AUD       = 9,
    NR_SYSS       = 10,
};

enum {
    MT_LARB_DISP = 0,
    MT_LARB_VDEC = 1,
    MT_LARB_IMG  = 2,
    MT_LARB_VENC = 3,
    MT_LARB_MJC  = 4,
};

enum {
    LARB_MONITOR_LEVEL_HIGH     = 10,
    LARB_MONITOR_LEVEL_MEDIUM   = 20,
    LARB_MONITOR_LEVEL_LOW      = 30,
};

struct larb_monitor {
    struct list_head link;
    int level;
    void (*backup)(struct larb_monitor *h, int larb_idx);       
    void (*restore)(struct larb_monitor *h, int larb_idx);      
};

enum monitor_clk_sel_0{
    no_clk_0             = 0,
    AD_UNIV_624M_CK      = 5,
    AD_UNIV_416M_CK      = 6,
    AD_UNIV_249P6M_CK    = 7,
    AD_UNIV_178P3M_CK_0  = 8,
    AD_UNIV_48M_CK       = 9,
    AD_USB_48M_CK        = 10,
    rtc32k_ck_i_0        = 20,
    AD_SYS_26M_CK_0      = 21,
};
enum monitor_clk_sel{
    no_clk               = 0,
    AD_SYS_26M_CK        = 1,
    rtc32k_ck_i          = 2,
    clkph_MCLK_o         = 7,
    AD_DPICLK            = 8,
    AD_MSDCPLL_CK        = 9,
    AD_MMPLL_CK          = 10,
    AD_UNIV_178P3M_CK    = 11,
    AD_MAIN_H156M_CK     = 12,
    AD_VENCPLL_CK        = 13,
};

enum ckmon_sel{
    clk_ckmon0           = 0,
    clk_ckmon1           = 1,
    clk_ckmon2           = 2,
    clk_ckmon3           = 3,
};

enum idle_mode{
    dpidle               = 0,
    soidle               = 1,
    slidle               = 2,
};

extern void register_larb_monitor(struct larb_monitor *handler);
extern void unregister_larb_monitor(struct larb_monitor *handler);

extern int enable_clock(enum cg_clk_id id, char *mod_name);
extern int disable_clock(enum cg_clk_id id, char *mod_name);
extern int mt_enable_clock(enum cg_clk_id id, char *mod_name);
extern int mt_disable_clock(enum cg_clk_id id, char *mod_name);

extern int enable_clock_ext_locked(int id, char *mod_name);
extern int disable_clock_ext_locked(int id, char *mod_name);

extern int clock_is_on(int id);

extern int clkmux_sel(int id, unsigned int clksrc, char *name);
extern void enable_mux(int id, char *name);
extern void disable_mux(int id, char *name);

extern void clk_set_force_on(int id);
extern void clk_clr_force_on(int id);
extern int clk_is_force_on(int id);

extern int enable_pll(int id, char *mod_name);
extern int disable_pll(int id, char *mod_name);

extern int pll_hp_switch_on(int id, int hp_on);
extern int pll_hp_switch_off(int id, int hp_off);

extern int pll_fsel(int id, unsigned int value);
extern int pll_is_on(int id);

extern int enable_subsys(int id, char *mod_name);
extern int disable_subsys(int id, char *mod_name);

extern int subsys_is_on(int id);
extern int md_power_on(int id);
extern int md_power_off(int id, unsigned int timeout);
extern int conn_power_on(void);
extern int conn_power_off(void);


extern void set_mipi26m(int en);
extern void set_ada_ssusb_xtal_ck(int en);

const char* grp_get_name(int id);
extern int clkmgr_is_locked(void);



extern void clk_stat_check(int id);
extern void slp_check_pm_mtcmos_pll(void);
extern void clk_misc_cfg_ops(bool flag);

#endif
