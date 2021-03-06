#include <linux/scatterlist.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/sdio.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/slab.h>
#include "mt_sd.h"
#include "sdio_autok.h"
#define AUTOK_VERSION_NO (0x62900010)
#define SDIO_AUTOK_ID (1)

#ifdef MT6582LTE
#undef MT6582LTE
#endif

#ifdef MT6592LTE
#undef MT6592LTE
#endif

#ifdef MT6595WIFI
#undef MT6595WIFI
#endif

#ifdef MT6752WIFI
#undef MT6752WIFI
#endif

#define MT6752WIFI

#define AUTOK_CMD_TIMES     (20)
#define AUTOK_RDAT_TIMES    (1)
#define AUTOK_WDAT_TIMES    (1)

#define CMD_TIMEOUT         (HZ/10 * 5)   

#define AUTOK_CYC_ALG_2

#define AUTOK_RDAT_ACC

#define AUTOK_TUNING_INACCURACY         (2)
#define AUTOK_CMD_EDGE_MATRIX_SCAN      (1)
#define AUTOK_CKGEN_ALLOW_MAX           (2)

#define AUTOK_RDAT_FBOUND_TH            (autok_rdat_fbound_th)

#define AUTOK_CMD_SCAN_RANGE_STG1       (pad_delay_period_cycle/4)
#define AUTOK_CMD_SCAN_DE_RANGE_STG2    (8)
#define AUTOK_SKIP_CMDTUNE_ON_STG2      (32)

#define AUTOK_CMDPAT_CHK_SHIFT          (6)

#define TUNING_TEST_TIME (64)

#define AUTOK_CMDMAT_CROSS_MAR          (8)

#define AUTOK_VCORE_SCAN_NUM            2

#define AUTOK_SS_IO_RING_TH             (2407)
#define AUTOK_SS_CORE_RING_TH           (3162)
#define AUTOK_TT_CORE_RING_TH           (3307)

#define AUTOK_TINY_MAR_TH_TOP           (15)
#define AUTOK_TINY_MAR_TH_BTM           (8)
#define AUTOK_TINY_MAR_R_MAR_MIN        (3)
#define AUTOK_TINY_MAR_R_SHIFT_MAX      (2)
#define AUTOK_TINY_MAR_PAT              (0xAA55)

#define SCALE_CMD_RSP_DLY_SEL         (32)
#define SCALE_CKGEN_MSDC_DLY_SEL      (32)
#define SCALE_PAD_TUNE_CMDRDLY        (32)
#define SCALE_CMDMAT_RSP_DLY_SEL      (SCALE_CMD_RSP_DLY_SEL/2)
#define SCALE_DATA_DRIVING            (8)
#define SCALE_INT_DAT_LATCH_CK_SEL    (8)
#if 1
#define SCALE_IOCON_RDSPL             (2)
#define SCALE_PAD_TUNE_DATRDDLY       (32)
#else
#define SCALE_IOCON_RD0SPL            (2)
#define SCALE_IOCON_RD1SPL            (2)
#define SCALE_IOCON_RD2SPL            (2)
#define SCALE_IOCON_RD3SPL            (2)
#define SCALE_DAT_RDDLY0_D0           (32)
#define SCALE_DAT_RDDLY0_D1           (32)
#define SCALE_DAT_RDDLY0_D2           (32)
#define SCALE_DAT_RDDLY0_D3           (32)
#endif
#define SCALE_WRDAT_CRCS_TA_CNTR      (8)
#define SCALE_IOCON_WD0SPL            (2)
#define SCALE_PAD_TUNE_DATWRDLY       (32)

#define MAX_DELAY_VARIATION_DUE_TO_TEMPERATURE_IN_PS (2600) 
#define F208M_CYCLE_IN_PS (4808)

#define MIN_CLK_GEN_DELAY_IN_PS (9120)
#define MAX_CLK_GEN_DELAY_IN_PS (24320)
#define MIN_PAD_DELAY_IN_PS (2400) 
#define MAX_PAD_DELAY_IN_PS (6400)
#define SCALE_OF_CLK_GEN_2_PAD_DELAY (MIN_CLK_GEN_DELAY_IN_PS/MIN_PAD_DELAY_IN_PS)

#define DIV_CEIL_FUNC(_n,_d) ((_n)/(_d)+(((_n)%(_d)==0)?0:1))
#define ABS_DIFF(_a,_b)      (((_a)>=(_b))?((_a)-(_b)):((_b)-(_a)))
#define THRESHOLD_VAL(_v,_t) (((_v)>=(_t))?(_t):(_v))
#define MAX_GET(_v,_t)       (((_v)>=(_t))?(_v):(_t))
#define MIN_GET(_v,_t)       (((_v)>=(_t))?(_t):(_v))
#define FREQ_MHZ_2_PERIOD_CYCLE_IN_PS(_Mhz) (1000000L/(_Mhz))

#define MIN_SCORE_OF_CLK_GEN_IN_ONE_CYCLE(_periodCycle) \
       (DIV_CEIL_FUNC((SCALE_CKGEN_MSDC_DLY_SEL*(_periodCycle)), MAX_CLK_GEN_DELAY_IN_PS))

#define MIN_SCORE_OF_PAD_DELAY_IN_ONE_CYCLE(_periodCycle) \
       (DIV_CEIL_FUNC((SCALE_PAD_TUNE_CMDRDLY*(_periodCycle)), MAX_PAD_DELAY_IN_PS))

#define MAX_SCALE_OF_CLK_GEN_IN_ONE_CYCLE(_periodCycle) \
       (DIV_CEIL_FUNC((SCALE_CKGEN_MSDC_DLY_SEL*(_periodCycle)), MIN_CLK_GEN_DELAY_IN_PS))

#define MAX_SCORE_OF_PAD_DELAY_AGAINST_TEMP_VAR \
       (DIV_CEIL_FUNC((SCALE_PAD_TUNE_CMDRDLY*MAX_DELAY_VARIATION_DUE_TO_TEMPERATURE_IN_PS), MIN_PAD_DELAY_IN_PS))

#define MIN_SCORE_OF_PAD_DELAY_AGAINST_TEMP_VAR \
       (DIV_CEIL_FUNC((SCALE_PAD_TUNE_CMDRDLY*MAX_DELAY_VARIATION_DUE_TO_TEMPERATURE_IN_PS), MAX_PAD_DELAY_IN_PS))

#define USER_DEF_MAX_SCORE_OF_PAD_DELAY_AGAINST_TEMP_VAR(_x,_y) \
       (DIV_CEIL_FUNC(((_x)*MAX_DELAY_VARIATION_DUE_TO_TEMPERATURE_IN_PS), (_y)))

#define MIN_DATA_SCORE (MIN_SCORE_OF_PAD_DELAY_AGAINST_TEMP_VAR - AUTOK_TUNING_INACCURACY)

#ifdef MT6290
#define AUTOK_PRINT(_fmt, args...)        printf("[AUTO_K]" _fmt, ## args)
#else
#define AUTOK_PSIZE PAGE_SIZE
char autok_single[128];
char *log_info = NULL;
int autok_size = 0;
int total_msg_size = 0;
#define AUTOK_PRINT(_fmt, args...)  \
do{ \
    autok_size = snprintf(autok_single, 128, _fmt, ## args);     \
      if(log_info != NULL && total_msg_size+autok_size < LOG_SIZE){ \
      memcpy(log_info+total_msg_size, autok_single, autok_size);    \
      total_msg_size += autok_size; \
    }   \
    printk(KERN_ERR "[AUTO_K]" _fmt, ## args);   \
}while(0)
#endif

#ifdef AUTOK_DEBUG
#define AUTOK_ERR()     while(1)
#else
#define AUTOK_ERR()     goto exit
#endif

#define msdc_retry(expr, retry, cnt,id) \
    do { \
        int backup = cnt; \
        while (retry) { \
            if (!(expr)) break; \
            if (cnt-- == 0) { \
                retry--; mdelay(1); cnt = backup; \
            } \
        } \
        WARN_ON(retry == 0); \
    } while(0)

#define msdc_reset(id) \
    do { \
        int retry = 3, cnt = 1000; \
        sdr_set_bits(MSDC_CFG, MSDC_CFG_RST); \
        mb(); \
        msdc_retry(sdr_read32(MSDC_CFG) & MSDC_CFG_RST, retry, cnt,id); \
    } while(0)

#define msdc_clr_int() \
    do { \
        volatile u32 val = sdr_read32(MSDC_INT); \
        sdr_write32(MSDC_INT, val); \
    } while(0)

#define msdc_clr_fifo(id) \
    do { \
        int retry = 3, cnt = 1000; \
        sdr_set_bits(MSDC_FIFOCS, MSDC_FIFOCS_CLR); \
        msdc_retry(sdr_read32(MSDC_FIFOCS) & MSDC_FIFOCS_CLR, retry, cnt,id); \
    } while(0)

#define msdc_reset_hw(id) \
    msdc_reset(id); \
    msdc_clr_fifo(id); \
    msdc_clr_int(); 

#define msdc_txfifocnt()   ((sdr_read32(MSDC_FIFOCS) & MSDC_FIFOCS_TXCNT) >> 16)
#define msdc_rxfifocnt()   ((sdr_read32(MSDC_FIFOCS) & MSDC_FIFOCS_RXCNT) >> 0)

typedef enum
{
    E_RESULT_PASS = 0,
    E_RESULT_CMD_CRC = 1,
    E_RESULT_W_CRC = 2,
    E_RESULT_R_CRC = 3,
    E_RESULT_ERR = 4,
    E_RESULT_START = 5,
    E_RESULT_PW_SMALL = 6,
    E_RESULT_KEEP_OLD = 7,
    E_RESULT_TO = 8,
    E_RESULT_CMP_ERR = 9,
    E_RESULT_MAX
}E_RESULT_TYPE;

typedef enum
{
    AUTOK_CMD = 0,
    AUTOK_DATA,
    AUTOK_FAIL,
    AUTOK_DONE
}E_AUTOK_STATE;

typedef enum
{
    ERR_NONE = 0,
    ERR_OCCURE,
    PASS_AFTER_ERR,
    ERR_MAX
}E_AUTOK_ERR_STA;

typedef enum
{
    PERIOD_NONE = 0,
    PERIOD_F_FIRST_POS,
    PERIOD_F_FIRST_POS_DONE,
    PERIOD_F_SECOND_POS,
    PERIOD_L_FIRST_POS,
    PERIOD_L_FIRST_POS_DONE,
    PERIOD_L_SECOND_POS,
    PERIOD_DONE,
    PERIOD_DONE_2,
    PERIOD_MAX,
}E_AUTOK_PERIOD_STA;

typedef struct
{
    unsigned int interDelaySel;
    unsigned int cmdScore;
    unsigned int cmdPadSel;
    unsigned int cmdEdgeSel;
    unsigned int readScore;
    unsigned int readPadSel;
    unsigned int writeScore;
    unsigned int writePadSel;
}S_AUTOK_CKGEN_DATA;

typedef enum
{
    SEARCH_FIRST_PASS = 0,
    SEARCH_SECOND_PASS,
    SEARCH_PASS_REGION,
    PASS_REGION_GET,
    SEARCH_MAX
}E_AUTOK_DATA_STA;

typedef struct
{
    unsigned int raw_data;
    unsigned int score;
    unsigned int numOfzero;
    unsigned int fstPosErr;
    unsigned int fstPosErrEnd;
    unsigned int period;
}S_AUTOK_CMD_DLY;

typedef enum
{
    TUNING_STG1 = 0,
    TUNING_STG2,
    TUNING_STG_MAX
}E_AUTOK_TUNING_STAGE;

typedef enum
{
    RD_SCAN_NONE,
    RD_SCAN_PAD_BOUND_S,
    RD_SCAN_PAD_BOUND_E,
    RD_SCAN_PAD_BOUND_S_2,
    RD_SCAN_PAD_BOUND_E_2,
    RD_SCAN_PAD_MARGIN,
    
}AUTOK_RAWD_SCAN_STA_E;

typedef enum
{
    CMDPAT_NONE,
    CMDPAT_IDENTICAL,
    CMDPAT_HALF_IDENTICAL,
}AUTOK_CMDPAT_COMP_E;

typedef enum
{
    CMDPAT_REG_1_L=1,
    CMDPAT_REG_2_L,

}AUTOK_CMDPAT_SCEN_E;

typedef struct
{
    unsigned int RawData;
    
    unsigned int BoundReg1_S;
    unsigned int BoundReg1_E;
    unsigned int Reg1Cnt;
    unsigned int BoundReg2_S;
    unsigned int BoundReg2_E;
    unsigned int Reg2Cnt;

    unsigned char fInvalidCKGEN;
    unsigned char CurCKGEN;
    
}AUTOK_RAWD_SCAN_T, *P_AUTOK_RAWD_SCAN_T;

typedef struct {
    unsigned int PadDlyPeriodLen;
    unsigned int CKGenPeriodLen;
    unsigned int fCMDIntDlyConf;

    unsigned int fFBound;
    unsigned int FBoundCK;
    unsigned int fLBound;
    unsigned int LBoundCK;

    unsigned int fHoleCK;
    unsigned int HoleCK;

    unsigned int FBoundMidRefCMD;
    unsigned int FBoundCKRefCMD;
    unsigned int FBoundCntRefCMD;
    unsigned int fFBoundRefCMD;
    
}AUTOK_CYC_SCAN_RES_T, *P_AUTOK_CYC_SCAN_RES_T;

typedef struct {
    unsigned int CmdPadDly;
    unsigned int CmdPadDlyStg1Bak;
    unsigned int CmdIntDly;
    unsigned int fTimingShiftLarge;
    unsigned int fRetOk;
}AUTOK_CMD_TUNE_RES_T, *P_AUTOK_CMD_TUNE_RES_T;

typedef struct {
    unsigned int fRetOk;
    unsigned int PadDlyRefRD;
    unsigned int CKGenSel;
}AUTOK_RD_TUNE_RES_T, *P_AUTOK_RD_TUNE_RES_T;

typedef struct
{
    unsigned int ck_s_b4;
    unsigned int raw_s_b4;
    unsigned int ck_e_b4;
    unsigned int raw_e_b4;
    AUTOK_CMDPAT_SCEN_E scen;
    int diff_b4;

    unsigned int cmd_int_shift;    
    unsigned int raw_s_sh;
    unsigned int raw_e_sh;
}AUTOK_CMDPAT_DOUBLE_CHK_T, *P_AUTOK_CMDPAT_DOUBLE_CHK_T;

typedef struct
{
    int pad_trans_s1;
    int pad_trans_e1;
    int pad_trans_m1;
    int pad_trans_cnt1;
    
    int pad_trans_s2;
    int pad_trans_e2;
    int pad_trans_m2;
    int pad_trans_cnt2;
    
}AUTOK_CMDMAT_CHAR_POS, *P_AUTOK_CMDMAT_CHAR_POS;


static const unsigned int tuning_data[] =       {0xAA55AA55,0xAA558080,0x807F8080,
                                                 0x807F7F7F,0x807F7F7F,0x404040BF,
                                                 0xBFBF40BF,0xBFBF2020,0x20DF2020,
                                                 0x20DFDFDF,0x101010EF,0xEFEF10EF,
                                                 0xEFEF0808,0x08F70808,0x08F7F7F7,
                                                 0x040404FB,0xFBFB04FB,0xFBFB0202,
                                                 0x02FD0202,0x02FDFDFD,0x010101FE,
                                                 0xFEFE01FE,0xFEFE0000,0x00FF0000,
                                                 0x00FFFFFF,0x000000FF,0xFFFF00FF,
                                                 0xFFFF0000,0xFF0FFF00,0xFFCCC3CC,
                                                 0xC33CCCFF,0xFEFFFEEF,0xFFDFFFDD,
                                                 0xFFFBFFFB,0xBFFF7FFF,0x77F7BDEF,
                                                 0xFFF0FFF0,0x0FFCCC3C,0xCC33CCCF,
                                                 0xFFEFFFEE,0xFFFDFFFD,0xDFFFBFFF,
                                                 0xBBFFF7FF,0xF77F7BDE};

#define TUNING_DATA_NO  (sizeof(tuning_data)/sizeof(unsigned int))

#if 1
static const unsigned char tuning_cmd[] = { 0x55, 0xAA, 0x5A, 0xA5,   
                                            0x55, 0xAA, 0x5A, 0xA5,   
                                            0x55, 0xAA, 0x5A, 0xA5,   
                                            0x55, 0xAA, 0x5A, 0xA5,   };
#else
static const unsigned char tuning_cmd[] = { 0x00, 0xFF, 0x0F, 0xF0,   
                                            0x33, 0xCC, 0x3C, 0xC3,      
                                            0x55, 0xAA, 0x5A, 0xA5,   
                                            0x77, 0x88, 0x78, 0x87,   
};
#endif

#define TUNING_CMD_NO  (sizeof(tuning_cmd)/sizeof(unsigned char))

static U_AUTOK_INTERFACE_DATA **g_pp_autok_data = NULL;

unsigned int mt65x2_vcore_tbl[] = {
    600000, 606250, 612500, 618750, 625000, 631250, 637500, 643750, 
    650000, 656250, 662500, 668750, 675000, 681250, 687500, 693750, 
    700000, 706250, 712500, 718750, 725000, 731250, 737500, 743750, 
    750000, 756250, 762500, 768750, 775000, 781250, 787500, 793750, 
    800000, 806250, 812500, 818750, 825000, 831250, 837500, 843750, 
    850000, 856250, 862500, 868750, 875000, 881250, 887500, 893750, 
    900000, 906250, 912500, 918750, 925000, 931250, 937500, 943750, 
    950000, 956250, 962500, 968750, 975000, 981250, 987500, 993750, 
    1000000, 1006250, 1012500, 1018750, 1025000, 1031250, 1037500, 1043750, 
    1050000, 1056250, 1062500, 1068750, 1075000, 1081250, 1087500, 1093750, 
    1100000, 1106250, 1112500, 1118750, 1125000, 1131250, 1137500, 1143750, 
    1150000, 1156250, 1162500, 1168750, 1175000, 1181250, 1187500, 1193750, 
    1200000, 1206250, 1212500, 1218750, 1225000, 1231250, 1237500, 1243750, 
    1250000, 1256250, 1262500, 1268750, 1275000, 1281250, 1287500, 1293750, 
    1300000, 1306250, 1312500, 1318750, 1325000, 1331250, 1337500, 1343750, 
    1350000, 1356250, 1362500, 1368750, 1375000, 1381250, 1387500, 1393750, 
    1400000};

#if (AUTOK_VCORE_SCAN_NUM == 2)
unsigned int g_autok_vcore_sel[AUTOK_VCORE_SCAN_NUM] = {
    1000000, 1125000};
#elif (AUTOK_VCORE_SCAN_NUM == 3)
unsigned int g_autok_vcore_sel[AUTOK_VCORE_SCAN_NUM] = {
    950000, 1000000, 1181250};
#elif (AUTOK_VCORE_SCAN_NUM == 6)
unsigned int g_autok_vcore_sel[AUTOK_VCORE_SCAN_NUM] = {
    950000, 1000000, 1050000, 1068750, 1125000, 1181250};
#endif

static unsigned int g_test_write_pattern[TUNING_TEST_TIME*TUNING_DATA_NO];
static unsigned int g_test_read_pattern[TUNING_TEST_TIME];

S_AUTOK_CKGEN_DATA autok_ckg_data[SCALE_CKGEN_MSDC_DLY_SEL];
AUTOK_RAWD_SCAN_T autok_rdata_scan[SCALE_CKGEN_MSDC_DLY_SEL];
S_AUTOK_CMD_DLY autok_cmd_cmdrrdly[SCALE_CMD_RSP_DLY_SEL];
S_AUTOK_CMD_DLY autok_cmd_ckgdly[SCALE_CKGEN_MSDC_DLY_SEL];

#ifdef AUTOK_CYC_ALG_0
S_AUTOK_CMD_DLY autok_cmd_ckgdly_cmdrrdly0[SCALE_CKGEN_MSDC_DLY_SEL];
#endif

#if defined(AUTOK_CYC_ALG_0) && defined(AUTOK_CMD_TUNE_LEGACY)
unsigned int autok_cmddly_stop_bit[SCALE_CMD_RSP_DLY_SEL];
#endif

#ifdef AUTOK_CYC_ALG_2
unsigned int autok_paddly_per_cyc_eval = 64;
#endif

static unsigned int autok_rdat_fbound_th = 4;
static unsigned int autok_vcore_scan_num = AUTOK_VCORE_SCAN_NUM;
static unsigned int freq_mhz = 200;

#if defined(MT6582LTE)
static unsigned int gfIOSS = 0;
static unsigned int gfCoreTT = 0;
#endif
static unsigned int gfTinyMar = 0;
static unsigned int gfEqualVcore = 0;

static char g_tune_result_str[33];

unsigned char autok_param_name[E_AUTOK_DLY_PARAM_MAX][25] = {
    {"PAD_CMD_RESP_RXDLY"},
    {"CMD_RSP_TA_CNTR"},
    {"R_SMPL"},
    {"CKGEN_MSDC_DLY_SEL"},
    {"PAD_CMD_RXDLY"},
    {"INT_DAT_LATCH_CK_SEL"},
    {"R_D_SMPL"},
    {"PAD_DATA_RD_RXDLY"},
    {"WRDAT_CRCS_TA_CNTR"},
    {"W_D_SMPL"},
    {"PAD_DATA_WR_RXDLY"}
};

extern void mt_vcore_dvfs_volt_set_by_sdio(unsigned int volt);
extern unsigned int mt_vcore_dvfs_volt_get_by_sdio(void);
extern void pmic_config_interface(unsigned int, unsigned int, unsigned int, unsigned int);
#ifdef PMIC_MT6323
extern void pmic_read_interface(unsigned int, unsigned int *, unsigned int, unsigned int);
#else   
extern unsigned int mt6333_get_reg_value(unsigned int reg);
#endif

extern unsigned int sdio_get_rings(unsigned int *io_ring, unsigned int *core_ring);

extern unsigned int msdc_do_command(struct msdc_host   *host, 
                                      struct mmc_command *cmd,
                                      int                 tune,
                                      unsigned long       timeout);
extern int msdc_pio_read(struct msdc_host *host, struct mmc_data *data);
extern int msdc_pio_write(struct msdc_host* host, struct mmc_data *data);


int autok_start_rw(struct msdc_host *host, u8 *value, unsigned size, unsigned blocks, bool write)
{
    int ret = 0;
    void __iomem *base = host->base;
    struct mmc_data *data = host->data;
    struct mmc_command *cmd = host->mrq->cmd;
    
    
    data->blocks = blocks;
    data->error = 0;
    
    sg_init_one(data->sg, value, size);
    
    host->xfer_size = blocks * data->blksz;
    host->blksz = data->blksz;
    host->autocmd = 0;
    host->dma_xfer = 0;
    
    sdr_write32(SDC_BLK_NUM, blocks);
    
    if (msdc_txfifocnt() || msdc_rxfifocnt()) {
        printk("[%s][SD%d] register abnormal,please check!\n",__func__, host->id);
        msdc_reset_hw(host->id);
    }
    
    if((ret = msdc_do_command(host, cmd, 0, CMD_TIMEOUT)) != 0)
    {
        return ret;
    }
    
    if(write == 0)
    {
        if((ret = msdc_pio_read(host, data))!= 0)
            return ret;
    }
    else
    {
        if((ret = msdc_pio_write(host, data))!= 0)
            return ret;
    }
    
    return 0;
}

int autok_io_rw_extended(struct msdc_host *host, unsigned int u4Addr, unsigned int u4Func, void *pBuffer, unsigned int u4Len, bool write)
{
    int ret = 0;
    u8 *value = (u8 *) pBuffer;
    struct sdio_func *sdioFunc;
    struct mmc_request mrq = {NULL};
    struct mmc_command cmd = {0};
    struct mmc_data data = {0};
    struct scatterlist sg;
    unsigned remainder = u4Len;
    unsigned max_blocks;
    unsigned size;
    
    if((pBuffer==NULL) || (host==NULL))
    {
        printk("[%s] [ERR] pBuffer = %p, host = %p\n", __func__, pBuffer, host);
        return -1;
    }
    
    if( u4Len < 4 )
    {
        printk("[%s] [ERR] u4Len = %d\n", __func__, u4Len);
        return -1;
    }
    
    
    mrq.cmd = &cmd;
    mrq.data = &data;
    host->mrq = &mrq;
    
    
    cmd.opcode = SD_IO_RW_EXTENDED;
    cmd.arg = 0;
    cmd.arg = write ? 0x80000000 : 0x00000000;
    cmd.arg |= u4Func << 28;
    cmd.arg |= u4Addr << 9;
    cmd.flags = MMC_RSP_SPI_R5 | MMC_RSP_R5 | MMC_CMD_ADTC;
    cmd.data = &data;

        
    data.flags = write ? MMC_DATA_WRITE : MMC_DATA_READ;
    data.sg = &sg;
    data.sg_len = 1;
	
	host->data = &data;
    	
    sdioFunc = host->mmc->card->sdio_func[u4Func - 1];
    
    max_blocks = min(sdioFunc->card->host->max_blk_count,
		sdioFunc->card->host->max_seg_size / sdioFunc->cur_blksize);
    max_blocks = min(max_blocks, 511u);

	while (remainder >= sdioFunc->cur_blksize) {
		unsigned blocks;

		blocks = remainder / sdioFunc->cur_blksize;
		if (blocks > max_blocks)
			blocks = max_blocks;
		size = blocks * sdioFunc->cur_blksize;

		cmd.arg |= 0x08000000 | blocks;
		
		data.blksz = sdioFunc->cur_blksize;
		
        if((ret = autok_start_rw(host, value, size, blocks, write)) != 0)
            goto stop;
		
		remainder -= size;
		value += size;
	}
	
	
	while (remainder > 0) {
		size = remainder;

		cmd.arg &= ~(0x08000000);
		cmd.arg |= size;
		
		data.blksz = size;
		
        if((ret = autok_start_rw(host, value, size, 1, write)) != 0)
            goto stop;

		remainder -= size;
		value += size;
	}
    
stop:
    host->mrq = NULL;
    host->data = NULL;
    host->dma_xfer = 0;
    host->blksz = 0;
    return ret;
}

int autok_io_rw_direct(struct msdc_host *host, unsigned int u4Addr, unsigned int u4Func, void *pBuffer, unsigned int u4Len, bool write)
{
    int ret = 0;
    u8 *value = (u8 *) pBuffer;
    void __iomem *base = host->base;
    struct mmc_command cmd = {0};
    struct mmc_request mrq = {NULL};
    
    if((pBuffer==NULL) || (host==NULL))
    {
        printk("[%s] [ERR] pBuffer = %p, host = %p\n", __func__, pBuffer, host);
        return -1;
    }
    
    if( u4Len > 1 )
    {
        printk("[%s] [ERR] u4Len = %d\n", __func__, u4Len);
        return -1;
    }
    
    
    mrq.cmd = &cmd;
    host->mrq = &mrq;
    
    cmd.opcode = SD_IO_RW_DIRECT;
	cmd.arg = write ? 0x80000000 : 0x00000000;
	cmd.arg |= u4Func << 28;
	cmd.arg |= u4Addr << 9;
	cmd.arg |= *value;
	cmd.flags = MMC_RSP_SPI_R5 | MMC_RSP_R5 | MMC_CMD_AC;
	memset(cmd.resp, 0, sizeof(cmd.resp));
	cmd.retries = 0;
	cmd.data = NULL;
	
	host->autocmd = 0;
	
    if (msdc_txfifocnt() || msdc_rxfifocnt()) {
        printk("[%s][SD%d] register abnormal,please check!\n",__func__, host->id);
        msdc_reset_hw(host->id);
    }
    
    if((ret = msdc_do_command(host, &cmd, 0, CMD_TIMEOUT)) != 0)
		goto stop;
		
    if(write == 0)
        *value = cmd.resp[0] & 0xFF;

stop:        
    host->mrq = NULL;
    host->data = NULL;
    host->dma_xfer = 0;
    host->blksz = 0;
    return ret;
}

 
unsigned int autok_get_current_vcore_offset(void)
{
    unsigned int vcore_uv = 0;
    unsigned int idx, size;
    unsigned int vcore_sel = 0;

    vcore_uv = mt_vcore_dvfs_volt_get_by_sdio();

    size = sizeof(mt65x2_vcore_tbl)/sizeof(mt65x2_vcore_tbl[0]);

    if (vcore_uv <= mt65x2_vcore_tbl[0]) {
        vcore_sel = 0;
    } else if (vcore_uv >= mt65x2_vcore_tbl[size - 1]) {
        vcore_sel = size - 1;
    } else {		
        for (idx = 0 ; idx < size -1 ; idx ++) {
            if ((vcore_uv >= mt65x2_vcore_tbl[idx]) && 
                (vcore_uv < mt65x2_vcore_tbl[idx+1])) {

                vcore_sel = idx;
                break;
            }
        }
    }

    
    AUTOK_PRINT("[%s] vcore_uv = %duV\r\n", __func__, vcore_uv);
    AUTOK_PRINT("[%s] Current Vcore = %duV(0x%x)\r\n", __func__, mt65x2_vcore_tbl[vcore_sel], vcore_sel);
    
    return vcore_sel;
}

int msdc_autok_read(struct msdc_host *host, unsigned int u4Addr, unsigned int u4Func, void *pBuffer, unsigned int u4Len, unsigned int u4Cmd)
{
    int ret = 0;
    
    if((pBuffer==NULL) || (host==NULL))
    {
        printk("[%s] pBuffer = %p, host = %p\n", __func__, pBuffer, host);
        return -1;
    }
        
    if( ((u4Cmd == CMD_53) && (u4Len < 4)) ||
        ((u4Cmd == CMD_52) && (u4Len > 1)) )
    {
        printk("[%s] u4Cmd = %d, u4Len = %d\n", __func__, u4Cmd, u4Len);
        return -1;
    }
    
    if(u4Cmd == CMD_53)
        ret = autok_io_rw_extended(host, u4Addr, u4Func, pBuffer, u4Len, 0);
    else if(u4Cmd == CMD_52)
        ret = autok_io_rw_direct(host, u4Addr, u4Func, pBuffer, u4Len, 0);
    else
    {
        printk("[%s] Doesn't support u4Cmd = %d\n", __func__, u4Cmd);
        ret = -1;
    }
    
    return ret;
}

int msdc_autok_write(struct msdc_host *host, unsigned int u4Addr, unsigned int u4Func, void *pBuffer, unsigned int u4Len, unsigned int u4Cmd)
{
    int ret = 0;
    
    if((pBuffer==NULL) || (host==NULL))
    {
        printk("[%s] pBuffer = %p, host = %p\n", __func__, pBuffer, host);
        return -1;
    }
        
    if( ((u4Cmd == CMD_53) && (u4Len < 4)) ||
        ((u4Cmd == CMD_52) && (u4Len > 1)) )
    {
        printk("[%s] u4Cmd = %d, u4Len = %d\n", __func__, u4Cmd, u4Len);
        return -1;
    }

    if(u4Cmd == CMD_53)
        ret = autok_io_rw_extended(host, u4Addr, u4Func, pBuffer, u4Len, 1);
    else if(u4Cmd == CMD_52)
        ret = autok_io_rw_direct(host, u4Addr, u4Func, pBuffer, u4Len, 1);
    else
    {
        printk("[%s] Doesn't support u4Cmd = %d\n", __func__, u4Cmd);
        ret = -1;
    }

    return ret;
}

int msdc_autok_adjust_param(struct msdc_host *host, enum AUTOK_PARAM param, u32 *value, int rw)
{
    void __iomem *base = host->base;
    ulong reg = 0;
    u32 field = 0;

    switch (param)
    {
        case CMD_EDGE:
            if((rw == AUTOK_WRITE) && (*value > 1))
            {
                printk("[%s] Input value(%d) for CMD_EDGE is out of range, it should be [0~1]\n", __func__, *value);
                return -1;
            }
            reg = (ulong)(MSDC_IOCON);
            field = (u32)(MSDC_IOCON_RSPL);
            break;
        case RDATA_EDGE:
            if((rw == AUTOK_WRITE) && (*value > 1))
            {
                printk("[%s] Input value(%d) for RDATA_EDGE is out of range, it should be [0~1]\n", __func__, *value);
                return -1;
            }
            reg = (ulong)(MSDC_IOCON);
            field = (u32)(MSDC_IOCON_R_D_SMPL);
            break;
        case WDATA_EDGE:
            if((rw == AUTOK_WRITE) && (*value > 1))
            {
                printk("[%s] Input value(%d) for WDATA_EDGE is out of range, it should be [0~1]\n", __func__, *value);
                return -1;
            }
            reg = (ulong)(MSDC_IOCON);
            field = (u32)(MSDC_IOCON_W_D_SMPL);
            break;
#if 0
        case CLK_DRV:
            if((rw == AUTOK_WRITE) && (*value > 7))
            {
                printk("[%s] Input value(%d) for CLK_DRV is out of range, it should be [0~7]\n", __func__, *value);
                return -1;
            }
                
            if(host->id != 2)
            {
                printk("[%s] MSDC%d doesn't support AUTO K\n", __func__, host->id);
                return -1;
            }

            reg = (ulong)(MSDC2_GPIO_CLK_BASE);
            field = (u32)(GPIO_MSDC1_MSDC2_DRVN);
            break;
        case CMD_DRV:
            if((rw == AUTOK_WRITE) && (*value > 7))
            {
                printk("[%s] Input value(%d) for CMD_DRV is out of range, it should be [0~7]\n", __func__, *value);
                return -1;
            }
            
            if(host->id != 2)
            {
                printk("[%s] MSDC%d doesn't support on AUTO K\n", __func__, host->id);
                return -1;
            }

            reg = (ulong)(MSDC2_GPIO_CMD_BASE);
            field = (u32)(GPIO_MSDC1_MSDC2_DRVN);
            break;
        case DAT_DRV:
            if((rw == AUTOK_WRITE) && (*value > 7))
            {
                printk("[%s] Input value(%d) for DAT_DRV is out of range, it should be [0~7]\n", __func__, *value);
                return -1;
            }
            
            if(host->id != 2)
            {
                printk("[%s] MSDC%d doesn't support on AUTO K\n", __func__, host->id);
                return -1;
            }

            reg = (ulong)(MSDC2_GPIO_DAT_BASE);
            field = (u32)(GPIO_MSDC1_MSDC2_DRVN);
            break;
#endif
        case DAT0_RD_DLY:
            if((rw == AUTOK_WRITE) && (*value > 31))
            {
                printk("[%s] Input value(%d) for DAT0_RD_DLY is out of range, it should be [0~31]\n", __func__, *value);
                return -1;
            }
            
            reg = (ulong)(MSDC_DAT_RDDLY0);
            field = (u32)(MSDC_DAT_RDDLY0_D0);
            break;
        case DAT1_RD_DLY:
            if((rw == AUTOK_WRITE) && (*value > 31))
            {
                printk("[%s] Input value(%d) for DAT1_RD_DLY is out of range, it should be [0~31]\n", __func__, *value);
                return -1;
            }
            
            reg = (ulong)(MSDC_DAT_RDDLY0);
            field = (u32)(MSDC_DAT_RDDLY0_D1);
            break;
        case DAT2_RD_DLY:
            if((rw == AUTOK_WRITE) && (*value > 31))
            {
                printk("[%s] Input value(%d) for DAT2_RD_DLY is out of range, it should be [0~31]\n", __func__, *value);
                return -1;
            }
            
            reg = (ulong)(MSDC_DAT_RDDLY0);
            field = (u32)(MSDC_DAT_RDDLY0_D2);
            break;
        case DAT3_RD_DLY:
            if((rw == AUTOK_WRITE) && (*value > 31))
            {
                printk("[%s] Input value(%d) for DAT3_RD_DLY is out of range, it should be [0~31]\n", __func__, *value);
                return -1;
            }
            
            reg = (ulong)(MSDC_DAT_RDDLY0);
            field = (u32)(MSDC_DAT_RDDLY0_D3);
            break;
        case DAT_WRD_DLY:
            if((rw == AUTOK_WRITE) && (*value > 31))
            {
                printk("[%s] Input value(%d) for DAT_WRD_DLY is out of range, it should be [0~31]\n", __func__, *value);
                return -1;
            }
            
            reg = (ulong)(MSDC_PAD_TUNE);
            field = (u32)(MSDC_PAD_TUNE_DATWRDLY);
            break;
        case DAT_RD_DLY:
            if((rw == AUTOK_WRITE) && (*value > 31))
            {
                printk("[%s] Input value(%d) for DAT_RD_DLY is out of range, it should be [0~31]\n", __func__, *value);
                return -1;
            }
            
            reg = (ulong)(MSDC_PAD_TUNE);
            field = (u32)(MSDC_PAD_TUNE_DATRRDLY);
            break;
        case CMD_RESP_RD_DLY:
            if((rw == AUTOK_WRITE) && (*value > 31))
            {
                printk("[%s] Input value(%d) for CMD_RESP_RD_DLY is out of range, it should be [0~31]\n", __func__, *value);
                return -1;
            }
            
            reg = (ulong)(MSDC_PAD_TUNE);
            field = (u32)(MSDC_PAD_TUNE_CMDRRDLY);
            break;
        case CMD_RD_DLY:
            if((rw == AUTOK_WRITE) && (*value > 31))
            {
                printk("[%s] Input value(%d) for CMD_RD_DLY is out of range, it should be [0~31]\n", __func__, *value);
                return -1;
            }
            
            reg = (ulong)(MSDC_PAD_TUNE);
            field = (u32)(MSDC_PAD_TUNE_CMDRDLY);
            break;
        case DATA_DLYLINE_SEL:
            if((rw == AUTOK_WRITE) && (*value > 1))
            {
                printk("[%s] Input value(%d) for DATA_DLYLINE_SEL is out of range, it should be [0~1]\n", __func__, *value);
                return -1;
            }
            
            reg = (ulong)(MSDC_IOCON);
            field = (u32)(MSDC_IOCON_DDLSEL);
            break;
        case READ_DATA_SMPL_SEL:
            if((rw == AUTOK_WRITE) && (*value > 1))
            {
                printk("[%s] Input value(%d) for READ_DATA_SMPL_SEL is out of range, it should be [0~1]\n", __func__, *value);
                return -1;
            }
            
            reg = (ulong)(MSDC_IOCON);
            field = (u32)(MSDC_IOCON_R_D_SMPL_SEL);
            break;
        case WRITE_DATA_SMPL_SEL:
            if((rw == AUTOK_WRITE) && (*value > 1))
            {
                printk("[%s] Input value(%d) for WRITE_DATA_SMPL_SEL is out of range, it should be [0~1]\n", __func__, *value);
                return -1;
            }
            
            reg = (ulong)(MSDC_IOCON);
            field = (u32)(MSDC_IOCON_W_D_SMPL_SEL);
            break;
        case INT_DAT_LATCH_CK:
            if((rw == AUTOK_WRITE) && (*value > 7))
            {
                printk("[%s] Input value(%d) for INT_DAT_LATCH_CK is out of range, it should be [0~7]\n", __func__, *value);
                return -1;
            }
            
            reg = (ulong)(MSDC_PATCH_BIT0);
            field = (u32)(MSDC_PB0_INT_DAT_LATCH_CK_SEL);
            break;
        case CKGEN_MSDC_DLY_SEL:
            if((rw == AUTOK_WRITE) && (*value > 31))
            {
                printk("[%s] Input value(%d) for CKGEN_MSDC_DLY_SEL is out of range, it should be [0~31]\n", __func__, *value);
                return -1;
            }
            
            reg = (ulong)(MSDC_PATCH_BIT0);
            field = (u32)(MSDC_PB0_CKGEN_MSDC_DLY_SEL);
            break;
        case CMD_RSP_TA_CNTR:
            if((rw == AUTOK_WRITE) && (*value > 7))
            {
                printk("[%s] Input value(%d) for CMD_RSP_TA_CNTR is out of range, it should be [0~7]\n", __func__, *value);
                return -1;
            }
            
            reg = (ulong)(MSDC_PATCH_BIT1);
            field = (u32)(MSDC_PB1_CMD_RSP_TA_CNTR);
            break;
        case WRDAT_CRCS_TA_CNTR:
            if((rw == AUTOK_WRITE) && (*value > 7))
            {
                printk("[%s] Input value(%d) for WRDAT_CRCS_TA_CNTR is out of range, it should be [0~7]\n", __func__, *value);
                return -1;
            }
            
            reg = (ulong)(MSDC_PATCH_BIT1);
            field = (u32)(MSDC_PB1_WRDAT_CRCS_TA_CNTR);
            break;
        case PAD_CLK_TXDLY:
            if((rw == AUTOK_WRITE) && (*value > 31))
            {
                printk("[%s] Input value(%d) for PAD_CLK_TXDLY is out of range, it should be [0~31]\n", __func__, *value);
                return -1;
            }
            
            reg = (ulong)(MSDC_PAD_TUNE);
            field = (u32)(MSDC_PAD_TUNE_CLKTXDLY);
            break;
        default:
            printk("[%s] Value of [enum AUTOK_PARAM param] is wrong\n", __func__);
            return -1;
    }

    if(rw == AUTOK_READ)
        sdr_get_field(reg, field, *value);
    else if(rw == AUTOK_WRITE) {
        sdr_set_field(reg, field, *value);

        if (param == CKGEN_MSDC_DLY_SEL)
            mdelay(1);

    }
    else
    {
        printk("[%s] Value of [int rw] is wrong\n", __func__);
        return -1;
    }

    return 0;
}

static E_RESULT_TYPE errMapping(struct msdc_host *host)
{

    E_RESULT_TYPE res= E_RESULT_PASS;

    switch(host->error)
    {
        case REQ_CMD_EIO:
            res = E_RESULT_CMD_CRC;
            break;
        case REQ_CMD_TMO:
            res = E_RESULT_TO;
            break;
        default:
            res = E_RESULT_ERR;
            break;
    }

    return res;
}

static void containGen(void)
{
    unsigned int i,j;

    unsigned int *pData = g_test_write_pattern;

    for(j=0; j<TUNING_DATA_NO; j++) {
        for(i=0; i<TUNING_TEST_TIME; i++) {
            *pData = tuning_data[j];
            pData++;
        }
    }
}

static E_RESULT_TYPE autok_write_test(struct msdc_host *host)
{
    int i;
    E_RESULT_TYPE res = E_RESULT_PASS;
    unsigned int reg;
    unsigned char *data;

    
    for(i=0; i<TUNING_DATA_NO; i++) {
        data = (unsigned char *)&tuning_data[i];
        if(msdc_autok_write(host, SDIO_IP_WTMDPCR1, LTE_MODEM_FUNC, (void*)data, 1, CMD_52) != 0) {
            res = E_RESULT_CMD_CRC;
            goto end;
        }
        if(msdc_autok_write(host, SDIO_IP_WTMDPCR1+1, LTE_MODEM_FUNC, (void*)(data+1), 1, CMD_52) != 0) {
            res = E_RESULT_CMD_CRC;
            goto end;
        }
        if(msdc_autok_write(host, SDIO_IP_WTMDPCR1+2, LTE_MODEM_FUNC, (void*)(data+2), 1, CMD_52) != 0) {
            res = E_RESULT_CMD_CRC;
            goto end;
        }
        if(msdc_autok_write(host, SDIO_IP_WTMDPCR1+3, LTE_MODEM_FUNC, (void*)(data+3), 1, CMD_52) != 0) {
            res = E_RESULT_CMD_CRC;
            goto end;
        }

        if(msdc_autok_write(host, SDIO_IP_WTMDR, LTE_MODEM_FUNC, (void*)&(g_test_write_pattern[i*TUNING_TEST_TIME]),(4*TUNING_TEST_TIME), CMD_53) != 0) {
            res = errMapping(host);
            goto end;
        }

        data = (unsigned char *)&reg;
        if(msdc_autok_read(host, SDIO_IP_WTMCR, LTE_MODEM_FUNC, (void*)data, 1, CMD_52) != 0) {
            res = E_RESULT_CMD_CRC;
            goto end;
        }
        if(msdc_autok_read(host, SDIO_IP_WTMCR+1, LTE_MODEM_FUNC, (void*)(data+1), 1, CMD_52) != 0) {
            res = E_RESULT_CMD_CRC;
            goto end;
        }
        if(msdc_autok_read(host, SDIO_IP_WTMCR+2, LTE_MODEM_FUNC, (void*)(data+2), 1, CMD_52) != 0) {
            res = E_RESULT_CMD_CRC;
            goto end;
        }
        if(msdc_autok_read(host, SDIO_IP_WTMCR+3, LTE_MODEM_FUNC, (void*)(data+3), 1, CMD_52) != 0) {
            res = E_RESULT_CMD_CRC;
            goto end;
        }

        if((reg & TEST_MODE_STATUS) == TEST_MODE_STATUS) {
            res = E_RESULT_ERR;
            goto end;
        }
    }
end:
    return res;
}

static E_RESULT_TYPE autok_read_test(struct msdc_host *host)
{
    int i;
    E_RESULT_TYPE res = E_RESULT_PASS;
    unsigned char *data;

    
    for(i=0; i<TUNING_DATA_NO; i++) {
        data = (unsigned char *)&tuning_data[i];
        if(msdc_autok_write(host, SDIO_IP_WTMDPCR0, LTE_MODEM_FUNC, (void*)data, 1, CMD_52) != 0) {
            res = E_RESULT_CMD_CRC;
            goto end;
        }
        if(msdc_autok_write(host, SDIO_IP_WTMDPCR0+1, LTE_MODEM_FUNC, (void*)(data+1), 1, CMD_52) != 0) {
            res = E_RESULT_CMD_CRC;
            goto end;
        }
        if(msdc_autok_write(host, SDIO_IP_WTMDPCR0+2, LTE_MODEM_FUNC, (void*)(data+2), 1, CMD_52) != 0) {
            res = E_RESULT_CMD_CRC;
            goto end;
        }
        if(msdc_autok_write(host, SDIO_IP_WTMDPCR0+3, LTE_MODEM_FUNC, (void*)(data+3), 1, CMD_52) != 0) {
            res = E_RESULT_CMD_CRC;
            goto end;
        }

        if(msdc_autok_read(host, SDIO_IP_WTMDR, LTE_MODEM_FUNC, (void*)g_test_read_pattern, (4*TUNING_TEST_TIME), CMD_53) != 0) {
            res = errMapping(host);
            goto end;
        }

        if(memcmp(g_test_read_pattern, &g_test_write_pattern[i*TUNING_TEST_TIME], 4*TUNING_TEST_TIME) != 0) {
            res = E_RESULT_CMP_ERR;
            printk("[%s] E_RESULT_CMP_ERR\n", __func__);
            goto end;
        }
    }
end:
    return res;
}

static E_RESULT_TYPE autok_cmd_test(struct msdc_host *host)
{
    int i;
    E_RESULT_TYPE res = E_RESULT_PASS;
    unsigned char *data;

#if 0
    
    for(i=0; i<TUNING_DATA_NO; i++) {
        data = (unsigned char *)&tuning_data[i];
        if(msdc_autok_write(host, SDIO_IP_WTMDPCR0, LTE_MODEM_FUNC, (void*)data, 1, CMD_52) != 0) {
            res = E_RESULT_CMD_CRC;
            goto end;
        }
        if(msdc_autok_write(host, SDIO_IP_WTMDPCR0+1, LTE_MODEM_FUNC, (void*)(data+1), 1, CMD_52) != 0) {
            res = E_RESULT_CMD_CRC;
            goto end;
        }
        if(msdc_autok_write(host, SDIO_IP_WTMDPCR0+2, LTE_MODEM_FUNC, (void*)(data+2), 1, CMD_52) != 0) {
            res = E_RESULT_CMD_CRC;
            goto end;
        }
        if(msdc_autok_write(host, SDIO_IP_WTMDPCR0+3, LTE_MODEM_FUNC, (void*)(data+3), 1, CMD_52) != 0) {
            res = E_RESULT_CMD_CRC;
            goto end;
        }

        data = (unsigned char *)g_test_read_pattern;
        if(msdc_autok_read(host, SDIO_IP_WTMDPCR0, LTE_MODEM_FUNC, (void*)data, 1, CMD_52) != 0) {
            res = E_RESULT_CMD_CRC;
            goto end;
        }
        if(msdc_autok_read(host, SDIO_IP_WTMDPCR0+1, LTE_MODEM_FUNC, (void*)(data+1), 1, CMD_52) != 0) {
            res = E_RESULT_CMD_CRC;
            goto end;
        }
        if(msdc_autok_read(host, SDIO_IP_WTMDPCR0+2, LTE_MODEM_FUNC, (void*)(data+2), 1, CMD_52) != 0) {
            res = E_RESULT_CMD_CRC;
            goto end;
        }
        if(msdc_autok_read(host, SDIO_IP_WTMDPCR0+3, LTE_MODEM_FUNC, (void*)(data+3), 1, CMD_52) != 0) {
            res = E_RESULT_CMD_CRC;
            goto end;
        }

        if(g_test_read_pattern[0] != tuning_data[i]) {
        #ifdef AUTOK_DEBUG
            printk("write: 0x%x read: 0x%x\r\n", tuning_data[i], g_test_read_pattern[0]);
        #endif
            res = E_RESULT_CMP_ERR;
            goto end;
        }
    }
#else
    
    
    for(i=0; i<TUNING_CMD_NO; i+=4) {
        data = (unsigned char *)&tuning_cmd[i];
        if(msdc_autok_write(host, SDIO_IP_WTMDPCR0, LTE_MODEM_FUNC, (void*)data, 1, CMD_52) != 0) {
            res = E_RESULT_CMD_CRC;
            goto end;
        }
        if(msdc_autok_write(host, SDIO_IP_WTMDPCR0+1, LTE_MODEM_FUNC, (void*)(data+1), 1, CMD_52) != 0) {
            res = E_RESULT_CMD_CRC;
            goto end;
        }
        if(msdc_autok_write(host, SDIO_IP_WTMDPCR0+2, LTE_MODEM_FUNC, (void*)(data+2), 1, CMD_52) != 0) {
            res = E_RESULT_CMD_CRC;
            goto end;
        }
        if(msdc_autok_write(host, SDIO_IP_WTMDPCR0+3, LTE_MODEM_FUNC, (void*)(data+3), 1, CMD_52) != 0) {
            res = E_RESULT_CMD_CRC;
            goto end;
        }
    }
#endif

end:
    return res;
}

static int autok_recovery(struct msdc_host *host)
{
    

#if 0
    MSDC_RESET();
    MSDC_CLR_FIFO();
    MSDC_WRITE32(MSDC_INT, MSDC_READ32(MSDC_INT));
#endif

    return 0;
}

static void autok_select_range(unsigned int  result, unsigned int *sel)
{
    unsigned char start = 0;
    unsigned char end = 0;  
    unsigned char bit = 0;
    unsigned char max_start = 0;
    unsigned char max_end = 0;
    unsigned char max_score = 0;

    
    if (result == 0) {
        start = 0; end = 31;
        goto end;
    }

find:
    start = end = 0;
    while (bit < 32) {
        if (result & (1 << bit)) { 
            bit++; continue;
        }
        start = end = bit;
        bit++;
        break;
    }

    while (bit < 32) {
        if (result & (1 << bit)){ 
            bit++;
            if((end - start) > max_score) {
                max_score = end - start;
                max_start = start;
                max_end = end;
            }
            goto find;

        }
        else {
            end = bit;
            bit++;
        }
    }
end:
    if((end -start) > max_score) {
        max_score = end - start;
        max_start = start;
        max_end = end;
    }

    AUTOK_PRINT("score<%d> choose bit<%d> from<0x%x>\r\n",(max_score+1),(max_end + max_start)/2, result);
    *sel = (max_end + max_start)/2;

}

#if 0
static unsigned int autok_accum_score(unsigned int result)
{
    unsigned int num = 0;

#if 0    
    unsigned int bit = 0;
    
    
    if (0 == result)
        return 32;

    if (0xFFFFFFFF == result)
        return 0;

    
    while (bit < 32) {
        if (result & (1 << bit)) { 
            bit++;
            continue;
        }
        
        bit++;
        num++;
    }
#endif

    return num;
}
#endif

static int autok_simple_score(unsigned int result)
{
    unsigned int bit = 0;
    unsigned int num = 0;
    unsigned int old = 0;

    
    if (0 == result) {
        strcpy(g_tune_result_str,"OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO");
        return 32;
    }

    if (0xFFFFFFFF == result) {
        strcpy(g_tune_result_str,"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
        return 0;
    }

    
    while (bit < 32) {
        if (result & (1 << bit)) { 
            g_tune_result_str[bit]='X';
            bit++;
            if (old < num)
                old = num;
            num = 0;
            continue;
        }
        g_tune_result_str[bit]='O';
        bit++;
        num++;
    }

    if (num > old)
        old = num;

    return old;
}

#ifdef AUTOK_CYC_ALG_0
#ifdef AUTOK_CMD_TUNE_LEGACY
static int 
autok_check_score(
    unsigned int result, 
    unsigned int *pNumOfzero, 
    unsigned int *pFrtPosErr, 
    unsigned int *pPeriod, 
    unsigned int minPadPerCycle)
{
    unsigned int  bit = 0;
    unsigned int  num = 0;
    unsigned int  old = 0;
    unsigned int frstErrS = 0, sndErrS = 0, frstErrE = 0, sndErrE = 0;
    E_AUTOK_PERIOD_STA sta=PERIOD_NONE;


    *pNumOfzero = 0;
    *pFrtPosErr = 0;
    
    if (0 == result) {
        strcpy(g_tune_result_str,"OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO");
        *pNumOfzero = 32;
        *pFrtPosErr = 32;
        return 32;
    }

    if (0xFFFFFFFF == result) {
        strcpy(g_tune_result_str,"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
        return 0;
    }

    
    while (bit < 32) {
        if (result & (1 << bit)) { 
            g_tune_result_str[bit]='X';
            switch(sta)
            {
                case PERIOD_NONE:
                    sta = PERIOD_L_FIRST_POS;
                    *pFrtPosErr = bit;
                    break;
                case PERIOD_L_FIRST_POS:
                    frstErrE = bit;
                    break;
                case PERIOD_L_FIRST_POS_DONE:
                    sta = PERIOD_L_SECOND_POS;
                    sndErrS = bit;
                    break;
                case PERIOD_L_SECOND_POS:
                    sndErrE = bit;
                    break;
                case PERIOD_F_FIRST_POS:
                    sta = PERIOD_F_FIRST_POS_DONE;
                    frstErrS = bit;
                    *pFrtPosErr = bit;
                    break;
                case PERIOD_F_SECOND_POS:
                    sta = PERIOD_DONE_2;
                    sndErrS = bit;
                    break;
                default:
                    break;
            }
            bit++;
            if (num > old)
                old = num;
            num = 0;
            continue;
        }
        g_tune_result_str[bit]='O';
        bit++;
        num++;
        *pNumOfzero=*pNumOfzero+1;
        switch(sta)
        {
            case PERIOD_NONE:
                sta = PERIOD_F_FIRST_POS;
                break;
            case PERIOD_F_FIRST_POS_DONE:
                sta = PERIOD_F_SECOND_POS;
                break;
            case PERIOD_L_FIRST_POS:
                sta = PERIOD_L_FIRST_POS_DONE;
                frstErrE = bit - 1;
                break;
            case PERIOD_L_SECOND_POS:
                sndErrE = bit - 1;
                sta = PERIOD_DONE;
                break;
            default:
                break;
        }

    }

    if (num > old)
        old = num;

    if (sta == PERIOD_DONE)
        *pPeriod = sndErrE - frstErrE;
    else if (sta == PERIOD_DONE_2)
        *pPeriod = sndErrS - frstErrS;
    else
        *pPeriod = 0;

    if ((*pPeriod < minPadPerCycle) || (*pPeriod > 32))
         *pPeriod = 0;   
    
    return old;
}
#endif
#endif

static void 
autok_check_rawd_style(
    P_AUTOK_RAWD_SCAN_T prAutok_raw_scan,
    unsigned char isRDat
    )
{
    unsigned int bit;
    unsigned char fInvalidCKGEN = 0;
    unsigned int filter = 2;
    AUTOK_RAWD_SCAN_STA_E RawScanSta = RD_SCAN_NONE;
    
    for (bit = 0; bit < 32; bit++) {
        if (prAutok_raw_scan->RawData & (1 << bit)) {
            switch (RawScanSta) {
                case RD_SCAN_NONE:
                    RawScanSta = RD_SCAN_PAD_BOUND_S;
                    prAutok_raw_scan->BoundReg1_S = 0;
                    prAutok_raw_scan->Reg1Cnt++;
                break;

                case RD_SCAN_PAD_MARGIN:
                    RawScanSta = RD_SCAN_PAD_BOUND_S;
                    prAutok_raw_scan->BoundReg1_S = bit;
                    prAutok_raw_scan->Reg1Cnt++;
                break;

                case RD_SCAN_PAD_BOUND_E:
                    if (filter) {
                        if (
                            ((bit - prAutok_raw_scan->BoundReg1_E) <= AUTOK_TUNING_INACCURACY)) {

                            AUTOK_PRINT("[WARN] Try to filter the holes on raw data when CKGEN=%d\r\n", 
                                prAutok_raw_scan->CurCKGEN);
                                
                            RawScanSta = RD_SCAN_PAD_BOUND_S;

                            prAutok_raw_scan->Reg1Cnt += 2;
                            prAutok_raw_scan->BoundReg1_E = 0;
                            prAutok_raw_scan->BoundReg2_S = 0;

                            filter--;
                        }
                        else {
                            RawScanSta = RD_SCAN_PAD_BOUND_S_2;
                            prAutok_raw_scan->BoundReg2_S = bit;
                            prAutok_raw_scan->Reg2Cnt++;
                        }
                    }
                    else {
                        RawScanSta = RD_SCAN_PAD_BOUND_S_2;
                        prAutok_raw_scan->BoundReg2_S = bit;
                        prAutok_raw_scan->Reg2Cnt++;
                    }
                break;

                
                case RD_SCAN_PAD_BOUND_E_2:
                    if (filter) {
                        filter--;
                        RawScanSta = RD_SCAN_PAD_BOUND_S_2;

                        
                        if ((bit - prAutok_raw_scan->BoundReg2_E) <= 2) {
                            if ((bit - prAutok_raw_scan->BoundReg2_E) >= 1)
                                prAutok_raw_scan->Reg2Cnt += (bit - prAutok_raw_scan->BoundReg2_E);

                            prAutok_raw_scan->BoundReg2_E = 0;
                        }
                        
                        else if ((prAutok_raw_scan->BoundReg2_S - prAutok_raw_scan->BoundReg1_E) <= 2){
                            
                            if ((prAutok_raw_scan->BoundReg2_S - prAutok_raw_scan->BoundReg1_E) >= 1)
                                prAutok_raw_scan->Reg1Cnt += 
                                    (prAutok_raw_scan->BoundReg2_E - prAutok_raw_scan->BoundReg1_E);

                            
                            prAutok_raw_scan->BoundReg1_E = prAutok_raw_scan->BoundReg2_E;

                            
                            prAutok_raw_scan->BoundReg2_S = bit;
                            prAutok_raw_scan->BoundReg2_E = 0;
                            prAutok_raw_scan->Reg2Cnt = 1;
                        }
                        else {
                            AUTOK_PRINT("[ERR] Find holes on raw data when CKGEN=%d, but can NOT filter! \r\n", 
                                prAutok_raw_scan->CurCKGEN);

                            fInvalidCKGEN = 1;
                            goto exit;
                        }

                        AUTOK_PRINT("[WARN] Try to filter the holes on raw data when CKGEN=%d\r\n", 
                            prAutok_raw_scan->CurCKGEN);
                    }
                    else {
                        AUTOK_PRINT("[WARN] Find too much fail regions when CKGEN=%d(Invalid)\r\n", 
                            prAutok_raw_scan->CurCKGEN);

                        fInvalidCKGEN = 1;
                        goto exit;
                    }
                break;

                case RD_SCAN_PAD_BOUND_S:
                    prAutok_raw_scan->Reg1Cnt++;
                break;
                
                case RD_SCAN_PAD_BOUND_S_2:
                    prAutok_raw_scan->Reg2Cnt++;
                break;
                
                default:
                break;
            }
        }
        else {
            switch (RawScanSta) {
                case RD_SCAN_NONE:
                    RawScanSta = RD_SCAN_PAD_MARGIN;
                break;

                case RD_SCAN_PAD_BOUND_S:
                    RawScanSta = RD_SCAN_PAD_BOUND_E;
                    prAutok_raw_scan->BoundReg1_E = bit - 1;
                break;

                case RD_SCAN_PAD_BOUND_S_2:
                    RawScanSta = RD_SCAN_PAD_BOUND_E_2;
                    prAutok_raw_scan->BoundReg2_E = bit - 1;
                break;

                case RD_SCAN_PAD_MARGIN:
                case RD_SCAN_PAD_BOUND_E:
                case RD_SCAN_PAD_BOUND_E_2:
                default:
                break;
            }
        }
    }

    if (isRDat) {

        if (prAutok_raw_scan->Reg1Cnt == 32) {
            fInvalidCKGEN = 1;
            goto exit;
        }

        if ((32 - (prAutok_raw_scan->Reg1Cnt + prAutok_raw_scan->Reg2Cnt)) <= 
            (AUTOK_TUNING_INACCURACY+1))
            fInvalidCKGEN = 1;
    }
    
exit:
    if (fInvalidCKGEN) {
        prAutok_raw_scan->fInvalidCKGEN = 1;
    }
}

#ifdef AUTOK_CYC_ALG_0
static AUTOK_CMDPAT_COMP_E 
autok_comp_cmd_pat(
    P_AUTOK_CMDPAT_DOUBLE_CHK_T prCMDPatChk, unsigned int fDoubleChk,
    int *diff, unsigned int fChkPos)
{
    AUTOK_RAWD_SCAN_T CMDPadScan0, CMDPadScan1;
    unsigned int FstMidErrPos0, FstMidErrPos1, SndMidErrPos0, SndMidErrPos1;
    unsigned int ck0, ck1, raw0, raw1;
    AUTOK_CMDPAT_COMP_E fIdent = CMDPAT_NONE;

    memset(&CMDPadScan0, 0, sizeof(CMDPadScan0));
    memset(&CMDPadScan1, 0, sizeof(CMDPadScan1));

    ck0 = prCMDPatChk->ck_s_b4;
    ck1 = prCMDPatChk->ck_e_b4;
    
    if (fDoubleChk) {
        raw0 = prCMDPatChk->raw_s_sh;
        raw1 = prCMDPatChk->raw_e_sh;
    }
    else {
        raw0 = prCMDPatChk->raw_s_b4;
        raw1 = prCMDPatChk->raw_e_b4;
    }

    CMDPadScan0.CurCKGEN = ck0;
    CMDPadScan0.RawData = raw0;
    autok_check_rawd_style(&CMDPadScan0, 0);
    FstMidErrPos0 = CMDPadScan0.BoundReg1_S + CMDPadScan0.Reg1Cnt/2;

    CMDPadScan1.CurCKGEN = ck1;
    CMDPadScan1.RawData = raw1;
    autok_check_rawd_style(&CMDPadScan1, 0);
    FstMidErrPos1 = CMDPadScan1.BoundReg1_S + CMDPadScan1.Reg1Cnt/2;

    
    *diff = (int)(FstMidErrPos1 - FstMidErrPos0);

CommonChk:
    
    if (!fDoubleChk) {

        
        if (fChkPos)
            if(ABS_DIFF(FstMidErrPos0, FstMidErrPos1) > (AUTOK_TUNING_INACCURACY+1))
                 goto exit;

        
        
        if (CMDPadScan0.Reg1Cnt && CMDPadScan0.Reg2Cnt &&
            CMDPadScan1.Reg1Cnt && CMDPadScan1.Reg2Cnt) {

            SndMidErrPos0 = 
                CMDPadScan0.BoundReg2_S + CMDPadScan0.Reg2Cnt/2;
                
            SndMidErrPos1 = 
                CMDPadScan1.BoundReg2_S + CMDPadScan1.Reg2Cnt/2;

            AUTOK_PRINT("CKGEN[%d].MidErrPos=%d, CKGEN[%d].MidErrPos=%d, Reg_Gap=%d\r\n", \
                    ck0, FstMidErrPos0, ck1, FstMidErrPos1, 
                    ABS_DIFF((SndMidErrPos0 - FstMidErrPos0), (SndMidErrPos1 - FstMidErrPos1)));

            
            #if 0
            if (3 < ABS_DIFF((SndMidErrPos0 - FstMidErrPos0), (SndMidErrPos1 - FstMidErrPos1)))
                goto exit;
            #endif
        }
        
        else {
            if ((CMDPadScan1.Reg2Cnt && !CMDPadScan0.Reg2Cnt) ||
                (!CMDPadScan1.Reg2Cnt && CMDPadScan0.Reg2Cnt)) {

                
                if (fChkPos) {
                    if (CMDPadScan1.Reg2Cnt) {
                        #if 0
                        if ((CMDPadScan1.BoundReg2_S - CMDPadScan1.BoundReg1_E) <= 
                            (AUTOK_TUNING_INACCURACY + 2))
                            fIdent = CMDPAT_HALF_IDENTICAL;
                        else
                            fIdent = CMDPAT_NONE;
                        #else

                        
                        fIdent = CMDPAT_HALF_IDENTICAL;
                        
                        #endif
                    }

                    if (CMDPadScan0.Reg2Cnt) {
                        #if 0
                        if ((CMDPadScan0.BoundReg2_S - CMDPadScan0.BoundReg1_E) <= 
                            (AUTOK_TUNING_INACCURACY + 2))
                            fIdent = CMDPAT_HALF_IDENTICAL;
                        else
                            fIdent = CMDPAT_NONE;
                        #else
                        
                        
                        fIdent = CMDPAT_HALF_IDENTICAL;
                        
                        #endif
                    }
                }
                    
                goto exit;
            }
        }
    }
    
    else {
        switch (prCMDPatChk->scen) {
            case CMDPAT_REG_1_L:
                if (ABS_DIFF(FstMidErrPos0, FstMidErrPos1) > (AUTOK_TUNING_INACCURACY+1)) {

                    
                    if (ABS_DIFF(FstMidErrPos0, FstMidErrPos1) < 25)
                        goto exit;
                    else {
                        AUTOK_PRINT("ck0 roll back from right!\r\n");

                        
                        *diff = (int)(prCMDPatChk->diff_b4);
                    }
                }
                
            break;

            case CMDPAT_REG_2_L:
                if (ABS_DIFF(FstMidErrPos0, FstMidErrPos1) > (AUTOK_TUNING_INACCURACY+1)) {

                    
                    if (CMDPadScan0.Reg1Cnt && !CMDPadScan0.Reg2Cnt && 
                        CMDPadScan1.Reg1Cnt && CMDPadScan1.Reg2Cnt &&
                        (CMDPadScan0.BoundReg1_S > AUTOK_CMDPAT_CHK_SHIFT)) {

                        SndMidErrPos1 = 
                            CMDPadScan1.BoundReg2_S + CMDPadScan1.Reg2Cnt/2;

                        if (ABS_DIFF(FstMidErrPos0, SndMidErrPos1) > 
                            (AUTOK_TUNING_INACCURACY+1))
                            goto exit;
                        else {
                            AUTOK_PRINT("Int. boundary of ck0 shift out, ck1 stay here!\r\n");
                            *diff = (int)(SndMidErrPos1 - FstMidErrPos0);
                        }
                    }
                    
                    else if (CMDPadScan0.Reg1Cnt && !CMDPadScan0.Reg2Cnt && 
                             CMDPadScan1.Reg1Cnt && !CMDPadScan1.Reg2Cnt)
                        goto exit;
                    
                }
                
            break;

            default:
                fDoubleChk = 0;
                goto CommonChk;

            break;
        }
    }

    fIdent = CMDPAT_IDENTICAL;

exit:
    
    if (fIdent != CMDPAT_NONE) {
        AUTOK_PRINT("CKGEN[%d].MidErrPos=%d, CKGEN[%d].MidErrPos=%d, Diff=%d\r\n", \
                ck0, FstMidErrPos0, ck1, FstMidErrPos1, *diff);

        
        if (!fDoubleChk) {
            
            prCMDPatChk->cmd_int_shift = AUTOK_CMDPAT_CHK_SHIFT;
            
            
            if (CMDPadScan0.Reg1Cnt && !CMDPadScan0.Reg2Cnt &&
                CMDPadScan1.Reg1Cnt && !CMDPadScan1.Reg2Cnt) {

                if (CMDPadScan0.BoundReg1_S < AUTOK_CMDPAT_CHK_SHIFT) {
                    
                    prCMDPatChk->cmd_int_shift = CMDPadScan1.BoundReg1_E + 1;
                    prCMDPatChk->scen = CMDPAT_REG_1_L;

                    AUTOK_PRINT("1 Fail Region, Near left boundary\r\n");
                }
            }

            
            if (CMDPadScan0.Reg1Cnt && CMDPadScan0.Reg2Cnt) {

                if (CMDPadScan0.BoundReg1_S < AUTOK_CMDPAT_CHK_SHIFT) {
                    AUTOK_PRINT("2 Fail Regions, Near left boundary\r\n");

                    prCMDPatChk->scen = CMDPAT_REG_2_L;

                    
                    if ((CMDPadScan0.BoundReg2_S - CMDPadScan0.BoundReg1_E) <= 
                        (AUTOK_CMDPAT_CHK_SHIFT+1))
                        prCMDPatChk->cmd_int_shift = CMDPadScan0.BoundReg2_E;
                }
                
                else if ((CMDPadScan0.BoundReg2_S - CMDPadScan0.BoundReg1_E) <= 
                    (AUTOK_CMDPAT_CHK_SHIFT+1)) {

                    AUTOK_PRINT("2 Fail Regions, nearby each other\r\n");
                    prCMDPatChk->cmd_int_shift = CMDPadScan0.BoundReg2_S - 
                        CMDPadScan0.BoundReg1_S;
                }    
            }
        }
    }

    return fIdent;
}
#endif

#if !defined(AUTOK_CMD_TUNE_LEGACY)
static void
autok_check_cmd_matrix (
    unsigned int *pMatrixRaw,
    unsigned int PadDlyNum,
    unsigned int IntDlyNum,
    P_AUTOK_CMDMAT_CHAR_POS prMatChar)
{
    unsigned int pad_idx, int_idx;
    unsigned int PadDlyScore, raw;
    AUTOK_RAWD_SCAN_T raw_scan;

    raw = 0;

    for (pad_idx = 0; pad_idx < PadDlyNum; pad_idx++) {
        PadDlyScore = 1;
        
        for (int_idx = 0; int_idx < IntDlyNum; int_idx++) {
            PadDlyScore &= ((pMatrixRaw[int_idx] >> pad_idx) & 0x1);
            if (!PadDlyScore)
                break;
        }

        if (PadDlyScore)
            raw |= 1 << pad_idx;
    }

    memset(&raw_scan, 0, sizeof(raw_scan));
    raw_scan.RawData = raw;
    autok_simple_score(raw_scan.RawData);
    AUTOK_PRINT("CMD pad mapped from martrix: %s\r\n", g_tune_result_str);
    
    autok_check_rawd_style(&raw_scan, 1);

    if (raw_scan.Reg1Cnt) {
        prMatChar->pad_trans_cnt1 = raw_scan.Reg1Cnt;
        prMatChar->pad_trans_s1 = raw_scan.BoundReg1_S;
        prMatChar->pad_trans_e1 = raw_scan.BoundReg1_E;
        prMatChar->pad_trans_m1 = raw_scan.BoundReg1_S + raw_scan.Reg1Cnt/2;
        AUTOK_PRINT("Find 1st Pad transition boundary: [%d, %d], Mid=%d\r\n", 
            prMatChar->pad_trans_s1, prMatChar->pad_trans_e1, prMatChar->pad_trans_m1);
    }

    if (raw_scan.Reg2Cnt) {
        prMatChar->pad_trans_cnt2 = raw_scan.Reg2Cnt;
        prMatChar->pad_trans_s2 = raw_scan.BoundReg2_S;
        prMatChar->pad_trans_e2 = raw_scan.BoundReg2_E;
        prMatChar->pad_trans_m2 = raw_scan.BoundReg2_S + raw_scan.Reg2Cnt/2;
        AUTOK_PRINT("Find 2nd Pad transition boundary: [%d, %d], Mid=%d\r\n", 
            prMatChar->pad_trans_s2, prMatChar->pad_trans_e2, prMatChar->pad_trans_m2);
    }

    if (!raw_scan.Reg1Cnt && !raw_scan.Reg2Cnt)
        AUTOK_PRINT("Can NOT find pad transition boundary!\r\n");

    return;
}
#endif

#if defined(AUTOK_CYC_ALG_2)

typedef enum
{
    AUTOK_CYC_SCAN_INIT = 0,
    AUTOK_CYC_SCAN_CHNG_EDGE,
    AUTOK_CYC_SCAN_CHNG_CKGEN,

    AUTOK_CYC_SCAN_STA_MAX,
}AUTOK_CYC_SCAN_STA_E;

#if defined(MT6582LTE)
#define AUTOK_TRANS_BOUND_RISING_TH     6   
#define AUTOK_TRANS_BOUND_FALLING_TH    6   
#elif defined(MT6592LTE)
#define AUTOK_TRANS_BOUND_RISING_TH     2   
#define AUTOK_TRANS_BOUND_FALLING_TH    2   
#elif defined(MT6595WIFI)
#define AUTOK_TRANS_BOUND_RISING_TH     3   
#define AUTOK_TRANS_BOUND_FALLING_TH    3   
#elif defined(MT6752WIFI)
#define AUTOK_TRANS_BOUND_RISING_TH     3   
#define AUTOK_TRANS_BOUND_FALLING_TH    3   
#endif

#define AUTOK_TRANS_BOUND_RISING    autok_trans_bound_rising
#define AUTOK_TRANS_BOUND_FALLING   autok_trans_bound_falling

static int autok_trans_bound_rising = AUTOK_TRANS_BOUND_RISING_TH;
static int autok_trans_bound_falling = AUTOK_TRANS_BOUND_FALLING_TH;

#define autok_calc_cycle(cyc, char_s, char_e, s_num, e_num, isTotalT, isSRising)     \
    if (isTotalT)   \
    {   \
          \
        if(char_s.pad_trans_s##s_num && char_e.pad_trans_e##e_num)   \
            cyc = ABS_DIFF(char_e.pad_trans_m##e_num, char_s.pad_trans_m##s_num);   \
                \
        else if(!char_s.pad_trans_s##s_num && char_e.pad_trans_e##e_num)   \
            cyc = ABS_DIFF(char_e.pad_trans_e##e_num, char_s.pad_trans_e##s_num);   \
                \
        else if(char_s.pad_trans_s##s_num && !char_e.pad_trans_e##e_num)    \
            cyc = ABS_DIFF(char_e.pad_trans_s##e_num, char_s.pad_trans_s##s_num);   \
          \
        else {      \
            if (isSRising)  \
                cyc = ABS_DIFF(char_s.pad_trans_e##s_num,   \
                    (char_e.pad_trans_s##e_num + AUTOK_TRANS_BOUND_RISING - 1)); \
            else            \
                cyc = ABS_DIFF(char_s.pad_trans_e##s_num,   \
                    (char_e.pad_trans_s##e_num + AUTOK_TRANS_BOUND_FALLING - 1)); \
        }           \
    }   \
    else    \
    {   \
          \
        if(char_s.pad_trans_s##s_num && char_e.pad_trans_e##e_num)        \
            cyc = ABS_DIFF(char_e.pad_trans_m##e_num, char_s.pad_trans_m##s_num);   \
        else if(!char_s.pad_trans_s##s_num && char_e.pad_trans_e##e_num) {  \
                \
            if (isSRising) {    \
                if (char_e.pad_trans_cnt##e_num < AUTOK_TRANS_BOUND_FALLING_TH) \
                    cyc = ABS_DIFF(char_e.pad_trans_s##e_num, \
                        (char_s.pad_trans_e##s_num - AUTOK_TRANS_BOUND_RISING + 1)); \
                else    \
                    cyc = ABS_DIFF(char_e.pad_trans_e##e_num,   \
                        char_s.pad_trans_e##s_num);    \
            }   \
            else {              \
                if (char_e.pad_trans_cnt##e_num < AUTOK_TRANS_BOUND_RISING_TH)  \
                    cyc = ABS_DIFF(char_e.pad_trans_s##e_num,  \
                        (char_s.pad_trans_e##s_num - AUTOK_TRANS_BOUND_FALLING + 1)); \
                else    \
                    cyc = ABS_DIFF(char_e.pad_trans_e##e_num,   \
                        char_s.pad_trans_e##s_num);   \
            }   \
        }   \
        else if(char_s.pad_trans_s##s_num && !char_e.pad_trans_e##e_num) {    \
                \
            if (isSRising)  \
                cyc = ABS_DIFF(char_e.pad_trans_s##e_num, \
                    char_s.pad_trans_s##s_num);   \
            else            \
                cyc = ABS_DIFF(char_e.pad_trans_s##e_num,  \
                    char_s.pad_trans_s##s_num);    \
        }   \
          \
        else {   \
            if (isSRising)  \
                cyc = ABS_DIFF(char_e.pad_trans_s##e_num, \
                    (char_s.pad_trans_e##s_num - AUTOK_TRANS_BOUND_RISING + 1)); \
            else            \
                cyc = ABS_DIFF(char_e.pad_trans_s##e_num,  \
                    (char_s.pad_trans_e##s_num - AUTOK_TRANS_BOUND_FALLING + 1)); \
        }   \
    }

static AUTOK_CYC_SCAN_RES_T
autok_cycle_scan(struct msdc_host *host, U_AUTOK_INTERFACE_DATA *pAutoKData)
{
    AUTOK_CYC_SCAN_RES_T rPadDlyRes;
    AUTOK_CMDMAT_CHAR_POS CMDMatChar[AUTOK_CYC_SCAN_STA_MAX];
    AUTOK_CYC_SCAN_STA_E CycScanSta = AUTOK_CYC_SCAN_INIT;
    AUTOK_RAWD_SCAN_T rRawScan;
    E_RESULT_TYPE res;
    S_AUTOK_CMD_DLY data;
    
    int pad_delay_period_cycle = 0;
    unsigned int k, x, m, n, cnt, ck_sel = 0, edge_sel, sel;
    unsigned int fCMDEdgeDefault = 1;
    unsigned int CMDMatRaw[SCALE_CMDMAT_RSP_DLY_SEL];
    unsigned int RegCnt = 0, Start[2], End[2];
    unsigned int pad_shift;

    unsigned int fRdatPatFound = 0, fStopRDAT = 0, reTuneCmd, reTuneCmdCnt = 0;

    
    memset(&rPadDlyRes, 0, sizeof(rPadDlyRes));
    memset(CMDMatChar, 0, sizeof(CMDMatChar));
    memset(autok_rdata_scan, 0, sizeof(autok_rdata_scan));
    memset(autok_cmd_cmdrrdly, 0, sizeof(autok_cmd_cmdrrdly));
    memset(autok_ckg_data, 0, sizeof(autok_ckg_data));

    
    if (freq_mhz >= 200)
        autok_paddly_per_cyc_eval = 64;
    else if (freq_mhz >= 150)
        autok_paddly_per_cyc_eval = 64;
    else if (freq_mhz >= 100)
        autok_paddly_per_cyc_eval = 90;
    else if (freq_mhz >= 50)
        autok_paddly_per_cyc_eval = 180;

    AUTOK_PRINT("Estimated pad delay per cycle:%d, cur. freq:%dMHz\r\n",
        autok_paddly_per_cyc_eval, freq_mhz);
    
ReTuneMatrix:
    memset(CMDMatRaw, 0, sizeof(CMDMatRaw));

    
    msdc_autok_adjust_param(host, CKGEN_MSDC_DLY_SEL, &ck_sel, MSDC_WRITE);
    if (fCMDEdgeDefault)
        edge_sel = AUTOK_CMD_EDGE_MATRIX_SCAN & 0x01;
    else
        edge_sel = ~AUTOK_CMD_EDGE_MATRIX_SCAN & 0x01;
    msdc_autok_adjust_param(host, CMD_EDGE, &edge_sel, MSDC_WRITE);

    AUTOK_PRINT("Start to scan CMD matrix(%s edge)...\r\n", 
        edge_sel ? "falling" : "rising");

    
    AUTOK_PRINT("CKGEN_MSDC_DLY \t PAD_TUNE_CMDRRDLY \t PAD_TUNE_CMDRDLY \r\n");
    for (x=0; x<SCALE_CMDMAT_RSP_DLY_SEL; x++) {
        msdc_autok_adjust_param(host, CMD_RESP_RD_DLY, &x, MSDC_WRITE);

        if (x == 0) {
            for (m = 0; m < SCALE_PAD_TUNE_CMDRDLY; m++) {
                msdc_autok_adjust_param(host, CMD_RD_DLY, &m, MSDC_WRITE);
                for (cnt = 0; cnt < AUTOK_CMD_TIMES; cnt++) {
                    if (autok_cmd_test(host) != E_RESULT_PASS) {
                        
                        CMDMatRaw[x] |= (1 << m);
                        break;
                    }
                }
            }

            
            RegCnt = 0;
            memset(&rRawScan, 0, sizeof(rRawScan));
            rRawScan.RawData = CMDMatRaw[x];
            autok_check_rawd_style(&rRawScan, 0);

            if (rRawScan.Reg1Cnt && rRawScan.BoundReg1_S && !rRawScan.BoundReg1_E)
                rRawScan.BoundReg1_E = 31;
            else if (rRawScan.Reg1Cnt && rRawScan.Reg2Cnt && !rRawScan.BoundReg2_E)
                rRawScan.BoundReg2_E = 31;

            if (rRawScan.Reg1Cnt) {
                Start[RegCnt] = rRawScan.BoundReg1_S ? 
                    (rRawScan.BoundReg1_S - 1) : rRawScan.BoundReg1_S;
                End[RegCnt] = (rRawScan.BoundReg1_E >= 31) ?
                    rRawScan.BoundReg1_E : (rRawScan.BoundReg1_E + 1);

                RegCnt++;
            }

            if (rRawScan.Reg2Cnt) {
                Start[RegCnt] = rRawScan.BoundReg2_S ? 
                    (rRawScan.BoundReg2_S - 1) : rRawScan.BoundReg2_S;

                if (rRawScan.fInvalidCKGEN)
                    End[RegCnt] = 31;
                else
                    End[RegCnt] = (rRawScan.BoundReg2_E >= 31) ?
                        rRawScan.BoundReg2_E : (rRawScan.BoundReg2_E + 1);

                RegCnt++;
            }
            
            AUTOK_PRINT("At INIT scan, RegCnt:%d, S1:%d, E1:%d, S2:%d, E2:%d, Invalid:%s\r\n",
                RegCnt, Start[0], End[0], Start[1], End[1],
                rRawScan.fInvalidCKGEN ? "YES" : "NO");
            
            
            if (!rRawScan.Reg1Cnt && !rRawScan.Reg2Cnt) {
                AUTOK_PRINT("Can NOT find transition boundary at INIT, quit the loop!\r\n");
                break;
            }
        } else {

            for (n = 0; n < RegCnt; n++) {
                for (m = Start[n]; m <= End[n]; m++) {
                    msdc_autok_adjust_param(host, CMD_RD_DLY, &m, MSDC_WRITE);
                    for (cnt = 0; cnt < AUTOK_CMD_TIMES; cnt++) {
                        if (autok_cmd_test(host) != E_RESULT_PASS) {
                            
                            CMDMatRaw[x] |= (1 << m);
                            break;
                        }
                    }
                }
            }
        }
        
        autok_simple_score(CMDMatRaw[x]);
        AUTOK_PRINT("%02d \t %02d \t %s\r\n", ck_sel, x, g_tune_result_str);
    }

    
    autok_check_cmd_matrix(CMDMatRaw, SCALE_PAD_TUNE_CMDRDLY, 
        SCALE_CMDMAT_RSP_DLY_SEL, &CMDMatChar[CycScanSta]);

    
    if (CMDMatChar[CycScanSta].pad_trans_cnt1 && CMDMatChar[CycScanSta].pad_trans_cnt2) {
        
        if (fCMDEdgeDefault) {
            autok_calc_cycle(pad_delay_period_cycle, CMDMatChar[CycScanSta], 
                CMDMatChar[CycScanSta], 1, 2, 1, AUTOK_CMD_EDGE_MATRIX_SCAN ? 0 : 1);
        }
        else {
            autok_calc_cycle(pad_delay_period_cycle, CMDMatChar[CycScanSta], 
                CMDMatChar[CycScanSta], 1, 2, 1, AUTOK_CMD_EDGE_MATRIX_SCAN ? 1 : 0);
        }
    }
    else {

        switch (CycScanSta) {
            case AUTOK_CYC_SCAN_INIT:
                
                if (fCMDEdgeDefault)
                    fCMDEdgeDefault = 0;

                
                if (CMDMatChar[CycScanSta].pad_trans_cnt1 &&
                    CMDMatChar[CycScanSta].pad_trans_s1 &&
                    CMDMatChar[CycScanSta].pad_trans_e1) {

                    if (AUTOK_CMD_EDGE_MATRIX_SCAN)
                        autok_trans_bound_falling = 
                            CMDMatChar[CycScanSta].pad_trans_cnt1;
                    else
                        autok_trans_bound_rising = 
                            CMDMatChar[CycScanSta].pad_trans_cnt1;

                    AUTOK_PRINT("Update transition boundary len:%d(%s edge)\r\n", 
                        (AUTOK_CMD_EDGE_MATRIX_SCAN ? 
                         autok_trans_bound_falling : autok_trans_bound_rising),
                        (AUTOK_CMD_EDGE_MATRIX_SCAN ? "falling" : "rising"));
                }
                
                if (CMDMatChar[CycScanSta].pad_trans_cnt1) {
                    
                    if (!AUTOK_CMD_EDGE_MATRIX_SCAN) {
                        rPadDlyRes.FBoundMidRefCMD = CMDMatChar[CycScanSta].pad_trans_m1;
                        rPadDlyRes.FBoundCKRefCMD = ck_sel;
                        rPadDlyRes.fFBoundRefCMD = 1;
                        rPadDlyRes.FBoundCntRefCMD = autok_trans_bound_rising * 2;
                    }
                }
                
                CycScanSta = AUTOK_CYC_SCAN_CHNG_EDGE;
                goto ReTuneMatrix;
            break;

            case AUTOK_CYC_SCAN_CHNG_EDGE:
                
                if (CMDMatChar[CycScanSta].pad_trans_cnt1 &&
                    !CMDMatChar[CycScanSta].pad_trans_cnt2) {

                    
                    if (CMDMatChar[CycScanSta].pad_trans_cnt1 &&
                        CMDMatChar[CycScanSta].pad_trans_s1 &&
                        CMDMatChar[CycScanSta].pad_trans_e1) {

                        if (AUTOK_CMD_EDGE_MATRIX_SCAN)
                            autok_trans_bound_rising = 
                                CMDMatChar[CycScanSta].pad_trans_cnt1;
                        else
                            autok_trans_bound_falling = 
                                CMDMatChar[CycScanSta].pad_trans_cnt1;

                        AUTOK_PRINT("Update transition boundary len:%d(%s edge)\r\n", 
                            (AUTOK_CMD_EDGE_MATRIX_SCAN ? 
                             autok_trans_bound_rising : autok_trans_bound_falling),
                            (AUTOK_CMD_EDGE_MATRIX_SCAN ? "rising" : "falling"));
                    }

                    
                    if (AUTOK_CMD_EDGE_MATRIX_SCAN) {
                        rPadDlyRes.FBoundMidRefCMD = CMDMatChar[CycScanSta].pad_trans_m1;
                        rPadDlyRes.FBoundCKRefCMD = ck_sel;
                        rPadDlyRes.fFBoundRefCMD = 1;
                        rPadDlyRes.FBoundCntRefCMD = autok_trans_bound_rising * 2;
                    }

                    
                    if (CMDMatChar[CycScanSta-1].pad_trans_cnt1 && 
                        !CMDMatChar[CycScanSta-1].pad_trans_cnt2) {
                        
                        if (CMDMatChar[CycScanSta-1].pad_trans_s1 > 
                            CMDMatChar[CycScanSta].pad_trans_s1) {
                            autok_calc_cycle(pad_delay_period_cycle, 
                                             CMDMatChar[CycScanSta],
                                             CMDMatChar[CycScanSta-1], 1, 1, 0,
                                             AUTOK_CMD_EDGE_MATRIX_SCAN ? 1 : 0);
                        }
                        else {
                            autok_calc_cycle(pad_delay_period_cycle, 
                                             CMDMatChar[CycScanSta-1],
                                             CMDMatChar[CycScanSta], 1, 1, 0,
                                             AUTOK_CMD_EDGE_MATRIX_SCAN ? 0 : 1);
                        }

                        pad_delay_period_cycle *= 2;
                    }
                    
                    if (!CMDMatChar[CycScanSta-1].pad_trans_cnt1 && 
                        !CMDMatChar[CycScanSta-1].pad_trans_cnt2) {

                        
                        if (CMDMatChar[CycScanSta].pad_trans_s1 && 
                            !CMDMatChar[CycScanSta].pad_trans_e1)
                            CMDMatChar[CycScanSta].pad_trans_m1 = 
                                CMDMatChar[CycScanSta].pad_trans_s1 + 
                                    (AUTOK_CMD_EDGE_MATRIX_SCAN ? 
                                     (AUTOK_TRANS_BOUND_RISING/2) : (AUTOK_TRANS_BOUND_FALLING/2));

                        ck_sel = CMDMatChar[CycScanSta].pad_trans_m1/4;
                        CycScanSta = AUTOK_CYC_SCAN_CHNG_CKGEN;
                        
                        
                        if (!ck_sel)
                            pad_delay_period_cycle = autok_paddly_per_cyc_eval;
                        
                        else {
                            if (!fCMDEdgeDefault)
                               fCMDEdgeDefault = 1; 
                            goto ReTuneMatrix;
                        }
                    }
                }
                
                else if (!CMDMatChar[CycScanSta].pad_trans_cnt1 && 
                         !CMDMatChar[CycScanSta].pad_trans_cnt2) {

                    
                    if (CMDMatChar[CycScanSta-1].pad_trans_cnt1 && 
                        !CMDMatChar[CycScanSta-1].pad_trans_cnt2) {

                        
                        if (CMDMatChar[CycScanSta-1].pad_trans_s1 && 
                            !CMDMatChar[CycScanSta-1].pad_trans_e1)
                            CMDMatChar[CycScanSta-1].pad_trans_m1 = 
                                CMDMatChar[CycScanSta-1].pad_trans_s1 + 
                                    (AUTOK_CMD_EDGE_MATRIX_SCAN ? 
                                     (AUTOK_TRANS_BOUND_FALLING/2) : (AUTOK_TRANS_BOUND_RISING/2));

                        ck_sel = CMDMatChar[CycScanSta-1].pad_trans_m1/4;
                        CycScanSta = AUTOK_CYC_SCAN_CHNG_CKGEN;
                        
                        
                        if (!ck_sel)
                            pad_delay_period_cycle = autok_paddly_per_cyc_eval;
                        
                        else {
                            if (fCMDEdgeDefault)
                               fCMDEdgeDefault = 0; 
                            goto ReTuneMatrix;
                        }
                    }
                    
                    
                    if (!CMDMatChar[CycScanSta-1].pad_trans_cnt1 && 
                        !CMDMatChar[CycScanSta-1].pad_trans_cnt2) {

                        
                        pad_delay_period_cycle = autok_paddly_per_cyc_eval;
                    }
                }
            break;

            case AUTOK_CYC_SCAN_CHNG_CKGEN:
                
                if (CMDMatChar[CycScanSta].pad_trans_cnt1 &&
                    !CMDMatChar[CycScanSta].pad_trans_cnt2) {

                    
                    if (CMDMatChar[CycScanSta].pad_trans_cnt1 &&
                        CMDMatChar[CycScanSta].pad_trans_s1 &&
                        CMDMatChar[CycScanSta].pad_trans_e1) {

                        if (fCMDEdgeDefault) {
                            if (AUTOK_CMD_EDGE_MATRIX_SCAN)
                                autok_trans_bound_falling = 
                                    CMDMatChar[CycScanSta].pad_trans_cnt1;
                            else
                                autok_trans_bound_rising = 
                                    CMDMatChar[CycScanSta].pad_trans_cnt1;

                            AUTOK_PRINT("Update transition boundary len:%d(%s edge)\r\n", 
                                (AUTOK_CMD_EDGE_MATRIX_SCAN ? 
                                 autok_trans_bound_falling : autok_trans_bound_rising),
                                (AUTOK_CMD_EDGE_MATRIX_SCAN ? "falling" : "rising"));
                        }
                        else {
                            if (AUTOK_CMD_EDGE_MATRIX_SCAN)
                                autok_trans_bound_rising = 
                                    CMDMatChar[CycScanSta].pad_trans_cnt1;
                            else
                                autok_trans_bound_falling = 
                                    CMDMatChar[CycScanSta].pad_trans_cnt1;

                            AUTOK_PRINT("Update transition boundary len:%d(%s edge)\r\n", 
                                (AUTOK_CMD_EDGE_MATRIX_SCAN ? 
                                 autok_trans_bound_rising : autok_trans_bound_falling),
                                (AUTOK_CMD_EDGE_MATRIX_SCAN ? "rising" : "falling"));
                        }
                    }

                    if (ck_sel > 2)
                        pad_shift = ck_sel * MIN_CLK_GEN_DELAY_IN_PS / MIN_PAD_DELAY_IN_PS;
                    else
                        pad_shift = ck_sel * 4;

                    
                    if (fCMDEdgeDefault) {

                        AUTOK_PRINT("Before shift, s1:%d, m1:%d, e1:%d, pad_shift:%d\r\n",
                            CMDMatChar[AUTOK_CYC_SCAN_CHNG_EDGE].pad_trans_s1,
                            CMDMatChar[AUTOK_CYC_SCAN_CHNG_EDGE].pad_trans_m1,
                            CMDMatChar[AUTOK_CYC_SCAN_CHNG_EDGE].pad_trans_e1, pad_shift);

                        
                        CMDMatChar[AUTOK_CYC_SCAN_CHNG_EDGE].pad_trans_m1 -= pad_shift;
                        CMDMatChar[AUTOK_CYC_SCAN_CHNG_EDGE].pad_trans_e1 -= pad_shift;

                        if (CMDMatChar[AUTOK_CYC_SCAN_CHNG_EDGE].pad_trans_e1 > 0) {
                            CMDMatChar[AUTOK_CYC_SCAN_CHNG_EDGE].pad_trans_s1 =
                                CMDMatChar[AUTOK_CYC_SCAN_CHNG_EDGE].pad_trans_e1 - 
                                (AUTOK_CMD_EDGE_MATRIX_SCAN ? 
                                AUTOK_TRANS_BOUND_RISING : AUTOK_TRANS_BOUND_FALLING) + 1;

                            if (CMDMatChar[AUTOK_CYC_SCAN_CHNG_EDGE].pad_trans_s1 < 0)
                                CMDMatChar[AUTOK_CYC_SCAN_CHNG_EDGE].pad_trans_s1 = 0;
                        }
                        else {
                            CMDMatChar[AUTOK_CYC_SCAN_CHNG_EDGE].pad_trans_e1 = 0;
                            CMDMatChar[AUTOK_CYC_SCAN_CHNG_EDGE].pad_trans_s1 = 0;
                        }

                        AUTOK_PRINT("Calc. the shift effect, S1:%d, E1:%d, BaseEdge:%s\r\n",
                            CMDMatChar[AUTOK_CYC_SCAN_CHNG_EDGE].pad_trans_s1,
                            CMDMatChar[AUTOK_CYC_SCAN_CHNG_EDGE].pad_trans_e1,
                            AUTOK_CMD_EDGE_MATRIX_SCAN ? "Rising" : "Falling");
                        
                        autok_calc_cycle(pad_delay_period_cycle, 
                                         CMDMatChar[AUTOK_CYC_SCAN_CHNG_EDGE],
                                         CMDMatChar[CycScanSta], 1, 1, 0,
                                         AUTOK_CMD_EDGE_MATRIX_SCAN ? 1 : 0);
                    }
                    
                    else {
                        AUTOK_PRINT("Before shift, s1:%d, m1:%d, e1:%d, pad_shift:%d\r\n",
                            CMDMatChar[AUTOK_CYC_SCAN_INIT].pad_trans_s1,
                            CMDMatChar[AUTOK_CYC_SCAN_INIT].pad_trans_m1,
                            CMDMatChar[AUTOK_CYC_SCAN_INIT].pad_trans_e1, pad_shift);

                        
                        CMDMatChar[AUTOK_CYC_SCAN_INIT].pad_trans_m1 -= pad_shift;
                        CMDMatChar[AUTOK_CYC_SCAN_INIT].pad_trans_e1 -= pad_shift;

                        if (CMDMatChar[AUTOK_CYC_SCAN_INIT].pad_trans_e1 > 0) {
                            CMDMatChar[AUTOK_CYC_SCAN_INIT].pad_trans_s1 =
                                CMDMatChar[AUTOK_CYC_SCAN_INIT].pad_trans_e1 - 
                                (AUTOK_CMD_EDGE_MATRIX_SCAN ? 
                                AUTOK_TRANS_BOUND_FALLING : AUTOK_TRANS_BOUND_RISING) + 1;

                            if (CMDMatChar[AUTOK_CYC_SCAN_INIT].pad_trans_s1 < 0)
                                CMDMatChar[AUTOK_CYC_SCAN_INIT].pad_trans_s1 = 0;
                        }
                        else {
                            CMDMatChar[AUTOK_CYC_SCAN_INIT].pad_trans_e1 = 0;
                            CMDMatChar[AUTOK_CYC_SCAN_INIT].pad_trans_s1 = 0;
                        }
                        
                        AUTOK_PRINT("Calc. the shift effect, S1:%d, E1:%d, BaseEdge:%s\r\n",
                            CMDMatChar[AUTOK_CYC_SCAN_INIT].pad_trans_s1,
                            CMDMatChar[AUTOK_CYC_SCAN_INIT].pad_trans_e1,
                            AUTOK_CMD_EDGE_MATRIX_SCAN ? "Falling" : "Rising");

                        autok_calc_cycle(pad_delay_period_cycle, 
                                         CMDMatChar[AUTOK_CYC_SCAN_INIT],
                                         CMDMatChar[CycScanSta], 1, 1, 0,
                                         AUTOK_CMD_EDGE_MATRIX_SCAN ? 0 : 1);
                    }

                    pad_delay_period_cycle *= 2;
                }
                
                else if (!CMDMatChar[CycScanSta].pad_trans_cnt1 && 
                         !CMDMatChar[CycScanSta].pad_trans_cnt2) {

                    
                    pad_delay_period_cycle = autok_paddly_per_cyc_eval;
                }
            break;

            default:
            break;
        }
    }

    if (pad_delay_period_cycle)
        AUTOK_PRINT("Find 1T in pad delay: %d\r\n", pad_delay_period_cycle);
    else
        AUTOK_PRINT("Can NOT find pad delay cycle\r\n");

    rPadDlyRes.CKGenPeriodLen = DIV_CEIL_FUNC((pad_delay_period_cycle*MIN_PAD_DELAY_IN_PS), 
                                    MIN_CLK_GEN_DELAY_IN_PS);

    AUTOK_PRINT("Calculated 1T in CKGEN: %d\r\n", rPadDlyRes.CKGenPeriodLen);
    if (rPadDlyRes.CKGenPeriodLen >= (SCALE_CKGEN_MSDC_DLY_SEL-1))
        rPadDlyRes.CKGenPeriodLen = SCALE_CKGEN_MSDC_DLY_SEL-1;

    
    k = 0;
    while(k <= rPadDlyRes.CKGenPeriodLen) {
        if ((!fRdatPatFound || !rPadDlyRes.fHoleCK) && !fStopRDAT) {

            
            
            
            msdc_autok_adjust_param(host, CKGEN_MSDC_DLY_SEL, &k, MSDC_WRITE);
            sel = 0;
            msdc_autok_adjust_param(host, CMD_EDGE, &sel, MSDC_WRITE);
        
            autok_cmd_cmdrrdly[0].raw_data = 0;
            sel = 0;
            msdc_autok_adjust_param(host, CMD_RESP_RD_DLY, &sel, MSDC_WRITE);
            for (m = 0; m < SCALE_PAD_TUNE_CMDRDLY; m++) {
                msdc_autok_adjust_param(host, CMD_RD_DLY, &m, MSDC_WRITE);
                for (cnt = 0; cnt < (AUTOK_CMD_TIMES/2); cnt++) {
                    if (autok_cmd_test(host) != E_RESULT_PASS) {
                        
                        autok_cmd_cmdrrdly[0].raw_data |= (1 << m);
                        break;
                    }
                }
            }
            autok_cmd_cmdrrdly[0].score = autok_simple_score(autok_cmd_cmdrrdly[0].raw_data);
            AUTOK_PRINT("CKGEN_MSDC_DLY \t PAD_TUNE_CMDRDLY \r\n");
            AUTOK_PRINT("%02d \t %02d \t %s\r\n", k, autok_cmd_cmdrrdly[0].score,g_tune_result_str);
        
            autok_select_range(autok_cmd_cmdrrdly[0].raw_data, &sel);
            msdc_autok_adjust_param(host, CMD_RD_DLY, &sel, MSDC_WRITE);
            
            
            AUTOK_PRINT("Scanning Read data...\r\n");
            data.raw_data = 0;
            reTuneCmd = 0;
            x = 0;
            
            while(x < SCALE_PAD_TUNE_DATRDDLY && reTuneCmd == 0) {

                msdc_autok_adjust_param(host, DAT_RD_DLY, &x, MSDC_WRITE);
                for (m = 0; m < AUTOK_RDAT_TIMES; m++) {
                    if ((res = autok_read_test(host)) != E_RESULT_PASS) {
                        data.raw_data|= (1 << x);
                        
                        if (autok_recovery(host)) {
                            AUTOK_PRINT("===tune read : error, fail to bring to tranfer status===\r\n");
                            goto exit;
                        }
                        if (res == E_RESULT_CMD_CRC) {
                            AUTOK_PRINT("[WARN] CMD CRC error in tuning read[%d %d], need to tune command again!!\r\n",x,m);
                            reTuneCmd = 1;
                        }
                        break;
                    }
                }
                
                x++;
            }

            if((reTuneCmd == 1) && (reTuneCmdCnt < 2)) {
                AUTOK_PRINT("[ERR] Re-start to tune CMD again!!\r\n");
                reTuneCmdCnt++;
                k = 0;
                continue;
            }
            else if (reTuneCmdCnt >= 2)
                AUTOK_ERR();

            autok_rdata_scan[k].RawData = data.raw_data;
            autok_rdata_scan[k].CurCKGEN = k;
            autok_check_rawd_style(&autok_rdata_scan[k], 1);

            #ifdef AUTOK_RDAT_ACC
            
            if (!autok_rdata_scan[k].fInvalidCKGEN) {

                
                if (autok_rdata_scan[k].BoundReg1_S && 
                    autok_rdata_scan[k].BoundReg1_E &&
                    !rPadDlyRes.fFBound) {

                    
                    if (autok_rdata_scan[k].Reg1Cnt > AUTOK_RDAT_FBOUND_TH) {
                        
                        if (k >= 1) {
                            if (!autok_rdata_scan[k-1].fInvalidCKGEN &&
                                autok_rdata_scan[k-1].BoundReg1_S &&
                                ((autok_rdata_scan[k].BoundReg1_S+1) < 
                                    autok_rdata_scan[k-1].BoundReg1_S) &&
                                (ABS_DIFF(autok_rdata_scan[k-1].BoundReg1_S, 
                                    autok_rdata_scan[k].BoundReg1_S) <= 5)) {
                                fRdatPatFound = 1;
                                rPadDlyRes.fFBound = 1;
                                rPadDlyRes.FBoundCK = k;
                            }
                        }
                    }
                }
                
                else if (!autok_rdata_scan[k].BoundReg1_S && autok_rdata_scan[k].Reg1Cnt) {
                    
                    rPadDlyRes.fLBound = 1;
                    rPadDlyRes.LBoundCK = k;
                }

                
                if (!rPadDlyRes.fHoleCK && 
                    (k >= (AUTOK_CKGEN_ALLOW_MAX + 1)) && rPadDlyRes.fFBound)
                    fStopRDAT = 1;
            }
            
            else if (!rPadDlyRes.fHoleCK) {
                rPadDlyRes.fHoleCK = 1;
                rPadDlyRes.HoleCK = k;
            }

            #endif

            k++;
        }    
        else
            break;
    }

    
    rPadDlyRes.PadDlyPeriodLen = pad_delay_period_cycle;
    pAutoKData[E_MSDC_PAD_DLY_PERIOD].data.sel = pad_delay_period_cycle;
    
exit:
    return rPadDlyRes;
}

#elif defined(AUTOK_CYC_ALG_0)

static AUTOK_CYC_SCAN_RES_T
autok_cycle_scan(struct msdc_host *host, U_AUTOK_INTERFACE_DATA *pAutoKData)
{
    unsigned int k,m,x,k_bak;
    unsigned int sel;

    #ifdef AUTOK_CMD_TUNE_LEGACY
    unsigned int pass, stop, single_init, cross, FakeCnt; 
    unsigned char fCMDIntDlyFound = 0, fCMDMatrixStress = 0;
    unsigned char CMDIntDly = 0, CMDIntDlyS, CMDIntDlyE;
    E_AUTOK_ERR_STA err;
    unsigned int max_score;
    unsigned int max_numZero = 0;
    #endif

    unsigned int fRdatPatFound = 0, fStopRDAT = 0;
    unsigned int pad_delay_period_cycle = 0;
    unsigned int clk_gen_delay_period_cycle, ckgen_dly_period_max;
    unsigned int periodCycle;
    unsigned int minPadCycleScore;
    unsigned int minClkGenCycleScore;
    unsigned char bTryFindPadCycle = 1;
    unsigned char bTryFindClkGenCycle = 1;
    unsigned int range_min, range_max, autok_cmd_times;
    int reTuneCmd, cnt, reTuneCmdCnt = 0;

    unsigned int fCMDPatDoubleChk = 0, fReScanCycle = 1;
    int PadDlyDiff;
    AUTOK_CMDPAT_COMP_E fIdent;
    AUTOK_CMDPAT_DOUBLE_CHK_T rCMDPatChk;
    
    E_RESULT_TYPE res;
    S_AUTOK_CMD_DLY data;
    AUTOK_CYC_SCAN_RES_T rPadDlyRes;

ReScanCycle:

    
    fRdatPatFound = 0;
    pad_delay_period_cycle = 0;
    bTryFindPadCycle = 1;
    bTryFindClkGenCycle = 1;

    fCMDPatDoubleChk = 0;

    #ifdef AUTOK_CMD_TUNE_LEGACY
    fCMDMatrixStress = 0;
    fCMDIntDlyFound = 0;
    CMDIntDly = 0;
    max_numZero = 0;
    #endif
    
    memset(&rPadDlyRes, 0, sizeof(rPadDlyRes));
    memset(&rCMDPatChk, 0, sizeof(rCMDPatChk));
    memset(autok_cmd_cmdrrdly, 0, sizeof(S_AUTOK_CMD_DLY)*SCALE_CMD_RSP_DLY_SEL);
    memset(autok_cmd_ckgdly, 0, sizeof(S_AUTOK_CMD_DLY)*SCALE_CKGEN_MSDC_DLY_SEL);
    memset(autok_cmd_ckgdly_cmdrrdly0, 0, sizeof(S_AUTOK_CMD_DLY)*SCALE_CKGEN_MSDC_DLY_SEL);
    memset(autok_ckg_data, 0, sizeof(S_AUTOK_CKGEN_DATA)*SCALE_CKGEN_MSDC_DLY_SEL);
    memset(autok_rdata_scan, 0, sizeof(autok_rdata_scan));
    memset(&data, 0, sizeof(data));
    #ifdef MT6290
    periodCycle = FREQ_MHZ_2_PERIOD_CYCLE_IN_PS(host->sclk/1000000);
    #else
    periodCycle = FREQ_MHZ_2_PERIOD_CYCLE_IN_PS(host->mclk/1000000);
    #endif
    clk_gen_delay_period_cycle = MAX_SCALE_OF_CLK_GEN_IN_ONE_CYCLE(periodCycle);
    clk_gen_delay_period_cycle = THRESHOLD_VAL(clk_gen_delay_period_cycle, SCALE_CKGEN_MSDC_DLY_SEL);
    ckgen_dly_period_max = clk_gen_delay_period_cycle;

    #ifdef MT6290
    minPadCycleScore = MIN_SCORE_OF_PAD_DELAY_IN_ONE_CYCLE(periodCycle);
    #else
    minPadCycleScore = SCALE_PAD_TUNE_CMDRDLY;
    #endif
    
    if(minPadCycleScore >= SCALE_PAD_TUNE_CMDRDLY) {
        bTryFindPadCycle = 0;
    }
    minClkGenCycleScore = MIN_SCORE_OF_CLK_GEN_IN_ONE_CYCLE(periodCycle);
    if(minClkGenCycleScore >= SCALE_CKGEN_MSDC_DLY_SEL) {
        bTryFindClkGenCycle = 0;
    }

    AUTOK_PRINT("period=%d MaxCkgen period=%d MinCkgen period=%d MinPad period=%d\r\n", \
            periodCycle, clk_gen_delay_period_cycle, minClkGenCycleScore, minPadCycleScore);

    
    range_min = 0;
    range_max = clk_gen_delay_period_cycle;
    
    k = range_min;
    range_max = THRESHOLD_VAL(range_max,SCALE_CKGEN_MSDC_DLY_SEL-1);

    AUTOK_PRINT("ckg scan range from %d to %d\r\n", k, range_max);
    
    while(k <= range_max) {
    
        pAutoKData[E_MSDC_CKGEN_MSDC_DLY_SEL].data.sel = k;
        msdc_autok_adjust_param(host, CKGEN_MSDC_DLY_SEL, &k, MSDC_WRITE);

        sel = 0;
        pAutoKData[E_MSDC_IOCON_RSPL].data.sel = sel;
        msdc_autok_adjust_param(host, CMD_EDGE, &sel, MSDC_WRITE);

        
        if (!fCMDPatDoubleChk) {

            if (!k)
                autok_cmd_times = AUTOK_CMD_TIMES;
            else
                autok_cmd_times = AUTOK_CMD_TIMES;

            autok_cmd_cmdrrdly[0].raw_data = 0;
            sel = 0;
            msdc_autok_adjust_param(host, CMD_RESP_RD_DLY, &sel, MSDC_WRITE);
            for (m = 0; m < SCALE_PAD_TUNE_CMDRDLY; m++) {
                msdc_autok_adjust_param(host, CMD_RD_DLY, &m, MSDC_WRITE);
                for (cnt = 0; cnt < autok_cmd_times; cnt++) {
                    if (autok_cmd_test(host) != E_RESULT_PASS) {
                        
                        autok_cmd_cmdrrdly[0].raw_data |= (1 << m);
                        break;
                    }
                }
            }
            autok_cmd_cmdrrdly[0].score = autok_simple_score(autok_cmd_cmdrrdly[0].raw_data);

            autok_cmd_ckgdly_cmdrrdly0[k].raw_data = autok_cmd_cmdrrdly[0].raw_data;
        }
        
        
        else {
            AUTOK_PRINT("Shift CMD internal delay and double check the cycle...\r\n");
            AUTOK_PRINT("CK_S(%d),CK_E(%d),CMD_Int_Shift(%d),diff_b4(%d)\r\n", 
                    rCMDPatChk.ck_s_b4, rCMDPatChk.ck_e_b4, 
                    rCMDPatChk.cmd_int_shift, rCMDPatChk.diff_b4);

            
            sel = rCMDPatChk.cmd_int_shift;
            msdc_autok_adjust_param(host, CMD_RESP_RD_DLY, &sel, MSDC_WRITE);

            autok_cmd_times = AUTOK_CMD_TIMES;
            
            
            msdc_autok_adjust_param(host, CKGEN_MSDC_DLY_SEL, &rCMDPatChk.ck_s_b4, MSDC_WRITE);

            autok_cmd_cmdrrdly[0].raw_data = 0;
            for (m = 0; m < SCALE_PAD_TUNE_CMDRDLY; m++) {
                msdc_autok_adjust_param(host, CMD_RD_DLY, &m, MSDC_WRITE);
                for (cnt = 0; cnt < autok_cmd_times; cnt++) {
                    if (autok_cmd_test(host) != E_RESULT_PASS) {
                        
                        autok_cmd_cmdrrdly[0].raw_data |= (1 << m);
                        break;
                    }
                }
            }
            rCMDPatChk.raw_s_sh = autok_cmd_cmdrrdly[0].raw_data;
            autok_cmd_cmdrrdly[0].score = autok_simple_score(autok_cmd_cmdrrdly[0].raw_data);
            AUTOK_PRINT("CKGEN(%02d): %s,  score=%d\r\n", rCMDPatChk.ck_s_b4, 
                g_tune_result_str, autok_cmd_cmdrrdly[0].score);

            
            msdc_autok_adjust_param(host, CKGEN_MSDC_DLY_SEL, &rCMDPatChk.ck_e_b4, MSDC_WRITE);

            autok_cmd_cmdrrdly[0].raw_data = 0;
            for (m = 0; m < SCALE_PAD_TUNE_CMDRDLY; m++) {
                msdc_autok_adjust_param(host, CMD_RD_DLY, &m, MSDC_WRITE);
                for (cnt = 0; cnt < autok_cmd_times; cnt++) {
                    if (autok_cmd_test(host) != E_RESULT_PASS) {
                        
                        autok_cmd_cmdrrdly[0].raw_data |= (1 << m);
                        break;
                    }
                }
            }
            rCMDPatChk.raw_e_sh = autok_cmd_cmdrrdly[0].raw_data;
            autok_cmd_cmdrrdly[0].score = autok_simple_score(autok_cmd_cmdrrdly[0].raw_data);
            AUTOK_PRINT("CKGEN(%02d): %s,  score=%d\r\n", rCMDPatChk.ck_e_b4, 
                g_tune_result_str, autok_cmd_cmdrrdly[0].score);

            
            fIdent = autok_comp_cmd_pat(&rCMDPatChk, 1, &PadDlyDiff, 1);

            if ((fIdent == CMDPAT_IDENTICAL) &&
                (ABS_DIFF(PadDlyDiff, rCMDPatChk.diff_b4) <= (AUTOK_TUNING_INACCURACY+1))) {

                k = k_bak;

                
                if (rCMDPatChk.scen == CMDPAT_REG_2_L) {
                    pad_delay_period_cycle = (MIN_CLK_GEN_DELAY_IN_PS*
                        (rCMDPatChk.ck_e_b4-rCMDPatChk.ck_s_b4))/MIN_PAD_DELAY_IN_PS;
                    pad_delay_period_cycle += PadDlyDiff;

                    AUTOK_PRINT("Update period change by ckg:pad cycle=%d ckg cycle=%d\r\n",
                        pad_delay_period_cycle, rCMDPatChk.ck_e_b4-rCMDPatChk.ck_s_b4);
                }
                
                goto CycDoubleChk;
            }
            else {
                
                pad_delay_period_cycle = 0;
                fCMDPatDoubleChk = 0;

                minClkGenCycleScore = rCMDPatChk.ck_e_b4 - rCMDPatChk.ck_s_b4 + 1;
                clk_gen_delay_period_cycle = ckgen_dly_period_max;
                k = k_bak + 1;
                AUTOK_PRINT("Resume to scan CKGEN from %d\r\n", minClkGenCycleScore);
                continue;
            }
        }

        AUTOK_PRINT("CKGEN_MSDC_DLY \t PAD_TUNE_CMDRDLY \r\n");
        AUTOK_PRINT("%02d \t %02d \t %s\r\n",k,autok_cmd_cmdrrdly[0].score,g_tune_result_str);

        if ((!fRdatPatFound || !rPadDlyRes.fHoleCK) && !fStopRDAT) {
            autok_select_range(autok_cmd_cmdrrdly[0].raw_data, &pAutoKData[E_MSDC_PAD_TUNE_CMDRDLY].data.sel);
            msdc_autok_adjust_param(host, CMD_RD_DLY, &pAutoKData[E_MSDC_PAD_TUNE_CMDRDLY].data.sel, MSDC_WRITE);
            
            
            AUTOK_PRINT("Scanning Read data...\r\n");
            data.raw_data = 0;
            reTuneCmd = 0;
            x = 0;
            
            while(x < SCALE_PAD_TUNE_DATRDDLY && reTuneCmd == 0) {

                msdc_autok_adjust_param(host, DAT_RD_DLY, &x, MSDC_WRITE);
                for (m = 0; m < AUTOK_RDAT_TIMES; m++) {
                    if ((res = autok_read_test(host)) != E_RESULT_PASS) {
                        data.raw_data|= (1 << x);
                        
                        if (autok_recovery(host)) {
                            AUTOK_PRINT("===tune read : error, fail to bring to tranfer status===\r\n");
                            goto exit;
                        }
                        if (res == E_RESULT_CMD_CRC) {
                            AUTOK_PRINT("[WARN] CMD CRC error in tuning read[%d %d], need to tune command again!!\r\n",x,m);
                            reTuneCmd = 1;
                        }
                        break;
                    }
                }
                
                x++;
            }

            if((reTuneCmd == 1) && (reTuneCmdCnt < 2)) {
                AUTOK_PRINT("[ERR] Re-start to tune CMD again!!\r\n");
                reTuneCmdCnt++;
                k = 0;
                continue;
            }
            else if (reTuneCmdCnt >= 2)
                AUTOK_ERR();

            autok_rdata_scan[k].RawData = data.raw_data;
            autok_rdata_scan[k].CurCKGEN = k;
            autok_check_rawd_style(&autok_rdata_scan[k], 1);

            #ifdef AUTOK_RDAT_ACC
            
            if (!autok_rdata_scan[k].fInvalidCKGEN) {

                
                if (autok_rdata_scan[k].BoundReg1_S && 
                    autok_rdata_scan[k].BoundReg1_E &&
                    !rPadDlyRes.fFBound) {

                    
                    if (autok_rdata_scan[k].Reg1Cnt > AUTOK_RDAT_FBOUND_TH) {
                        
                        if (k >= 1) {
                            if (!autok_rdata_scan[k-1].fInvalidCKGEN &&
                                autok_rdata_scan[k-1].BoundReg1_S &&
                                ((autok_rdata_scan[k].BoundReg1_S+1) < 
                                    autok_rdata_scan[k-1].BoundReg1_S) &&
                                (ABS_DIFF(autok_rdata_scan[k-1].BoundReg1_S, 
                                    autok_rdata_scan[k].BoundReg1_S) <= 5)) {
                                fRdatPatFound = 1;
                                rPadDlyRes.fFBound = 1;
                                rPadDlyRes.FBoundCK = k;
                            }
                        }
                    }
                }
                
                else if (!autok_rdata_scan[k].BoundReg1_S && autok_rdata_scan[k].Reg1Cnt) {
                    
                    rPadDlyRes.fLBound = 1;
                    rPadDlyRes.LBoundCK = k;
                }

                
                if (!rPadDlyRes.fHoleCK && 
                    (k >= (AUTOK_CKGEN_ALLOW_MAX + 1)) && rPadDlyRes.fFBound)
                    fStopRDAT = 1;
            }
            
            else if (!rPadDlyRes.fHoleCK) {
                rPadDlyRes.fHoleCK = 1;
                rPadDlyRes.HoleCK = k;
            }

            #endif
        }

    #ifdef AUTOK_CMD_TUNE_LEGACY
        AUTOK_PRINT("CKGEN_MSDC_DLY \t PAD_TUNE_CMDRRDLY \t PAD_TUNE_CMDRDLY \r\n");
        pass = 0;
        single_init = 0;
        cross = 0;
        FakeCnt = 0;
        max_score = 0;

        
        sel = AUTOK_CMD_EDGE_MATRIX_SCAN & 0x01;
        pAutoKData[E_MSDC_IOCON_RSPL].data.sel = sel;
        msdc_autok_adjust_param(host, CMD_EDGE, &sel, MSDC_WRITE);

        
        if (fCMDIntDlyFound) {
            CMDIntDlyS = CMDIntDly;

            #ifdef MT6290
            CMDIntDlyE = CMDIntDly + 1;
            
            #else
            CMDIntDlyE = CMDIntDly;
            max_score = 32;
            #endif
        }
        else {
            CMDIntDlyS = 0;
            CMDIntDlyE = SCALE_CMD_RSP_DLY_SEL;
        }

        
        autok_cmd_times = AUTOK_CMD_TIMES;
        if (fCMDMatrixStress)
            autok_cmd_times *= 2;

        for (x=CMDIntDlyS; x<CMDIntDlyE; x++){
            autok_cmd_cmdrrdly[x].raw_data = 0;
            autok_cmd_cmdrrdly[x].fstPosErrEnd = 0;
            autok_cmddly_stop_bit[x] = 0;
            stop = 0;
            err = ERR_NONE;
            msdc_autok_adjust_param(host, CMD_RESP_RD_DLY, &x, MSDC_WRITE);
            for (m = 0; m < SCALE_PAD_TUNE_CMDRDLY && stop == 0; m++) {
                msdc_autok_adjust_param(host, CMD_RD_DLY, &m, MSDC_WRITE);
                for (cnt = 0; cnt < autok_cmd_times; cnt++) {
                    if (autok_cmd_test(host) != E_RESULT_PASS) {
                        
                        autok_cmd_cmdrrdly[x].raw_data |= (1 << m);
                        if ((err == PASS_AFTER_ERR) && !fCMDIntDlyFound) {

                            
                            if ((cross > 1) && (cross < (AUTOK_CMDMAT_CROSS_MAR - FakeCnt))) {
                                if (pass) {
                                    pass = 0;
                                    FakeCnt++;
                                    
                                    if (FakeCnt >= AUTOK_CMDMAT_CROSS_MAR) {
                                        AUTOK_PRINT("[ERR]Too many fake points!\r\n");
                                        AUTOK_ERR();
                                    }
                                }
                            }
                            
                            stop = 1;
                            autok_cmddly_stop_bit[x] = m;
                            
                        }
                        else if(err == ERR_NONE) {
                            err = ERR_OCCURE;
                        }
                        break;
                    }
                }
                if( (cnt == autok_cmd_times)&& (err == ERR_OCCURE)) {
                    err = PASS_AFTER_ERR;
                    autok_cmd_cmdrrdly[x].fstPosErrEnd = m - 1;
                }
            }

            
            if (!x && !fCMDIntDlyFound && !stop && !single_init) {
                single_init = 1;
            }
            else if (!x && !fCMDIntDlyFound && stop && !cross)
                cross = 1;
            
            
            if(pass == 1 && stop == 1) {
                break;
            }
            else if(pass == 0 && stop == 1) {
                
                if ((single_init && x && !cross) || FakeCnt) {
                    AUTOK_PRINT("[NOTICE]There are GAPs here.....(%d,%d)\r\n", 
                        cross, FakeCnt);

                    if (!FakeCnt) {
                        
                        if (single_init && (x >= 3)) {
                            
                            if (ABS_DIFF(autok_cmd_cmdrrdly[x-1].fstPosErr, autok_cmd_cmdrrdly[x-3].fstPosErr) >= 2) {

                                AUTOK_PRINT("[NOTICE]CrossP happenned!\r\n");
                                single_init = 0;
                                cross = x;
                                pass = 1;
                            }
                            
                            else {
                                AUTOK_PRINT("[NOTICE]CrossP is to be happenned!\r\n");
                                max_score = 0;
                                cross = 1;
                            }
                        }
                        
                        else {
                            
                            if ((ABS_DIFF(autok_cmd_cmdrrdly[x-1].fstPosErr, autok_cmd_cmdrrdly[0].fstPosErr) >= (x - 1)) &&
                               ((autok_cmddly_stop_bit[x] - autok_cmd_cmdrrdly[x].fstPosErrEnd) <= 2)) {

                                AUTOK_PRINT("[NOTICE]CrossP happenned!\r\n");
                                single_init = 0;
                                cross = x;
                                pass = 1;
                            }
                            
                            else {
                                AUTOK_PRINT("[NOTICE]CrossP is to be happenned!\r\n");
                                max_score = 0;
                                cross = 1;
                            }
                        }
                    }
                    
                    else
                        pass = 1;
                }
                
                continue;
            }
            
            if (cross) {
                pass = 1;
                
                
                cross++;
            }

            autok_cmd_cmdrrdly[x].score = autok_check_score(autok_cmd_cmdrrdly[x].raw_data, \
                                                              &autok_cmd_cmdrrdly[x].numOfzero, \
                                                              &autok_cmd_cmdrrdly[x].fstPosErr, \
                                                              &autok_cmd_cmdrrdly[x].period,
                                                              minPadCycleScore);
            AUTOK_PRINT("%02d \t %02d \t %02d \t %s\r\n",k,x,autok_cmd_cmdrrdly[x].score,g_tune_result_str);

            if(autok_cmd_cmdrrdly[x].score > max_score) {
                max_score = autok_cmd_cmdrrdly[x].score;
                max_numZero = autok_cmd_cmdrrdly[x].numOfzero;
                sel = x;
            }
            else if(autok_cmd_cmdrrdly[x].score == max_score) {
                if(autok_cmd_cmdrrdly[x].numOfzero > max_numZero) {
                    max_numZero = autok_cmd_cmdrrdly[x].numOfzero;
                    sel = x;
                }
            }
        }

        
        if (k && !fCMDIntDlyFound && !fCMDMatrixStress) {
            if (ABS_DIFF(sel, autok_ckg_data[k-1].interDelaySel) > 8) {

                AUTOK_PRINT("[WARN]Restart to scan CKGEN due to invalid CMD matrix!IntDly Gap(%d)\r\n",
                    ABS_DIFF(sel, autok_ckg_data[k-1].interDelaySel));

                
                k = k -1;
                fCMDMatrixStress = 1;
                continue;
            }
        }

        
        if(max_score == 0) {
            AUTOK_PRINT("autok algorithm for tuning cmd internal delay need to scan more!!\r\n");
            
            for (x=0; x<SCALE_CMD_RSP_DLY_SEL; x++){
                msdc_autok_adjust_param(host, CMD_RESP_RD_DLY, &x, MSDC_WRITE);

                if (autok_cmddly_stop_bit[x] == (SCALE_PAD_TUNE_CMDRDLY - 1))
                    autok_cmddly_stop_bit[x] -= 1;
                    
                for (m = autok_cmddly_stop_bit[x]+1; m < SCALE_PAD_TUNE_CMDRDLY; m++) {
                    msdc_autok_adjust_param(host, CMD_RD_DLY, &m, MSDC_WRITE);
                    for (cnt = 0; cnt < AUTOK_CMD_TIMES; cnt++) {
                        if (autok_cmd_test(host) != E_RESULT_PASS) {
                            
                            autok_cmd_cmdrrdly[x].raw_data |= (1 << m);
                            break;
                        }
                    }
                }
                autok_cmd_cmdrrdly[x].score = autok_check_score(autok_cmd_cmdrrdly[x].raw_data, \
                                              &autok_cmd_cmdrrdly[x].numOfzero, \
                                              &autok_cmd_cmdrrdly[x].fstPosErr, \
                                              &autok_cmd_cmdrrdly[x].period,
                                              minPadCycleScore);
                AUTOK_PRINT("%02d \t %02d \t %02d \t %s\r\n",k,x,autok_cmd_cmdrrdly[x].score,g_tune_result_str);

                if(autok_cmd_cmdrrdly[x].score > max_score) {
                    max_score = autok_cmd_cmdrrdly[x].score;
                    max_numZero = autok_cmd_cmdrrdly[x].numOfzero;
                    sel = x;
                }
                else if(autok_cmd_cmdrrdly[x].score == max_score) {
                    if(autok_cmd_cmdrrdly[x].numOfzero > max_numZero) {
                        max_numZero = autok_cmd_cmdrrdly[x].numOfzero;
                        sel = x;
                    }
                }
            }
        }

        #ifdef MT6290
        if(1) {
        #else
        if(!fCMDIntDlyFound) {
        #endif

            
            autok_ckg_data[k].interDelaySel = sel;
            autok_cmd_ckgdly[k] = autok_cmd_cmdrrdly[sel];
            msdc_autok_adjust_param(host, CMD_RESP_RD_DLY, &sel, MSDC_WRITE);
            AUTOK_PRINT("CMD internal delay %d score= %d numOfZero=%d fstPosErr=%d\r\n", \
                    sel,max_score,max_numZero,autok_cmd_ckgdly[k].fstPosErr);

            
            sel = ~AUTOK_CMD_EDGE_MATRIX_SCAN & 0x01;
            data.raw_data = 0;
            msdc_autok_adjust_param(host, CMD_EDGE, &sel, MSDC_WRITE);
            for (m = 0; m < SCALE_PAD_TUNE_CMDRDLY; m++) {
                msdc_autok_adjust_param(host, CMD_RD_DLY, &m, MSDC_WRITE);
                for (cnt = 0; cnt < AUTOK_CMD_TIMES; cnt++) {
                    if (autok_cmd_test(host) != E_RESULT_PASS) {
                        
                        data.raw_data |= (1 << m);
                        break;
                    }
                }
            }
            data.score = autok_check_score(data.raw_data, \
                                          &data.numOfzero, \
                                          &data.fstPosErr, \
                                          &data.period,
                                          minPadCycleScore);
                AUTOK_PRINT("%s edge %s score=%d fstPosErr=%d\r\n", sel ? "Falling" : "Rising",
                    g_tune_result_str,data.score,data.fstPosErr);
            sel = AUTOK_CMD_EDGE_MATRIX_SCAN & 0x01;
        #if 0
            if(data.fstPosErr < autok_cmd_ckgdly[k].fstPosErr) {
                sel = ~AUTOK_CMD_EDGE_MATRIX_SCAN & 0x01;
                autok_cmd_ckgdly[k] = data;
            }
        #else
            if(data.score > max_score) {
                sel = ~AUTOK_CMD_EDGE_MATRIX_SCAN & 0x01;
                autok_cmd_ckgdly[k] = data;
            }
            else if(data.score == max_score) {
                if(data.fstPosErr < autok_cmd_ckgdly[k].fstPosErr) {
                    sel = ~AUTOK_CMD_EDGE_MATRIX_SCAN & 0x01;
                    autok_cmd_ckgdly[k] = data;
                }
            }
        #endif

            if(sel != (AUTOK_CMD_EDGE_MATRIX_SCAN & 0x01))
                msdc_autok_adjust_param(host, CMD_EDGE, &sel, MSDC_WRITE);

            pAutoKData[E_MSDC_IOCON_RSPL].data.sel = sel;

            AUTOK_PRINT("%s CMD edge is choosen!\r\n", sel ? "Falling" : "Rising");

            autok_ckg_data[k].cmdEdgeSel = pAutoKData[E_MSDC_IOCON_RSPL].data.sel;
            autok_ckg_data[k].cmdScore = autok_cmd_ckgdly[k].score;
        }

        
        if (!fCMDIntDlyFound && (k >= AUTOK_CKGEN_ALLOW_MAX)) {
            
            fCMDIntDlyFound = 1;
            CMDIntDly = sel;
            
            pAutoKData[E_MSDC_PAD_TUNE_CMDRRDLY].data.sel = sel;
        }
    #endif      
    
        
        if(bTryFindPadCycle == 1 && pad_delay_period_cycle == 0 && autok_cmd_ckgdly[k].period >= minPadCycleScore) {
            
            pad_delay_period_cycle = autok_cmd_ckgdly[k].period;
            clk_gen_delay_period_cycle = DIV_CEIL_FUNC(autok_cmd_ckgdly[k].period, 
                                                DIV_CEIL_FUNC(MIN_CLK_GEN_DELAY_IN_PS, MIN_PAD_DELAY_IN_PS));

            range_max = clk_gen_delay_period_cycle;
            
            rPadDlyRes.fCMDIntDlyConf = 1;
            
            AUTOK_PRINT("period change by pad :pad cycle=%d ckg cycle=%d\r\n", \
                    pad_delay_period_cycle,clk_gen_delay_period_cycle);
        }

        
        if(bTryFindClkGenCycle == 1 && pad_delay_period_cycle == 0 && k >= minClkGenCycleScore) {
            
            for(x=0;x<=0 && pad_delay_period_cycle==0;x++) {
                for(m=x+minClkGenCycleScore;m<=k;m++) {

                    memset(&rCMDPatChk, 0, sizeof(rCMDPatChk));
                    rCMDPatChk.ck_s_b4 = x;
                    rCMDPatChk.raw_s_b4 = autok_cmd_ckgdly_cmdrrdly0[x].raw_data;
                    rCMDPatChk.ck_e_b4 = m;
                    rCMDPatChk.raw_e_b4 = autok_cmd_ckgdly_cmdrrdly0[m].raw_data;

                    
                    fIdent = autok_comp_cmd_pat(&rCMDPatChk, 0, &PadDlyDiff, 1);
                    rCMDPatChk.diff_b4 = PadDlyDiff;
                    
                    if (fIdent == CMDPAT_NONE)
                        continue;

                    #if 0
                    if (fIdent == CMDPAT_HALF_IDENTICAL) {
                        rCMDPatChk.cmd_int_shift = 12;
                    }
                    else if (fIdent == CMDPAT_IDENTICAL) {
                        rCMDPatChk.cmd_int_shift = 6;
                    }
                    #endif

                    #if 0
                    
                    if (((fIdent == CMDPAT_HALF_IDENTICAL) || (fIdent == CMDPAT_IDENTICAL)) && 
                          fCMDPatChkMore) {

                        fCMDPatChkMore = 0;

                        
                        minClkGenCycleScore = m - x + 1;
                        AUTOK_PRINT("Find a fake CKGEN(%d) for a cycle, continue to scan...\r\n", 
                            minClkGenCycleScore - 1);

                        goto CycDoubleChk;
                    }

                    if ((fIdent == CMDPAT_HALF_IDENTICAL) && !fCMDPatNearAtInit)
                        continue;
                    #endif
                    
                    clk_gen_delay_period_cycle = m-x;
                    
                    
                    pad_delay_period_cycle = (MIN_CLK_GEN_DELAY_IN_PS*(m-x))/MIN_PAD_DELAY_IN_PS;
                    pad_delay_period_cycle += PadDlyDiff;
                    
                    AUTOK_PRINT("period change by ckg:pad cycle=%d ckg cycle=%d\r\n", \
                            pad_delay_period_cycle, clk_gen_delay_period_cycle);
                
                    break;
                }
            }

            if (!pad_delay_period_cycle) {
                AUTOK_PRINT("[NOTICE]It is NOT the 1T whole cycle!\r\n");

                
                minClkGenCycleScore += 1;
            }
        }

CycDoubleChk:
        if (!fCMDPatDoubleChk && !rPadDlyRes.fCMDIntDlyConf && pad_delay_period_cycle) {
            fCMDPatDoubleChk = 1;
            k_bak = k;
            continue;
        }
        else if (fCMDPatDoubleChk && pad_delay_period_cycle) {
            range_max = clk_gen_delay_period_cycle;
        }
        
        
        if (k >= clk_gen_delay_period_cycle)
            break;

        k++;
    }

    
    AUTOK_PRINT("Show the raw data by CMD cycle scan: \r\n");
    for(k = range_min; k <= ckgen_dly_period_max; k++) {
        autok_simple_score(autok_cmd_ckgdly_cmdrrdly0[k].raw_data);
        AUTOK_PRINT("%2d \t %s\r\n", k, g_tune_result_str);
    }

    
    if (pad_delay_period_cycle == 0) {
        AUTOK_PRINT("[WARN]Can NOT find pad delay cycle!\r\n");

        
        autok_get_current_vcore_offset();
        
        
        AUTOK_PRINT("Show the raw data of RDAT: \r\n");
        for(k = range_min; k <= MAX_GET(rPadDlyRes.FBoundCK, rPadDlyRes.LBoundCK); k++) {
            autok_simple_score(autok_rdata_scan[k].RawData);
            AUTOK_PRINT("%2d \t %s\r\n", k, g_tune_result_str);
        }
        
        if (fReScanCycle) {
            AUTOK_PRINT("Try to rescan the cycle...\r\n");
            fReScanCycle--;
            goto ReScanCycle;
        }
        else {
            AUTOK_PRINT("[ERR]Still can NOT find pad delay cycle anyway!\r\n");
            AUTOK_ERR();
        }
    }

    
    rPadDlyRes.CKGenPeriodLen = clk_gen_delay_period_cycle; 
    rPadDlyRes.PadDlyPeriodLen = pad_delay_period_cycle;
    pAutoKData[E_MSDC_PAD_DLY_PERIOD].data.sel = pad_delay_period_cycle;

exit:    
    return rPadDlyRes;
}
#endif

static AUTOK_CMD_TUNE_RES_T 
autok_tune_cmd(
    struct msdc_host *host, 
    E_AUTOK_TUNING_STAGE stg, 
    U_AUTOK_INTERFACE_DATA *pAutoKData,
    P_AUTOK_CYC_SCAN_RES_T pPadDlyRes,
    P_AUTOK_RD_TUNE_RES_T pRDTuneRes)
{
    unsigned int CMDIntPassWin;
    unsigned char fCMDIntDlyScan = 0;
    unsigned int CMDEdgeSel = 0, CMDDlySel = 0;

    unsigned int sel = 0;
    int cnt;
    unsigned int pad_delay_period_cycle = 0;

    #ifdef AUTOK_CMD_TUNE_LEGACY
    unsigned int pad_delay_half_period = 0;
    unsigned int PadDlyCMDRef = 0, CMDIntMargin;
    unsigned int fCMDPadFullRange = 0, CMDPadRange;
    unsigned int CMDScanRangeStg2 = 0, CMDPadScanEdge = 1;
    unsigned int CMDIntDlyStg1, CMDIntDlyStg2;
    unsigned char CMDDlyScanS = 0, CMDDlyScanE = 0, fCMDPadLatchMid = 1;
    unsigned int m;
    #else
    unsigned int x, m;
    unsigned int fCMDEdgeDefault = 1;
    AUTOK_CMDMAT_CHAR_POS CMDMatChar;
    unsigned int CMDMatRaw[SCALE_CMDMAT_RSP_DLY_SEL];
    #endif
    
    S_AUTOK_CMD_DLY data;
    AUTOK_CMD_TUNE_RES_T rCmdTuneRes;
    AUTOK_RAWD_SCAN_T CMDPadScan;

    AUTOK_PRINT("=====autok_tune_cmd=====\r\n");
    
    
    memset(&rCmdTuneRes, 0, sizeof(rCmdTuneRes));
    pad_delay_period_cycle = pPadDlyRes->PadDlyPeriodLen;
    AUTOK_PRINT("CMD tune, Pad delays per 1 cycle: %d\r\n", pad_delay_period_cycle);
    
#ifdef AUTOK_CMD_TUNE_LEGACY
ReTuneCMDPad:
    if (CMDPadScanEdge) {
        sel = 1;
        msdc_autok_adjust_param(host, CMD_EDGE, &sel, MSDC_WRITE);
    }
    else {
        sel = 0;
        msdc_autok_adjust_param(host, CMD_EDGE, &sel, MSDC_WRITE);
    }

    if (stg == TUNING_STG1) {
        
        pad_delay_period_cycle = pPadDlyRes->PadDlyPeriodLen;
        
        
        pAutoKData[E_MSDC_PAD_TUNE_CMDRRDLY].data.sel = 
            autok_ckg_data[pRDTuneRes->CKGenSel].interDelaySel;
        msdc_autok_adjust_param(host, CMD_RESP_RD_DLY, &pAutoKData[E_MSDC_PAD_TUNE_CMDRRDLY].data.sel, MSDC_WRITE);
    
        
        PadDlyCMDRef = pRDTuneRes->PadDlyRefRD;

        
        if (!fCMDPadFullRange)
            CMDPadRange = AUTOK_CMD_SCAN_RANGE_STG1;
        
        else
            CMDPadRange = 32;
        
        if (PadDlyCMDRef > CMDPadRange)
            CMDDlyScanS = PadDlyCMDRef - CMDPadRange;
        else
            CMDDlyScanS = 0;

        if ((PadDlyCMDRef + CMDPadRange) > 31)
            CMDDlyScanE = 31;
        else
            CMDDlyScanE = PadDlyCMDRef + CMDPadRange;
    }
    else if (stg == TUNING_STG2) {
        
        PadDlyCMDRef = pAutoKData[E_MSDC_PAD_TUNE_CMDRDLY].data.sel;
        pad_delay_period_cycle = pAutoKData[E_MSDC_PAD_DLY_PERIOD].data.sel;
        CMDIntMargin = pAutoKData[E_MSDC_CMD_INT_MARGIN].data.sel;
        rCmdTuneRes.CmdPadDlyStg1Bak = PadDlyCMDRef;
        AUTOK_PRINT("CMD tune, Pad delays per 1 cycle: %d\r\n", pad_delay_period_cycle);
        pad_delay_half_period = pad_delay_period_cycle / 2;

        
        if ((CMDIntMargin == AUTOK_SKIP_CMDTUNE_ON_STG2)) {
            AUTOK_PRINT("[WARN] The internal delay is not stable enough, skip the CMD tune!\r\n");
            rCmdTuneRes.fTimingShiftLarge = 1;

            goto exit;
        }

        if (CMDIntMargin) {
            CMDScanRangeStg2 = CMDIntMargin - 5;
            AUTOK_PRINT("[WARN] The internal delay may NOT be stable enough! Reduce the range.\r\n");
        }
        else
            CMDScanRangeStg2 = pad_delay_half_period/2; 

        if (PadDlyCMDRef > CMDScanRangeStg2)
            CMDDlyScanS = PadDlyCMDRef - CMDScanRangeStg2;
        else
            CMDDlyScanS = 0;

        if ((PadDlyCMDRef + CMDScanRangeStg2) > 31)
            CMDDlyScanE = 31;
        else
            CMDDlyScanE = PadDlyCMDRef + CMDScanRangeStg2;

        CMDIntDlyStg1 = pAutoKData[E_MSDC_PAD_TUNE_CMDRRDLY].data.sel;
        CMDIntDlyStg2 = CMDIntDlyStg1;
        if (CMDPadScanEdge) {
            if (pad_delay_half_period <= 16) {
                if (CMDIntDlyStg1 >= pad_delay_half_period)
                    CMDIntDlyStg1 = CMDIntDlyStg1 - pad_delay_half_period;
                else
                    CMDIntDlyStg1 = CMDIntDlyStg1 + pad_delay_half_period;
            }
            else {
                if (CMDIntDlyStg1 >= pad_delay_half_period)
                    CMDIntDlyStg1 = CMDIntDlyStg1 - pad_delay_half_period;
                else if (CMDIntDlyStg1 < (32 - pad_delay_half_period))
                    CMDIntDlyStg1 = CMDIntDlyStg1 + pad_delay_half_period;
                else {
                    if (CMDIntDlyStg1 >= 16)
                        CMDIntDlyStg1 = 0;
                    else
                        CMDIntDlyStg1 = 31;
                }
            }
        }
        msdc_autok_adjust_param(host, CMD_RESP_RD_DLY, &CMDIntDlyStg1, MSDC_WRITE);
        AUTOK_PRINT("Before scan CMD pad delay, re-select CMD internal delay to %d(%d)\r\n",
            CMDIntDlyStg1, CMDIntDlyStg2);
    }
    
    
    AUTOK_PRINT("CMD pad delay scan range: [%d, %d]\r\n", CMDDlyScanS, CMDDlyScanE);
    data.raw_data = 0;
    for (m = CMDDlyScanS; m < CMDDlyScanE; m++) {
        msdc_autok_adjust_param(host, CMD_RD_DLY, &m, MSDC_WRITE);
        for (cnt = 0; cnt < (AUTOK_CMD_TIMES * 5); cnt++) {
            if (autok_cmd_test(host) != E_RESULT_PASS) {
                
                data.raw_data |= (1 << m);
                break;
            }
        }
    }

    memset(&CMDPadScan, 0, sizeof(CMDPadScan));
    CMDPadScan.RawData = data.raw_data;
    data.score = autok_simple_score(CMDPadScan.RawData);
    AUTOK_PRINT("CMD delay scan result: \r\n");
    AUTOK_PRINT("%s edge %s score=%d\r\n", CMDPadScanEdge ? "Falling" : "Rising", 
        g_tune_result_str, data.score);
    
    autok_check_rawd_style(&CMDPadScan, 0);

    
    if (stg == TUNING_STG1) {
        
        if (CMDPadScan.Reg1Cnt && (!CMDPadScan.BoundReg2_S) && !fCMDPadFullRange) {
            CMDDlySel = CMDPadScan.BoundReg1_S + 
                            (CMDPadScan.BoundReg1_E - CMDPadScan.BoundReg1_S) / 2;
        }
        
        else if (CMDPadScan.Reg1Cnt && fCMDPadFullRange) {
            CMDDlySel = CMDPadScan.BoundReg1_S + 
                            (CMDPadScan.BoundReg1_E - CMDPadScan.BoundReg1_S) / 2;
        }
        else if (!CMDPadScan.Reg1Cnt && !CMDPadScan.BoundReg2_S &&
                    (data.score == 32) && !fCMDPadFullRange) {
            AUTOK_PRINT("[WARN] Can NOT find the best CMD pad delay, the timing between CMD & RDAT differs too much!\r\n");
            #if 0
            if ((PadDlyCMDRef < (AUTOK_CMD_SCAN_RANGE_STG1/2)) ||
                ((31 - PadDlyCMDRef) < (AUTOK_CMD_SCAN_RANGE_STG1/2))) {
                CMDDlySel = PadDlyCMDRef;
                fCMDPadLatchMid = 0;
            } 
            else {
                AUTOK_PRINT("CMD pad tune should NOT goto here!\r\n");
                AUTOK_ERR();
            }
            
            #else

            
            fCMDPadFullRange = 1;
            AUTOK_PRINT("[NOTICE]Try again for full range!\r\n");
            goto ReTuneCMDPad;
            
            #endif
        }
        
        else if (!CMDPadScan.Reg1Cnt && !CMDPadScan.BoundReg2_S &&
                    (data.score == 32) && fCMDPadFullRange) {

            AUTOK_PRINT("[WARN]Still can NOT find the best CMD pad delay! Just select the middle score!\r\n");
            CMDEdgeSel = 1;
            CMDDlySel = 15;
        }
        else if (CMDPadScan.Reg2Cnt && ((CMDPadScan.BoundReg2_E - CMDPadScan.BoundReg1_S) <= 6)) {
            AUTOK_PRINT("[WARN] Find holes in fail region when scan CMD pad delay!\r\n");

            CMDDlySel = CMDPadScan.BoundReg1_S + 
                            (CMDPadScan.BoundReg2_E - CMDPadScan.BoundReg1_S) / 2;
        }
        else {
            AUTOK_PRINT("[ERR] Undefined scenario when scan CMD pad delay!!!\r\n");
            AUTOK_ERR();
        }

        AUTOK_PRINT("CMD pad delay tune done: CMDDlySet=%d, CMDDlyRefRDAT=%d\r\n", 
            CMDDlySel, PadDlyCMDRef);

    }
    else if (stg == TUNING_STG2) {
        
        if (CMDPadScan.Reg1Cnt && (!CMDPadScan.BoundReg2_S)) {
            if (CMDPadScanEdge) {
                CMDDlySel = CMDPadScan.BoundReg1_S + 
                                (CMDPadScan.BoundReg1_E - CMDPadScan.BoundReg1_S) / 2;
            }
            else {
                CMDIntPassWin = pad_delay_period_cycle - CMDPadScan.Reg1Cnt;
                CMDIntPassWin /= 2;

                if (CMDPadScan.Reg1Cnt && !CMDPadScan.BoundReg1_E && CMDPadScan.BoundReg1_S)
                    CMDPadScan.BoundReg1_E = 31;

                
                if (CMDIntPassWin >= 32) {
                    
                    if ((31 - CMDPadScan.BoundReg1_E) == CMDPadScan.BoundReg1_S)
                        sel = 0;
                    else if ((31 - CMDPadScan.BoundReg1_E) > CMDPadScan.BoundReg1_S) {
                        if ((31 - CMDPadScan.BoundReg1_E - CMDPadScan.BoundReg1_S) >
                            AUTOK_TUNING_INACCURACY)
                            sel = 31;
                        else
                            sel = 0;
                    }
                    else if ((31 - CMDPadScan.BoundReg1_E) <= CMDPadScan.BoundReg1_S)
                        sel = 0;
                }
                else if ((CMDIntPassWin > 16) && (CMDIntPassWin < 32)) {
                    
                    if (CMDPadScan.BoundReg1_E < (32 - CMDIntPassWin)) {
                        CMDDlySel = CMDPadScan.BoundReg1_E + CMDIntPassWin - 1;
                    }
                    else if (CMDPadScan.BoundReg1_S > CMDIntPassWin) {
                        CMDDlySel = CMDPadScan.BoundReg1_S - CMDIntPassWin;
                    }
                    
                    else if ((CMDPadScan.BoundReg1_E >= (32 - CMDIntPassWin)) &&
                            (CMDPadScan.BoundReg1_S <= CMDIntPassWin)) {

                        
                        if ((31 - CMDPadScan.BoundReg1_E) == CMDPadScan.BoundReg1_S)
                            CMDDlySel = 0;
                        else if ((31 - CMDPadScan.BoundReg1_E) > CMDPadScan.BoundReg1_S) {
                            if ((31 - CMDPadScan.BoundReg1_E - CMDPadScan.BoundReg1_S) >
                                AUTOK_TUNING_INACCURACY)
                                CMDDlySel = 31;
                            else
                                CMDDlySel = 0;
                        }
                        else if ((31 - CMDPadScan.BoundReg1_E) <= CMDPadScan.BoundReg1_S)
                            CMDDlySel = 0;
                        }
                    else
                        AUTOK_PRINT("[ERR]Should NOT goto here in CMD pad scan!!!\r\n");
                }
                else {
                    if (CMDPadScan.BoundReg1_E < CMDIntPassWin) {
                        CMDDlySel = CMDPadScan.BoundReg1_E + CMDIntPassWin;
                    }
                    else if (CMDPadScan.BoundReg1_S > CMDIntPassWin) {
                        CMDDlySel = CMDPadScan.BoundReg1_S - CMDIntPassWin;
                    }
                    else {
                        if ((CMDPadScan.BoundReg1_E + CMDIntPassWin) >= 32)
                            sel = 0;
                        else
                            sel = CMDPadScan.BoundReg1_E + CMDIntPassWin;
                    }
                }
            }
                            
        }
        else if ((!CMDPadScan.Reg1Cnt) && (!CMDPadScan.BoundReg2_S) &&
                    (data.score == 32) && CMDPadScanEdge) {
            AUTOK_PRINT("[WARN] Temperature shift on stage2 exceeds our expectation! Scan rising edge again!\r\n");

            CMDPadScanEdge = 0;
            goto ReTuneCMDPad;
        }
        
        else if ((data.score == 32) &&(((PadDlyCMDRef - CMDDlyScanS) < CMDScanRangeStg2) ||
                ((CMDDlyScanE - PadDlyCMDRef) < CMDScanRangeStg2))) {
            AUTOK_PRINT("[WARN] Can NOT find the suitable CMD pad delay! the temp shift is too much!!!\r\n");
            CMDDlySel = PadDlyCMDRef;
            rCmdTuneRes.fTimingShiftLarge = 1;
        }
        else {
            AUTOK_PRINT("[ERR] Undefined scenario when scan CMD pad delay!!!\r\n");
            AUTOK_ERR();
        }
        
        AUTOK_PRINT("CMD pad delay tune done: CMDDlySet=%d, CMDDlySTG1=%d\r\n", 
            CMDDlySel, PadDlyCMDRef);

    }

    
    rCmdTuneRes.CmdPadDly = CMDDlySel;
    pAutoKData[E_MSDC_PAD_TUNE_CMDRDLY].data.sel = CMDDlySel;
    msdc_autok_adjust_param(host, CMD_RD_DLY, &pAutoKData[E_MSDC_PAD_TUNE_CMDRDLY].data.sel, MSDC_WRITE);
    
    
    pAutoKData[E_MSDC_IOCON_RSPL].data.sel = CMDEdgeSel;
    msdc_autok_adjust_param(host, CMD_EDGE, &pAutoKData[E_MSDC_IOCON_RSPL].data.sel, MSDC_WRITE);

#else   

reTuneCMDMatrix:

    memset(CMDMatRaw, 0, sizeof(CMDMatRaw));

    
    if (fCMDEdgeDefault) 
        sel = AUTOK_CMD_EDGE_MATRIX_SCAN & 0x01;
    else
        sel = ~AUTOK_CMD_EDGE_MATRIX_SCAN & 0x01;
        
    pAutoKData[E_MSDC_IOCON_RSPL].data.sel = sel;
    msdc_autok_adjust_param(host, CMD_EDGE, &sel, MSDC_WRITE);

    AUTOK_PRINT("Start to scan CMD matrix(%s edge)...\r\n", 
        sel ? "falling" : "rising");

    
    AUTOK_PRINT("CKGEN_MSDC_DLY \t PAD_TUNE_CMDRRDLY \t PAD_TUNE_CMDRDLY \r\n");
    for (x=0; x<SCALE_CMDMAT_RSP_DLY_SEL; x++){
        msdc_autok_adjust_param(host, CMD_RESP_RD_DLY, &x, MSDC_WRITE);
            
        for (m = 0; m < SCALE_PAD_TUNE_CMDRDLY; m++) {
            msdc_autok_adjust_param(host, CMD_RD_DLY, &m, MSDC_WRITE);
            for (cnt = 0; cnt < AUTOK_CMD_TIMES; cnt++) {
                if (autok_cmd_test(host) != E_RESULT_PASS) {
                    
                    CMDMatRaw[x] |= (1 << m);
                    break;
                }
            }
        }
        
        autok_simple_score(CMDMatRaw[x]);
        AUTOK_PRINT("%02d \t %02d \t %s\r\n", pRDTuneRes->CKGenSel,
            x, g_tune_result_str);

    }    

    
    memset(&CMDMatChar, 0, sizeof(CMDMatChar));
    autok_check_cmd_matrix(CMDMatRaw, SCALE_PAD_TUNE_CMDRDLY, 
        SCALE_CMDMAT_RSP_DLY_SEL, &CMDMatChar);

    
    if (CMDMatChar.pad_trans_cnt1) {

        
        CMDDlySel = CMDMatChar.pad_trans_s1 + CMDMatChar.pad_trans_cnt1/2;
        rCmdTuneRes.CmdPadDly = CMDDlySel;
        
        
        if (fCMDEdgeDefault)
            CMDEdgeSel = ~AUTOK_CMD_EDGE_MATRIX_SCAN & 0x01;
        else
            CMDEdgeSel = AUTOK_CMD_EDGE_MATRIX_SCAN & 0x01;
    }
    
    else {
        
        if (fCMDEdgeDefault) {
            fCMDEdgeDefault = 0;
            AUTOK_PRINT("Swtich to another edge...\r\n");
            goto reTuneCMDMatrix;
        }
        
        else {
            AUTOK_PRINT("can NOT find the transition boundary\r\n");

            CMDDlySel = 15;
            msdc_autok_adjust_param(host, CMD_EDGE, &CMDEdgeSel, MSDC_READ);
        }
    }

    
    pAutoKData[E_MSDC_PAD_TUNE_CMDRDLY].data.sel = CMDDlySel;
    msdc_autok_adjust_param(host, CMD_RD_DLY, &CMDDlySel, MSDC_WRITE);

    
    pAutoKData[E_MSDC_IOCON_RSPL].data.sel = CMDEdgeSel;
    msdc_autok_adjust_param(host, CMD_EDGE, &CMDEdgeSel, MSDC_WRITE);
    
    AUTOK_PRINT("CMD pad delay tune done: CMDDlySet=%d, CMDEdgeSet=%s\r\n", 
        CMDDlySel, CMDEdgeSel ? "falling" : "rising");
    
#endif

    if (stg == TUNING_STG1) {

    #ifdef AUTOK_CMD_TUNE_LEGACY
        if (((pAutoKData[E_MSDC_PAD_TUNE_CMDRRDLY].data.sel == 0) && pPadDlyRes->fCMDIntDlyConf) || 
            !fCMDPadLatchMid || !pPadDlyRes->fCMDIntDlyConf) 

            fCMDIntDlyScan = 1;
    #else
        fCMDIntDlyScan = 1;
    #endif
    }
    else if (stg == TUNING_STG2)
        fCMDIntDlyScan = 1;

    if (fCMDIntDlyScan) {
        data.raw_data = 0;

        
        for (m=0; m<SCALE_CMD_RSP_DLY_SEL; m++) {
            msdc_autok_adjust_param(host, CMD_RESP_RD_DLY, &m, MSDC_WRITE);
            for (cnt = 0; cnt < (AUTOK_CMD_TIMES * 5); cnt++) {
                if (autok_cmd_test(host) != E_RESULT_PASS) {
                    
                    data.raw_data |= (1 << m);
                    break;
                }
            }
        }
        
        memset(&CMDPadScan, 0, sizeof(CMDPadScan));
        CMDPadScan.RawData = data.raw_data;
        data.score = autok_simple_score(CMDPadScan.RawData);
        AUTOK_PRINT("Int CMD scan %s, score=%d\r\n", g_tune_result_str, data.score);
        autok_check_rawd_style(&CMDPadScan, 0);

        if (data.score == 32) {
            sel = 15;
            pAutoKData[E_MSDC_CMD_INT_MARGIN].data.sel = 
                AUTOK_SKIP_CMDTUNE_ON_STG2;
        }
        else {
            
            CMDIntPassWin = pad_delay_period_cycle - CMDPadScan.Reg1Cnt;
            CMDIntPassWin /= 2;

            if (CMDPadScan.Reg1Cnt && !CMDPadScan.BoundReg1_E && CMDPadScan.BoundReg1_S)
                CMDPadScan.BoundReg1_E = 31;

            
            if (CMDIntPassWin >= 32) {
                
                if ((31 - CMDPadScan.BoundReg1_E) == CMDPadScan.BoundReg1_S)
                    sel = 0;
                else if ((31 - CMDPadScan.BoundReg1_E) > CMDPadScan.BoundReg1_S) {
                    if ((31 - CMDPadScan.BoundReg1_E - CMDPadScan.BoundReg1_S) >
                        AUTOK_TUNING_INACCURACY)
                        sel = 31;
                    else
                        sel = 0;
                }
                else if ((31 - CMDPadScan.BoundReg1_E) <= CMDPadScan.BoundReg1_S)
                    sel = 0;
            }
            else if ((CMDIntPassWin > 16) && (CMDIntPassWin < 32)) {
                
                if (CMDPadScan.BoundReg1_E < (32 - CMDIntPassWin)) {
                    sel = CMDPadScan.BoundReg1_E + CMDIntPassWin - 1;
                }
                else if (CMDPadScan.BoundReg1_S > CMDIntPassWin) {
                    sel = CMDPadScan.BoundReg1_S - CMDIntPassWin;
                }
                
                else if ((CMDPadScan.BoundReg1_E >= (32 - CMDIntPassWin)) &&
                        (CMDPadScan.BoundReg1_S <= CMDIntPassWin)) {

                    AUTOK_PRINT("[NOTICE]It is NOT the best margin(CMD_INT) but try best\r\n");

                    
                    if ((31 - CMDPadScan.BoundReg1_E) == CMDPadScan.BoundReg1_S)
                        sel = 0;
                    else if ((31 - CMDPadScan.BoundReg1_E) > CMDPadScan.BoundReg1_S) {
                        if ((31 - CMDPadScan.BoundReg1_E - CMDPadScan.BoundReg1_S) >
                            AUTOK_TUNING_INACCURACY)
                            sel = 31;
                        else
                            sel = 0;
                    }
                    else if ((31 - CMDPadScan.BoundReg1_E) <= CMDPadScan.BoundReg1_S)
                        sel = 0;

                    if (!sel)
                        pAutoKData[E_MSDC_CMD_INT_MARGIN].data.sel = CMDPadScan.BoundReg1_S;
                    else
                        pAutoKData[E_MSDC_CMD_INT_MARGIN].data.sel = 31 - CMDPadScan.BoundReg1_E;
                }
                else {
                    AUTOK_PRINT("[ERR]Should NOT goto here in CMD internal scan!!!\r\n");
                    AUTOK_ERR();
                }
            }
            else {
                if (CMDPadScan.BoundReg1_E < CMDIntPassWin) {
                    sel = CMDPadScan.BoundReg1_E + CMDIntPassWin;
                }
                else if (CMDPadScan.BoundReg1_S > CMDIntPassWin) {
                    sel = CMDPadScan.BoundReg1_S - CMDIntPassWin;
                }
                else {
                    if ((CMDPadScan.BoundReg1_E + CMDIntPassWin) >= 32)
                        sel = 0;
                    else
                        sel = CMDPadScan.BoundReg1_E + CMDIntPassWin;

                    pAutoKData[E_MSDC_CMD_INT_MARGIN].data.sel = 
                        CMDPadScan.BoundReg1_S;
                }
            }
        }

        rCmdTuneRes.CmdIntDly = sel;
        pAutoKData[E_MSDC_PAD_TUNE_CMDRRDLY].data.sel = sel;
        msdc_autok_adjust_param(host, CMD_RESP_RD_DLY, &pAutoKData[E_MSDC_PAD_TUNE_CMDRRDLY].data.sel, MSDC_WRITE);
        
    } else {
        rCmdTuneRes.CmdIntDly = pAutoKData[E_MSDC_PAD_TUNE_CMDRRDLY].data.sel;
        AUTOK_PRINT("CMD internal delay(%d) is determined by former scan\r\n", rCmdTuneRes.CmdIntDly);
    }

    AUTOK_PRINT("CMD Internal delay tune done: %d\r\n", rCmdTuneRes.CmdIntDly);
    rCmdTuneRes.fRetOk = 1;

#if defined(AUTOK_CMD_TUNE_LEGACY) || !defined(AUTOK_DEBUG)
exit:
#endif

    return rCmdTuneRes;
}

static AUTOK_RD_TUNE_RES_T 
autok_tune_rd(
    struct msdc_host *host, 
    E_AUTOK_TUNING_STAGE stg, 
    U_AUTOK_INTERFACE_DATA *pAutoKData,
    P_AUTOK_CYC_SCAN_RES_T pPadDlyRes,
    P_AUTOK_CMD_TUNE_RES_T pCmdTuneRes)
{
    int ck, CKLastLBound = 0, bound_diff;
    unsigned int LeftBoundCnt = 0, CKGENLeftBound[4];
    unsigned int FBoundFound = 0, CKGENFBound[2];
    unsigned int LBoundPad = 0, LBoundCKGEN = 0;
    unsigned int FBoundCnt = 0, FBoundCKGEN = 0, FBoundLMargin = 0, FBoundLDeMar = 0;
    unsigned int IntBound = 0;
    unsigned int DataPassWin, DataMargin;
    unsigned int RealCKGEN, RealPadDelay;
    unsigned int RBoundPad = 0;
    unsigned int RDlyRefCMD, RDlyRefSTG1;
    unsigned int baseCKGEN = 0;
    unsigned int max_LBoundCnt = 0, LDeMarMin;
    unsigned int fUnEnoughRightMar = 0;
    #if defined(MT6582LTE)
    unsigned int TinyMar = 0;
    #endif
    
    unsigned int m,x;
    unsigned int sel=0;
    int reTuneCmd = 0;
    unsigned int pad_delay_period_cycle = 0;
    unsigned int range_max;
    unsigned int CKGENMax = AUTOK_CKGEN_ALLOW_MAX;
    
    S_AUTOK_CMD_DLY data;
    E_RESULT_TYPE res;
    AUTOK_RD_TUNE_RES_T rRdTuneRes;

    AUTOK_PRINT("=====autok_tune_rd=====\r\n");

    
    memset(&rRdTuneRes, 0, sizeof(rRdTuneRes));
    memset(CKGENFBound, 0, sizeof(CKGENFBound));

    #ifdef AUTOK_RDAT_ACC
        
        if (!pPadDlyRes->fHoleCK)
            pPadDlyRes->HoleCK = pPadDlyRes->CKGenPeriodLen;

        if (pPadDlyRes->fFBound) {
            range_max = MAX_GET(pPadDlyRes->FBoundCK, pPadDlyRes->HoleCK);
            FBoundFound = 1;
            CKGENFBound[0] = pPadDlyRes->FBoundCK;        
        }
        else
            range_max = pPadDlyRes->CKGenPeriodLen;

        
        pPadDlyRes->HoleCK--;
        CKGENMax = MIN_GET(CKGENMax, pPadDlyRes->HoleCK);
        pPadDlyRes->HoleCK++;

        AUTOK_PRINT("MAX allow CKGEN:%d, Hole CKGEN:%d\r\n", CKGENMax, pPadDlyRes->HoleCK);
        AUTOK_PRINT("RDAT pattern %s found(CKGEN=%d)\r\n", 
            pPadDlyRes->fFBound ? "Full Bound" : "Left Bound", 
            pPadDlyRes->fFBound ? pPadDlyRes->FBoundCK : range_max);

    #else
        range_max = pPadDlyRes->CKGenPeriodLen;
    #endif
    
    if (stg == TUNING_STG1) {
        pad_delay_period_cycle = pPadDlyRes->PadDlyPeriodLen;
        AUTOK_PRINT("Read Tune, Pad delays per 1 cycle: %d\r\n", pad_delay_period_cycle);
        AUTOK_PRINT("CKGEN_MSDC_DLY \t PAD_TUNE_DATRDDLY \r\n");
        
        for (ck = range_max; ck >= 0; ck--) {

            autok_ckg_data[range_max - ck].readScore = 
                autok_simple_score(autok_rdata_scan[range_max - ck].RawData);
            AUTOK_PRINT("%02d \t %02d \t %s %s\r\n", range_max - ck, 
                autok_ckg_data[range_max - ck].readScore, g_tune_result_str, 
                autok_rdata_scan[range_max - ck].fInvalidCKGEN ? "(Invalid)" : "");

            if (!autok_rdata_scan[ck].fInvalidCKGEN && !FBoundFound) {
                
                if ((autok_rdata_scan[ck].BoundReg1_S == 0) && autok_rdata_scan[ck].Reg1Cnt) {
                    
                    if (LeftBoundCnt < (sizeof(CKGENLeftBound)/sizeof(unsigned int)))
                        CKGENLeftBound[LeftBoundCnt++] = ck;

                    
                    if (autok_rdata_scan[ck].Reg1Cnt > max_LBoundCnt) {
                        max_LBoundCnt = autok_rdata_scan[ck].Reg1Cnt;
                        CKLastLBound = ck;
                    }

                }

                
                if ((autok_rdata_scan[ck].BoundReg1_S > 0) &&
                    (autok_rdata_scan[ck].BoundReg1_E > 0) &&
                    (autok_rdata_scan[ck].Reg1Cnt > AUTOK_RDAT_FBOUND_TH) &&
                    (FBoundFound < (sizeof(CKGENFBound)/sizeof(unsigned int)))) {

                    if (LeftBoundCnt) {
                        if (ck < CKGENLeftBound[LeftBoundCnt - 1])
                            CKGENFBound[FBoundFound++] = ck;
                    }
                    else
                        CKGENFBound[FBoundFound++] = ck;
                }
            }
            
        } 

        if (LeftBoundCnt) {

            x = 0;

    findLBound:
            
            LBoundPad = autok_rdata_scan[CKGENLeftBound[x]].BoundReg1_E;
            LBoundCKGEN = CKGENLeftBound[x];
            if ((LeftBoundCnt >= 2) && (x < (LeftBoundCnt - 1))) {
                
                if (autok_rdata_scan[CKGENLeftBound[x]].BoundReg1_E > 
                    autok_rdata_scan[CKGENLeftBound[x+1]].BoundReg1_E) {
                    x++;
                    goto findLBound;
                }
                    
                bound_diff = ABS_DIFF(autok_rdata_scan[CKGENLeftBound[x]].BoundReg1_E, 
                    autok_rdata_scan[CKGENLeftBound[x+1]].BoundReg1_E);

                if (AUTOK_TUNING_INACCURACY < ABS_DIFF(bound_diff,
                        (ABS_DIFF(CKGENLeftBound[x], CKGENLeftBound[x+1]) *
                         DIV_CEIL_FUNC(MIN_CLK_GEN_DELAY_IN_PS, MIN_PAD_DELAY_IN_PS)))) {
                    
                    AUTOK_PRINT("[WARN] The left Pad boundary NOT correct! Find again!\r\n");
                    x++;
                    goto findLBound;
                }
            } else {
                AUTOK_PRINT("[WARN] The left Pad boundary may NOT be correct!\r\n");
            }

            
            if ((autok_rdata_scan[CKGENLeftBound[LeftBoundCnt-1]].BoundReg2_S > 0) &&
                (autok_rdata_scan[CKGENLeftBound[LeftBoundCnt-1]].BoundReg2_E == 0))
                IntBound = autok_rdata_scan[CKGENLeftBound[LeftBoundCnt-1]].BoundReg2_S; 
        }
        else {
            AUTOK_PRINT("[WARN] Can NOT find left pad boundary!!!\r\n");
        }

        
        if (FBoundFound) {
            FBoundCKGEN = CKGENFBound[0];
            FBoundCnt = autok_rdata_scan[CKGENFBound[0]].BoundReg1_E - 
                            autok_rdata_scan[CKGENFBound[0]].BoundReg1_S + 1;

            FBoundLMargin = autok_rdata_scan[CKGENFBound[0]].BoundReg1_S;
            
            if (FBoundFound == 2) {
                bound_diff = autok_rdata_scan[CKGENFBound[1]].BoundReg1_E - 
                                autok_rdata_scan[CKGENFBound[1]].BoundReg1_S + 1;

                if (3 < ABS_DIFF(bound_diff, FBoundCnt))
                    AUTOK_PRINT("[WARN] The full Pad boundary may NOT correct!\r\n");
            }

            
            
                if ((autok_rdata_scan[FBoundCKGEN].BoundReg2_S > 0) &&
                    (autok_rdata_scan[FBoundCKGEN].BoundReg2_E == 0))
                    IntBound = autok_rdata_scan[FBoundCKGEN].BoundReg2_S; 
            
        }
        else if (LeftBoundCnt) {
            AUTOK_PRINT("[WARN] Can NOT find full pad boundary directly!!!\r\n");
            
            FBoundCnt = autok_rdata_scan[CKLastLBound].BoundReg1_E - 
                            autok_rdata_scan[CKLastLBound].BoundReg1_S + 1;
            FBoundCKGEN = SCALE_CKGEN_MSDC_DLY_SEL;

            if (IntBound) {
                m = IntBound;
                m /= 4; 
                m += 2;
            }
            else
                
                m = CKLastLBound;

            if (m > CKLastLBound)
                m = CKLastLBound;

            AUTOK_PRINT("Cur. base full boundary count is %d\r\n", FBoundCnt);
            AUTOK_PRINT("Find full boundary in CKGEN range [%d, %d]\r\n", 
                CKLastLBound-1, CKLastLBound-m);

            LDeMarMin = 5;
            for(x = 1; x <= m; x++) {
                if (!autok_rdata_scan[CKLastLBound-x].fInvalidCKGEN) {
                    if (autok_rdata_scan[CKLastLBound-x].BoundReg1_S) {
                        FBoundLDeMar = ABS_DIFF(4*x, autok_rdata_scan[CKLastLBound-x].BoundReg1_S);
                        if (FBoundLDeMar < LDeMarMin)
                            LDeMarMin = FBoundLDeMar;
                    }
                }
            }

            FBoundLMargin = 0;
            FBoundCnt += LDeMarMin;
            FBoundLDeMar = LDeMarMin;
            
            AUTOK_PRINT("[NOTICE] Full boundary is calculated by %d pad delay\r\n", FBoundCnt);
        }

        AUTOK_PRINT("RData pattern found: LBoundCKGEN:%d, LBoundPad:%d, FBoundCKGEN:%d, FBoundCnt:%d, IntBound:%d\r\n",
                LBoundCKGEN, LBoundPad, FBoundCKGEN, FBoundCnt, IntBound);

        if (!FBoundCnt) {
            AUTOK_PRINT("[WARN] Can NOT find neither left and full pad boundary !\r\n");

            
            if (freq_mhz <= 100) {
                FBoundCnt = autok_rdat_fbound_th + 2;
                FBoundCKGEN = pPadDlyRes->CKGenPeriodLen - 1;
                FBoundLMargin = 31;
            }
            else {
                if (pPadDlyRes->fFBoundRefCMD) {
                    FBoundCnt = pPadDlyRes->FBoundCntRefCMD;
                    FBoundCKGEN = pPadDlyRes->FBoundCKRefCMD;
                    FBoundLMargin = pPadDlyRes->FBoundMidRefCMD - FBoundCnt/2;

                    if (autok_rdata_scan[FBoundCKGEN].BoundReg1_S <= FBoundLMargin)
                        IntBound = autok_rdata_scan[FBoundCKGEN].BoundReg1_S;

                    AUTOK_PRINT("Refered from CMD: FBoundCKGEN:%d, FBoundCnt:%d, FBoundLMargin:%d, IntBound:%d\r\n",
                            FBoundCKGEN, FBoundCnt, FBoundLMargin, IntBound);
                }
                else
                    AUTOK_ERR();
            }
        }

 

        
        DataPassWin = pad_delay_period_cycle - FBoundCnt;

        

    ChngBaseCK:
        
        if (!autok_rdata_scan[baseCKGEN].BoundReg1_S && 
            ((autok_rdata_scan[baseCKGEN].BoundReg1_E && (autok_rdata_scan[baseCKGEN].Reg1Cnt > 1)) || 
            (!autok_rdata_scan[baseCKGEN].BoundReg1_E && (autok_rdata_scan[baseCKGEN].Reg1Cnt == 1)))) {

            if (autok_rdata_scan[baseCKGEN].BoundReg2_S)
                DataMargin = (autok_rdata_scan[baseCKGEN].BoundReg2_S - 
                    autok_rdata_scan[baseCKGEN].BoundReg1_E) / 2;
            else
                DataMargin = (31 - autok_rdata_scan[baseCKGEN].BoundReg1_E) / 2;

            AUTOK_PRINT("Left boundary found at initial timing(PWin:%d vs. Margin.Max:%d)\r\n", 
                DataMargin, DataPassWin/2);

            if (DataMargin < (DataPassWin / 2)) {
                RealCKGEN = baseCKGEN + DIV_CEIL_FUNC(((DataPassWin / 2) - DataMargin),
                                DIV_CEIL_FUNC(MIN_CLK_GEN_DELAY_IN_PS, MIN_PAD_DELAY_IN_PS));

                
                if (RealCKGEN == baseCKGEN)
                    RealCKGEN = baseCKGEN + 1;
                
                RealPadDelay = autok_rdata_scan[baseCKGEN].BoundReg1_E + 1 + (DataPassWin / 2) - 
                                    (RealCKGEN - baseCKGEN) * DIV_CEIL_FUNC(MIN_CLK_GEN_DELAY_IN_PS, MIN_PAD_DELAY_IN_PS);
            }
            else {
                RealCKGEN = baseCKGEN;
                RealPadDelay = autok_rdata_scan[baseCKGEN].BoundReg1_E + (DataPassWin / 2);
            }

            
            if (RealCKGEN > CKGENMax) {
                AUTOK_PRINT("[WARN] Exceeds the maximun CKGEN range[%d]\r\n", CKGENMax);
                AUTOK_PRINT("Maybe NOT the best point due to CKGEN limit!\r\n");
                
                RealCKGEN = CKGENMax;
                RealPadDelay = autok_rdata_scan[baseCKGEN].BoundReg1_E + 1 + (DataPassWin / 2) - 
                                    (RealCKGEN - baseCKGEN) * DIV_CEIL_FUNC(MIN_CLK_GEN_DELAY_IN_PS, MIN_PAD_DELAY_IN_PS);
            }
            
            
            rRdTuneRes.PadDlyRefRD = autok_rdata_scan[baseCKGEN].BoundReg1_E + (DataPassWin / 2) -
                                (RealCKGEN - baseCKGEN) * DIV_CEIL_FUNC(MIN_CLK_GEN_DELAY_IN_PS, MIN_PAD_DELAY_IN_PS);
        } 
        else if (!autok_rdata_scan[baseCKGEN].BoundReg1_S && 
                 !autok_rdata_scan[baseCKGEN].BoundReg1_E &&
                 (autok_rdata_scan[baseCKGEN].Reg1Cnt == 32)) {

            AUTOK_PRINT("[WARN] Timing is compressed by too large output delay(CK=%d)\r\n", baseCKGEN);
            baseCKGEN++;

            if (baseCKGEN <= CKGENMax)
                goto ChngBaseCK;
            else {
                AUTOK_PRINT("[ERR] Exceeds the Max. CKGEN limitation!\r\n");
                AUTOK_ERR();
            }   
        }
        
        else {
            if (FBoundCnt && (FBoundCKGEN != SCALE_CKGEN_MSDC_DLY_SEL)) {

                
                if (FBoundCKGEN > 2) {
                    if (FBoundLMargin)
                        RBoundPad = FBoundCKGEN * MIN_CLK_GEN_DELAY_IN_PS/MIN_PAD_DELAY_IN_PS +
                                        FBoundLMargin;
                    else
                        RBoundPad = FBoundCKGEN * MIN_CLK_GEN_DELAY_IN_PS/MIN_PAD_DELAY_IN_PS -
                                        FBoundLDeMar;
                }
                else {
                    if (FBoundLMargin)
                        RBoundPad = FBoundCKGEN * DIV_CEIL_FUNC(MIN_CLK_GEN_DELAY_IN_PS, MIN_PAD_DELAY_IN_PS) +
                                        FBoundLMargin;
                    else
                        RBoundPad = FBoundCKGEN * DIV_CEIL_FUNC(MIN_CLK_GEN_DELAY_IN_PS, MIN_PAD_DELAY_IN_PS) -
                                        FBoundLDeMar;
                }
            }   
            else if (LBoundPad)
                RBoundPad = LBoundCKGEN * DIV_CEIL_FUNC(MIN_CLK_GEN_DELAY_IN_PS, MIN_PAD_DELAY_IN_PS) +
                                LBoundPad - FBoundCnt;

            AUTOK_PRINT("Right boundary found at initial timing(Rbound:%d vs. Margin.Max:%d)\r\n", 
                RBoundPad, DataPassWin/2);

            if (RBoundPad >= (DataPassWin/2)) {
            
                RealCKGEN = (RBoundPad - (DataPassWin/2)) /
                                DIV_CEIL_FUNC(MIN_CLK_GEN_DELAY_IN_PS, MIN_PAD_DELAY_IN_PS);

                RealPadDelay = RBoundPad - (DataPassWin/2) -
                                    RealCKGEN *DIV_CEIL_FUNC(MIN_CLK_GEN_DELAY_IN_PS, MIN_PAD_DELAY_IN_PS);
            }
            
            else {
                int LBound = 0, NewMargin = 0;
                
                AUTOK_PRINT("[WARN] Un-enough margin to right boundary! Check layout!\r\n");


                if (autok_rdata_scan[0].Reg1Cnt && autok_rdata_scan[0].Reg2Cnt) {
                    RBoundPad = autok_rdata_scan[0].BoundReg1_S;
                    LBound = autok_rdata_scan[0].BoundReg2_S - autok_rdata_scan[0].BoundReg1_E - 1;
                }
                else if (autok_rdata_scan[0].Reg1Cnt && !autok_rdata_scan[0].Reg2Cnt) {
                    RBoundPad = autok_rdata_scan[0].BoundReg1_S;

                    if (autok_rdata_scan[0].BoundReg1_E && !IntBound) {
                        LBound = 31 - autok_rdata_scan[0].BoundReg1_E;
                    }
                    else if (!autok_rdata_scan[0].BoundReg1_E && IntBound) {
                        LBound = (autok_rdata_scan[0].Reg1Cnt - (32- IntBound)) - FBoundCnt;
                    }
                    else if (!autok_rdata_scan[0].BoundReg1_E && !IntBound) {
                        LBound = autok_rdata_scan[0].Reg1Cnt - FBoundCnt;
                    }
                    else if (autok_rdata_scan[0].BoundReg1_E && IntBound) {
                        LBound = IntBound - autok_rdata_scan[0].BoundReg1_E - 1;
                    }
                }
                else
                    AUTOK_PRINT("[NOTICE] Might be the low freq case\r\n");

                
                NewMargin = LBound + CKGENMax * 
                                DIV_CEIL_FUNC(MIN_CLK_GEN_DELAY_IN_PS, MIN_PAD_DELAY_IN_PS);

                AUTOK_PRINT("RBound:%d, LBound:%d, CKGEN shift Mar:%d\r\n", 
                    RBoundPad, LBound, NewMargin);

                if ((NewMargin/2 > (int)0) && (NewMargin/2 >= (int)(RBoundPad + 2))) {

                    AUTOK_PRINT("Shifting CKGEN has more margin\r\n");
                    RealCKGEN = CKGENMax;

                    if (!IntBound)
                        IntBound = 32;

                    RealPadDelay = IntBound - NewMargin/2 - 1;
                }
                else {
                    RealCKGEN = 0;
                    RealPadDelay = 0;
                    fUnEnoughRightMar = 1;
                }
            }

            if (RealCKGEN > CKGENMax) {
                AUTOK_PRINT("[WARN] Exceeds the maximun CKGEN range[%d]\r\n", CKGENMax);
                AUTOK_PRINT("Maybe NOT the best point due to CKGEN limit!\r\n");
            
                RealCKGEN = CKGENMax;
                RealPadDelay = RBoundPad - (DataPassWin/2) -
                                    RealCKGEN *DIV_CEIL_FUNC(MIN_CLK_GEN_DELAY_IN_PS, MIN_PAD_DELAY_IN_PS);
            }

            
            rRdTuneRes.PadDlyRefRD = RealPadDelay;
        }

        if (!IntBound) 
            IntBound = 32;
        
        if (autok_rdata_scan[RealCKGEN].Reg1Cnt && 
            !autok_rdata_scan[RealCKGEN].BoundReg1_S) {

            AUTOK_PRINT("Left boundary after shifting CKGEN\r\n");
            AUTOK_PRINT("[WARN] Try to select the middle score!\r\n");
            
            RealPadDelay = autok_rdata_scan[RealCKGEN].BoundReg1_E + 1 +  
                (IntBound - autok_rdata_scan[RealCKGEN].BoundReg1_E - 1)/2;

            #if defined(MT6582LTE)
            TinyMar = (IntBound - autok_rdata_scan[RealCKGEN].BoundReg1_E - 1);
            
            if (gfIOSS && gfCoreTT && (IntBound < 32)) {

                gfTinyMar = 1;
                pAutoKData[E_MSDC_F_TINY_MARGIN].data.sel = AUTOK_TINY_MAR_PAT;

                if (!(TinyMar % 2))
                    RealPadDelay--;

                if (TinyMar > AUTOK_TINY_MAR_TH_BTM) {

                    AUTOK_PRINT("[WARN] Tiny margin(%d), shift the pad delay(%d) to right\r\n", 
                        TinyMar, RealPadDelay);
                    
                    if ((IntBound-(RealPadDelay+AUTOK_TINY_MAR_R_SHIFT_MAX)) >= 
                        (AUTOK_TINY_MAR_R_MAR_MIN + 1))
                        RealPadDelay += AUTOK_TINY_MAR_R_SHIFT_MAX;
                    else {
                        if ((IntBound-(AUTOK_TINY_MAR_R_MAR_MIN+1)) > RealPadDelay)
                            RealPadDelay = IntBound-(AUTOK_TINY_MAR_R_MAR_MIN+1);
                    }
                }
            }
            #endif
        }
        
        else if (!fUnEnoughRightMar) {

            AUTOK_PRINT("Right boundary after shifting CKGEN\r\n");
            
            if ((IntBound < 32) && ((RealPadDelay >= autok_rdata_scan[RealCKGEN].BoundReg1_S) ||
               ((RealPadDelay < autok_rdata_scan[RealCKGEN].BoundReg1_S) &&
               ((autok_rdata_scan[RealCKGEN].BoundReg1_S - RealPadDelay) <= 
                (DataPassWin/2 - AUTOK_TUNING_INACCURACY))))) {

                AUTOK_PRINT("[WARN] The margin may NOT up to T/2 due to internal boundary\r\n");

                if (IntBound > RealPadDelay) {
                    if ((IntBound - RealPadDelay + DataPassWin/2)/2 < IntBound)
                        RealPadDelay = IntBound - (IntBound - RealPadDelay + DataPassWin/2)/2;
                    else
                        RealPadDelay = 0;
                }
                else {
                    AUTOK_PRINT("[WARN] The internal boundary is too large!\r\n");

                    if ((DataPassWin/2 + IntBound - RealPadDelay)/2 < IntBound)
                        RealPadDelay = IntBound - (DataPassWin/2 + IntBound - RealPadDelay)/2;
                    else
                        RealPadDelay = 0;
                }
                
                AUTOK_PRINT("Try to select the best margin(sacrifying the left margin)\r\n");
            }
            else if (IntBound == 32) {

                
                if (autok_rdata_scan[RealCKGEN].Reg1Cnt && 
                    autok_rdata_scan[RealCKGEN].BoundReg1_S) {

                    if (RealPadDelay >= 
                        (autok_rdata_scan[RealCKGEN].BoundReg1_S-1)/2)
                        RealPadDelay = (autok_rdata_scan[RealCKGEN].BoundReg1_S-1)/2;
                }
                
                else {
                    if (RealPadDelay >= 16)
                        RealPadDelay = 16;
                }
            }
        }

        if (IntBound == 32)
            IntBound = 0;

        
        if (((RealPadDelay >= IntBound) && IntBound) || (RealPadDelay >= 32)) {
            AUTOK_PRINT("[ERR] Invalid pad delay(%d): exceeds the range\r\n", RealPadDelay);
            AUTOK_ERR();
        }
        if (autok_rdata_scan[RealCKGEN].RawData & (unsigned int)(1 << RealPadDelay)) {
            AUTOK_PRINT("[ERR] Invalid pad delay(%d): NOT at the pass window\r\n", RealPadDelay);
            AUTOK_ERR();
        }

        pAutoKData[E_MSDC_PAD_TUNE_DATRRDLY].data.sel = RealPadDelay;
        autok_ckg_data[RealCKGEN].readPadSel = pAutoKData[E_MSDC_PAD_TUNE_DATRRDLY].data.sel;
        sel = RealCKGEN;
        rRdTuneRes.CKGenSel = RealCKGEN;
        AUTOK_PRINT("Read Data tune done: CKGEN=%d, Read pad=%d\r\n", sel, 
            autok_ckg_data[RealCKGEN].readPadSel);
        
        goto APPLYSET;
    }
    else if (stg == TUNING_STG2) {
    
        data.raw_data = 0;
        reTuneCmd = 0;
        x = 0;
        
        while(x < SCALE_PAD_TUNE_DATRDDLY && reTuneCmd == 0) {
            msdc_autok_adjust_param(host, DAT_RD_DLY, &x, MSDC_WRITE);

            for (m = 0; m < AUTOK_RDAT_TIMES; m++) {
                if ((res = autok_read_test(host)) != E_RESULT_PASS) {
                    data.raw_data|= (1 << x);
                    
                    if (autok_recovery(host)) {
                        AUTOK_PRINT("===tune read : error, fail to bring to tranfer status===\r\n");
                        goto exit;
                    }
                    if (res == E_RESULT_CMD_CRC) {
                        AUTOK_PRINT("[WARN] CMD CRC error in tuning read[%d %d], need to tune command again!!\r\n",x,m);
                        reTuneCmd = 1;
                    }
                    break;
                }
            }
            
            x++;
        }

        if(reTuneCmd == 1) {
            AUTOK_PRINT("[ERR] CMD CRC error in tuning read!\r\n");
            AUTOK_ERR();
        }

        memset(&autok_rdata_scan[0], 0, sizeof(AUTOK_RAWD_SCAN_T));
        autok_rdata_scan[0].RawData = data.raw_data;
        autok_select_range(data.raw_data, &sel);
        data.score = autok_simple_score(data.raw_data);
        AUTOK_PRINT("Read data scan %s, score=%d\r\n", g_tune_result_str, data.score);
        
        autok_check_rawd_style(&autok_rdata_scan[0], 1);

        RDlyRefSTG1 = pAutoKData[E_MSDC_PAD_TUNE_DATRRDLY].data.sel;
        RDlyRefCMD = sel;
        if (pCmdTuneRes->CmdPadDlyStg1Bak && !pCmdTuneRes->fTimingShiftLarge) {
            if (pCmdTuneRes->CmdPadDly >= pCmdTuneRes->CmdPadDlyStg1Bak) {
                RDlyRefCMD = RDlyRefSTG1 + ABS_DIFF(pCmdTuneRes->CmdPadDly, pCmdTuneRes->CmdPadDlyStg1Bak);
            }
            else {
                RDlyRefCMD = RDlyRefSTG1 - ABS_DIFF(pCmdTuneRes->CmdPadDly, pCmdTuneRes->CmdPadDlyStg1Bak);
            }

            AUTOK_PRINT("Read data pad delay Refered from CMD: %d\r\n", RDlyRefCMD);

            if (!autok_rdata_scan[0].fInvalidCKGEN) {
                if ((RDlyRefCMD >= sel) && 
                    ((autok_rdata_scan[0].Reg2Cnt && !autok_rdata_scan[0].BoundReg2_E) ||
                    (autok_rdata_scan[0].Reg1Cnt && !autok_rdata_scan[0].BoundReg1_E))) 
                    RDlyRefCMD = sel;
            }
        }
        
        else {
            AUTOK_PRINT("[WARN]Read data margin is NOT enough! Check layout!!!\r\n");
            
            if (!autok_rdata_scan[0].fInvalidCKGEN) {
                if (!RDlyRefSTG1) {
                    if (autok_rdata_scan[0].BoundReg1_S * 2 >= data.score)
                        RDlyRefCMD = RDlyRefSTG1;
                    else
                        RDlyRefCMD = sel;
                }
                else if (RDlyRefSTG1 < (autok_rdata_scan[0].BoundReg1_S/2)) {
                    RDlyRefCMD = RDlyRefSTG1;
                }
            }
            else
                RDlyRefCMD = sel;        
        }
        
        AUTOK_PRINT("Read data pad delay tune done: %d\r\n", RDlyRefCMD);

        pAutoKData[E_MSDC_PAD_TUNE_DATRRDLY].data.sel = RDlyRefCMD;
    }

APPLYSET:
    if (stg == TUNING_STG1) {
        
        pAutoKData[E_MSDC_CKGEN_MSDC_DLY_SEL].data.sel = sel;
        msdc_autok_adjust_param(host, CKGEN_MSDC_DLY_SEL, &pAutoKData[E_MSDC_CKGEN_MSDC_DLY_SEL].data.sel, MSDC_WRITE);

        
        pAutoKData[E_MSDC_PAD_TUNE_DATRRDLY].data.sel = autok_ckg_data[sel].readPadSel;
        msdc_autok_adjust_param(host, DAT_RD_DLY, &pAutoKData[E_MSDC_PAD_TUNE_DATRRDLY].data.sel, MSDC_WRITE);
    }
    else if (stg == TUNING_STG2) {
        msdc_autok_adjust_param(host, DAT_RD_DLY, &pAutoKData[E_MSDC_PAD_TUNE_DATRRDLY].data.sel, MSDC_WRITE);
    }

    rRdTuneRes.fRetOk = 1;
    
exit:    
    return rRdTuneRes;
}

static E_RESULT_TYPE 
autok_doub_chk_wdat_int_bound(struct msdc_host *host, unsigned int *pRaw)
{
    #define AUTOK_WDAT_DOUBCHK_WIN      (5)

    unsigned int ckgen = 0, rdat_sel = 0;
    unsigned int raw_data, chk_win = 0, chk_win_ex = 0, temp, idx;
    unsigned int m,x;
    int reTuneCmd = 0;
    
    unsigned int w_data;
    AUTOK_RAWD_SCAN_T rDatPattern;
    E_RESULT_TYPE res = E_RESULT_ERR;

    AUTOK_PRINT("Double check for WDAT internal boundary\r\n");

    
    *pRaw = 0;

    
    msdc_autok_adjust_param(host, CKGEN_MSDC_DLY_SEL, &ckgen, MSDC_READ);
    msdc_autok_adjust_param(host, DAT_RD_DLY, &rdat_sel, MSDC_READ);
    raw_data = autok_rdata_scan[ckgen].RawData;

    
    chk_win = 1 << rdat_sel;
    temp = chk_win = chk_win << (AUTOK_WDAT_DOUBCHK_WIN/2);
    for (idx = 0; idx < AUTOK_WDAT_DOUBCHK_WIN; idx++)
        chk_win |= (temp >> idx);
    
    memset(&rDatPattern, 0, sizeof(rDatPattern));
    rDatPattern.RawData = chk_win;
    autok_check_rawd_style(&rDatPattern, 0);
    if (rDatPattern.Reg1Cnt < AUTOK_WDAT_DOUBCHK_WIN) {
        AUTOK_PRINT("Make up %d gears\r\n", AUTOK_WDAT_DOUBCHK_WIN - rDatPattern.Reg1Cnt);
        
        if (!rDatPattern.BoundReg1_S) {
            chk_win |= (chk_win << (AUTOK_WDAT_DOUBCHK_WIN - rDatPattern.Reg1Cnt));
        }
        else if (rDatPattern.BoundReg1_S && !rDatPattern.BoundReg1_E) {
            chk_win |= (chk_win >> (AUTOK_WDAT_DOUBCHK_WIN - rDatPattern.Reg1Cnt));
        }
    }
    
    chk_win_ex = temp = chk_win;
    chk_win_ex |= (temp << 1);
    chk_win_ex |= (temp >> 1);

    AUTOK_PRINT("CKGEN:%d, RDAT_SEL:%d, RAW:0x%X, CHK_WIN:0x%X, Ext.CHK_WIN:0x%X\r\n", 
        ckgen, rdat_sel, raw_data, chk_win, chk_win_ex);

    
    temp = raw_data & chk_win_ex;
    if (temp) {
        memset(&rDatPattern, 0, sizeof(rDatPattern));
        rDatPattern.RawData = temp;
        autok_check_rawd_style(&rDatPattern, 0);

        
        if (rDatPattern.Reg1Cnt && !rDatPattern.Reg2Cnt) {
            
            if (rDatPattern.BoundReg1_S >= rdat_sel) {
                temp |= (temp >> 1);
                chk_win_ex ^= temp;

                
                chk_win = chk_win_ex;
                chk_win_ex |= (chk_win_ex >> 1);
                temp = raw_data & chk_win_ex;
                if (temp)
                    chk_win &= (chk_win << 1);
            }
            
            else {
                temp |= (temp << 1);
                chk_win_ex ^= temp;

                
                chk_win = chk_win_ex;
                chk_win_ex |= (chk_win_ex << 1);
                temp = raw_data & chk_win_ex;
                if (temp)
                    chk_win &= (chk_win >> 1);
            }
        }
        
        else if (rDatPattern.Reg1Cnt && rDatPattern.Reg2Cnt) {
            chk_win = chk_win_ex ^ temp;
            temp = chk_win;
            chk_win &= (temp >> 1);
            chk_win &= (temp << 1);
        }
    }

    memset(&rDatPattern, 0, sizeof(rDatPattern));
    rDatPattern.RawData = chk_win;
    autok_check_rawd_style(&rDatPattern, 0);
    autok_simple_score(chk_win);
    AUTOK_PRINT("Check window: %s [%d, %d]\r\n", 
        g_tune_result_str, rDatPattern.BoundReg1_S, 
        rDatPattern.BoundReg1_E ? rDatPattern.BoundReg1_E : 31);

    
    for (idx = rDatPattern.BoundReg1_S; 
         idx <= (rDatPattern.BoundReg1_E ? rDatPattern.BoundReg1_E : 31); idx++) {

        w_data = 0;
        reTuneCmd = 0;
        x = 0;
        
        
        msdc_autok_adjust_param(host, DAT_RD_DLY, &idx, MSDC_WRITE);
        
        while ( x < SCALE_PAD_TUNE_DATWRDLY && reTuneCmd == 0) {
            msdc_autok_adjust_param(host, DAT_WRD_DLY, &x, MSDC_WRITE);
            for (m = 0; m < (AUTOK_WDAT_TIMES); m++) {
                if ((res = autok_write_test(host)) != E_RESULT_PASS) {
                    w_data |= (1 << x);
                    
                    if (autok_recovery(host)) {
                        AUTOK_PRINT("===tune write : error, fail to bring to tranfer status===\r\n");
                        goto exit;
                    }
                    if (res == E_RESULT_CMD_CRC) {
                        AUTOK_PRINT("[WARN] CMD CRC error in tuning write[%d %d], need to tune command again!!\r\n",x,m);
                        reTuneCmd = 1;
                    }
                    break;
                }
            }
            x++;
        }
        
        if(reTuneCmd == 1) {
            AUTOK_PRINT("[Err] CMD CRC error in tuning write!!\r\n");
            AUTOK_ERR();
        }

        if (autok_simple_score(w_data) < 32) {

            if (idx < rdat_sel)
                *pRaw = w_data >> ABS_DIFF(idx, rdat_sel);
            else 
                *pRaw = w_data << ABS_DIFF(idx, rdat_sel);

            AUTOK_PRINT("Internal transition boundary found! RDAT:%d\r\n", idx);
            AUTOK_PRINT("%s\r\n", g_tune_result_str);
            break;
        }
    }

    res = E_RESULT_PASS;
    
exit:

    
    msdc_autok_adjust_param(host, DAT_RD_DLY, &rdat_sel, MSDC_WRITE);
    
    return res;
}

static E_RESULT_TYPE 
autok_tune_wd(struct msdc_host *host, U_AUTOK_INTERFACE_DATA *pAutoKData)
{
    unsigned int m,x;
    unsigned int sel=0;
    unsigned int pad_delay_period_cycle = 0;
    unsigned int WRIntPassWin;
    unsigned int fDoubleChk = 0;
    int reTuneCmd = 0;
    
    S_AUTOK_CMD_DLY data;
    AUTOK_RAWD_SCAN_T WRIntScan;
    E_RESULT_TYPE res = E_RESULT_ERR;

    AUTOK_PRINT("=====autok_tune_wd=====\r\n");

    
    pad_delay_period_cycle = pAutoKData[E_MSDC_PAD_DLY_PERIOD].data.sel;
    AUTOK_PRINT("Write Tune, Pad delays per 1 cycle: %d\r\n", pad_delay_period_cycle);
    
    data.raw_data = 0;
    reTuneCmd = 0;
    x = 0;
    while ( x < SCALE_PAD_TUNE_DATWRDLY && reTuneCmd == 0) {
        msdc_autok_adjust_param(host, DAT_WRD_DLY, &x, MSDC_WRITE);
        for (m = 0; m < (AUTOK_WDAT_TIMES * 5); m++) {
            if ((res = autok_write_test(host)) != E_RESULT_PASS) {
                data.raw_data |= (1 << x);
                
                if (autok_recovery(host)) {
                    AUTOK_PRINT("===tune write : error, fail to bring to tranfer status===\r\n");
                    goto exit;
                }
                if (res == E_RESULT_CMD_CRC) {
                    AUTOK_PRINT("[WARN] CMD CRC error in tuning write[%d %d], need to tune command again!!\r\n",x,m);
                    reTuneCmd = 1;
                }
                break;
            }
        }
        x++;
    }
    
    if(reTuneCmd == 1) {
        AUTOK_PRINT("[Err] CMD CRC error in tuning write!!\r\n");
        AUTOK_ERR();
    }

#if defined(MT6592LTE)
    fDoubleChk = 1;
#endif

doubleChkWDAT:
    memset(&WRIntScan, 0, sizeof(WRIntScan));
    WRIntScan.RawData = data.raw_data;
    data.score = autok_simple_score(WRIntScan.RawData);
    autok_check_rawd_style(&WRIntScan, 0);
    AUTOK_PRINT("CKGEN_MSDC_DLY \t PAD_TUNE_DATWDDLY \r\n");
    AUTOK_PRINT("%d \t %d \t %s\r\n", pAutoKData[E_MSDC_CKGEN_MSDC_DLY_SEL].data.sel, 
        data.score, g_tune_result_str);

    if (data.score == 32) {
        sel = 15;

        
    }
    else {
        
        WRIntPassWin = pad_delay_period_cycle - WRIntScan.Reg1Cnt;
        WRIntPassWin /= 2;

        

        if (WRIntScan.Reg1Cnt && !WRIntScan.BoundReg1_E && WRIntScan.BoundReg1_S)
            WRIntScan.BoundReg1_E = 31;

        
        if (WRIntPassWin >= 32) {
            
            if ((31 - WRIntScan.BoundReg1_E) == WRIntScan.BoundReg1_S)
                sel = 0;
            else if ((31 - WRIntScan.BoundReg1_E) > WRIntScan.BoundReg1_S) {
                if ((31 - WRIntScan.BoundReg1_E - WRIntScan.BoundReg1_S) >
                    AUTOK_TUNING_INACCURACY)
                    sel = 31;
                else
                    sel = 0;
            }
            else if ((31 - WRIntScan.BoundReg1_E) <= WRIntScan.BoundReg1_S)
                sel = 0;
        }
        else if ((WRIntPassWin > 16) && (WRIntPassWin < 32)) {
            
            if (WRIntScan.BoundReg1_E < (32 - WRIntPassWin)) {
                sel = WRIntScan.BoundReg1_E + WRIntPassWin - 1;
            }
            else if (WRIntScan.BoundReg1_S > WRIntPassWin) {
                sel = WRIntScan.BoundReg1_S - WRIntPassWin;
            }
            
            else if ((WRIntScan.BoundReg1_E >= (32 - WRIntPassWin)) &&
                    (WRIntScan.BoundReg1_S <= WRIntPassWin)) {

                AUTOK_PRINT("[NOTICE]It is NOT the best margin(WR_INT) but try best\r\n");
                
                
                if ((31 - WRIntScan.BoundReg1_E) == WRIntScan.BoundReg1_S)
                    sel = 0;
                else if ((31 - WRIntScan.BoundReg1_E) > WRIntScan.BoundReg1_S) {
                    if ((31 - WRIntScan.BoundReg1_E - WRIntScan.BoundReg1_S) >
                        AUTOK_TUNING_INACCURACY)
                        sel = 31;
                    else
                        sel = 0;
                }
                else if ((31 - WRIntScan.BoundReg1_E) <= WRIntScan.BoundReg1_S)
                    sel = 0;
            }
            else {
                AUTOK_PRINT("[ERR]Should NOT goto here in WR internal scan!!!\r\n");
                AUTOK_ERR();
            }
        }
        else {
            if (WRIntScan.BoundReg1_E < WRIntPassWin) {
                sel = WRIntScan.BoundReg1_E + WRIntPassWin;
            }
            else if (WRIntScan.BoundReg1_S > WRIntPassWin) {
                sel = WRIntScan.BoundReg1_S - WRIntPassWin;
            }
            else {
                if ((WRIntScan.BoundReg1_E + WRIntPassWin) >= 32)
                    sel = 0;
                else
                    sel = WRIntScan.BoundReg1_E + WRIntPassWin;
            }
            
        }
    }

    
    if ((data.score == 32) && fDoubleChk) {
        fDoubleChk = 0;
        
        if (E_RESULT_PASS == autok_doub_chk_wdat_int_bound(host, &data.raw_data))
            goto doubleChkWDAT;
        else
            goto exit;
    }

    pAutoKData[E_MSDC_PAD_TUNE_DATWRDLY].data.sel = sel;
    sel = pAutoKData[E_MSDC_CKGEN_MSDC_DLY_SEL].data.sel;
    autok_ckg_data[sel].writePadSel = pAutoKData[E_MSDC_PAD_TUNE_DATWRDLY].data.sel;
    msdc_autok_adjust_param(host, DAT_WRD_DLY, &pAutoKData[E_MSDC_PAD_TUNE_DATWRDLY].data.sel, MSDC_WRITE);

    AUTOK_PRINT("Write Internal delay tune done: %d\r\n", pAutoKData[E_MSDC_PAD_TUNE_DATWRDLY].data.sel);

    res = E_RESULT_PASS;

exit:
    return res;
}

static E_RESULT_TYPE 
autok_tune_algorithm(
    struct msdc_host *host, 
    E_AUTOK_TUNING_STAGE stg, 
    U_AUTOK_INTERFACE_DATA *pAutoKData)
{
    E_RESULT_TYPE ret = E_RESULT_ERR;
    
    AUTOK_CYC_SCAN_RES_T rPadDlyRes;
    AUTOK_RD_TUNE_RES_T rRdTuneRes;
    AUTOK_CMD_TUNE_RES_T rCmdTuneRes;
    
    
    AUTOK_PRINT("=======autok_stg%d_tune=======\r\n",stg+1);

    
    memset(&rPadDlyRes, 0, sizeof(rPadDlyRes));
    memset(&rRdTuneRes, 0, sizeof(rRdTuneRes));
    memset(&rCmdTuneRes, 0, sizeof(rCmdTuneRes));
    
    
    if (stg == TUNING_STG1) {
        rPadDlyRes = autok_cycle_scan(host, pAutoKData);
        if (!rPadDlyRes.PadDlyPeriodLen)
            goto err;
    }

    
    if (stg == TUNING_STG1) {
        rRdTuneRes = autok_tune_rd(host, stg, pAutoKData, &rPadDlyRes, &rCmdTuneRes);
        if (!rRdTuneRes.fRetOk)
            goto err;
    }
    
    
    rCmdTuneRes = autok_tune_cmd(host, stg, pAutoKData, &rPadDlyRes, &rRdTuneRes);
    if (!rCmdTuneRes.fRetOk)
        goto err;

    
    if (stg == TUNING_STG2) {
        rRdTuneRes = autok_tune_rd(host, stg, pAutoKData, &rPadDlyRes, &rCmdTuneRes);
        if (!rRdTuneRes.fRetOk)
            goto err;
    }
    
    
    ret = autok_tune_wd(host, pAutoKData);

err:
    return ret;
}


static void autok_tuning_parameter_init(struct msdc_host *host, E_AUTOK_TUNING_STAGE stg, U_AUTOK_INTERFACE_DATA *pAutokData)
{
    unsigned int val=0;

    if (stg == TUNING_STG1)
        containGen();
    
    
    msdc_autok_adjust_param(host, DATA_DLYLINE_SEL, &val, MSDC_WRITE);
    msdc_autok_adjust_param(host, DAT_RD_DLY, &val, MSDC_WRITE);
    msdc_autok_adjust_param(host, DAT_WRD_DLY, &val, MSDC_WRITE);

    
    msdc_autok_adjust_param(host, READ_DATA_SMPL_SEL, &val, MSDC_WRITE);
    msdc_autok_adjust_param(host, WRITE_DATA_SMPL_SEL, &val, MSDC_WRITE);


    
    msdc_autok_adjust_param(host, CMD_RESP_RD_DLY, &val, MSDC_WRITE);

    
    msdc_autok_adjust_param(host, CMD_RD_DLY, &val, MSDC_WRITE);

    
    msdc_autok_adjust_param(host, CKGEN_MSDC_DLY_SEL, &val, MSDC_WRITE);

    if(stg == TUNING_STG1) {
        
        msdc_autok_adjust_param(host, CMD_EDGE, &val, MSDC_WRITE);

        
        msdc_autok_adjust_param(host, CMD_RSP_TA_CNTR, &val, MSDC_READ);
        #ifdef MT6290
        pAutokData[E_MSDC_CMD_RSP_TA_CNTR].data.sel = val;
        #else

        #if 0   
        pAutokData[E_MSDC_CMD_RSP_TA_CNTR].data.sel = 3;
        #else
        pAutokData[E_MSDC_CMD_RSP_TA_CNTR].data.sel = val;
        #endif

        
        if (freq_mhz <= 100)
            pAutokData[E_MSDC_CMD_RSP_TA_CNTR].data.sel = 0;

        msdc_autok_adjust_param(host, CMD_RSP_TA_CNTR, &pAutokData[E_MSDC_CMD_RSP_TA_CNTR].data.sel, MSDC_WRITE);
        #endif
        
        
        msdc_autok_adjust_param(host, INT_DAT_LATCH_CK, &val, MSDC_READ);
        #ifdef MT6290
        pAutokData[E_MSDC_INT_DAT_LATCH_CK_SEL].data.sel = val;
        #else

        #if 0   
        pAutokData[E_MSDC_INT_DAT_LATCH_CK_SEL].data.sel = 0;
        #else
        pAutokData[E_MSDC_INT_DAT_LATCH_CK_SEL].data.sel = val;
        #endif

        
        if (freq_mhz <= 100)
            pAutokData[E_MSDC_INT_DAT_LATCH_CK_SEL].data.sel = 0;
            
        msdc_autok_adjust_param(host, INT_DAT_LATCH_CK, &pAutokData[E_MSDC_INT_DAT_LATCH_CK_SEL].data.sel, MSDC_WRITE);
        #endif

        
        msdc_autok_adjust_param(host, WRDAT_CRCS_TA_CNTR, &val, MSDC_READ);
        #ifdef MT6290
        pAutokData[E_MSDC_WRDAT_CRCS_TA_CNTR].data.sel = val;
        #else

        #if 0   
        pAutokData[E_MSDC_WRDAT_CRCS_TA_CNTR].data.sel = 3;
        #else
        pAutokData[E_MSDC_WRDAT_CRCS_TA_CNTR].data.sel = val;
        #endif

        
        if (freq_mhz <= 100)
            pAutokData[E_MSDC_WRDAT_CRCS_TA_CNTR].data.sel = 0;

        msdc_autok_adjust_param(host, WRDAT_CRCS_TA_CNTR, &pAutokData[E_MSDC_WRDAT_CRCS_TA_CNTR].data.sel, MSDC_WRITE);
        #endif
        
        
        pAutokData[E_MSDC_IOCON_RDSPL].data.sel = 0;
        msdc_autok_adjust_param(host, RDATA_EDGE, &pAutokData[E_MSDC_IOCON_RDSPL].data.sel, MSDC_WRITE);

        
        pAutokData[E_MSDC_IOCON_WDSPL].data.sel = 0;
        msdc_autok_adjust_param(host, WDATA_EDGE, &pAutokData[E_MSDC_IOCON_WDSPL].data.sel, MSDC_WRITE);
    }
    else {
        
        msdc_autok_adjust_param(host, CKGEN_MSDC_DLY_SEL, &pAutokData[E_MSDC_CKGEN_MSDC_DLY_SEL].data.sel, MSDC_WRITE);
        
        
        msdc_autok_adjust_param(host, CMD_RESP_RD_DLY, &pAutokData[E_MSDC_PAD_TUNE_CMDRRDLY].data.sel, MSDC_WRITE);

        
        msdc_autok_adjust_param(host, CMD_RD_DLY, &pAutokData[E_MSDC_PAD_TUNE_CMDRDLY].data.sel, MSDC_WRITE);

        
        if (gfTinyMar && !gfEqualVcore) {
            val = pAutokData[E_MSDC_PAD_TUNE_DATRRDLY].data.sel + 1;
            msdc_autok_adjust_param(host, DAT_RD_DLY, &val, MSDC_WRITE);
        }
        else
            msdc_autok_adjust_param(host, DAT_RD_DLY, &pAutokData[E_MSDC_PAD_TUNE_DATRRDLY].data.sel, MSDC_WRITE);

        
        msdc_autok_adjust_param(host, DAT_WRD_DLY, &pAutokData[E_MSDC_PAD_TUNE_DATWRDLY].data.sel, MSDC_WRITE);
        
        
        msdc_autok_adjust_param(host, CMD_EDGE, &pAutokData[E_MSDC_IOCON_RSPL].data.sel, MSDC_WRITE);

        
        msdc_autok_adjust_param(host, CMD_RSP_TA_CNTR, &pAutokData[E_MSDC_CMD_RSP_TA_CNTR].data.sel, MSDC_WRITE);

        
        msdc_autok_adjust_param(host, INT_DAT_LATCH_CK, &pAutokData[E_MSDC_INT_DAT_LATCH_CK_SEL].data.sel, MSDC_WRITE);

        
        msdc_autok_adjust_param(host, RDATA_EDGE, &pAutokData[E_MSDC_IOCON_RDSPL].data.sel, MSDC_WRITE);

        
        msdc_autok_adjust_param(host, WRDAT_CRCS_TA_CNTR, &pAutokData[E_MSDC_WRDAT_CRCS_TA_CNTR].data.sel, MSDC_WRITE);

        
        msdc_autok_adjust_param(host, WDATA_EDGE, &pAutokData[E_MSDC_IOCON_WDSPL].data.sel, MSDC_WRITE);

    }

}

static void autok_vcore_set(unsigned int vcore_uv) 
{
    unsigned int idx, size;
    unsigned int vcore_sel = 0x48;

    size = sizeof(mt65x2_vcore_tbl)/sizeof(mt65x2_vcore_tbl[0]);

    if (vcore_uv <= mt65x2_vcore_tbl[0]) {
        vcore_sel = 0;
    } else if (vcore_uv >= mt65x2_vcore_tbl[size - 1]) {
        vcore_sel = size - 1;
    } else {		
        for (idx = 0 ; idx < size-1 ; idx ++) {
            if ((vcore_uv >= mt65x2_vcore_tbl[idx]) && 
                (vcore_uv < mt65x2_vcore_tbl[idx+1])) {

                vcore_sel = idx;
                break;
            }
        }
    }

    
    AUTOK_PRINT("Set vcore to %duV(0x%x)\r\n", mt65x2_vcore_tbl[vcore_sel], vcore_sel);

    #ifdef AUTOK_VCORE_SET_LEGACY
        #error K2_shall_not_use_this
        pmic_config_interface(0x21E, vcore_sel, 0xFFFF, 0);
        pmic_config_interface(0x220, vcore_sel, 0xFFFF, 0);
    #else
        #ifdef CONFIG_SDIOAUTOK_SUPPORT
        
        mt_vcore_dvfs_volt_set_by_sdio(mt65x2_vcore_tbl[vcore_sel]);
        #endif
    #endif
    
    mdelay(1);
}

static void autok_get_reg_field(unsigned int param, unsigned int *field)
{
    switch (param) {
        case E_MSDC_PAD_TUNE_CMDRRDLY:
            *field = CMD_RESP_RD_DLY;
        break;
        case E_MSDC_CMD_RSP_TA_CNTR:
            *field = CMD_RSP_TA_CNTR;
        break;
        case E_MSDC_IOCON_RSPL:
            *field = CMD_EDGE;
        break;
        case E_MSDC_CKGEN_MSDC_DLY_SEL:
            *field = CKGEN_MSDC_DLY_SEL;
        break;
        case E_MSDC_PAD_TUNE_CMDRDLY:
            *field = CMD_RD_DLY;
        break;
        case E_MSDC_INT_DAT_LATCH_CK_SEL:
            *field = INT_DAT_LATCH_CK;
        break;
        case E_MSDC_IOCON_RDSPL:
            *field = RDATA_EDGE;
        break;
        case E_MSDC_PAD_TUNE_DATRRDLY:
            *field = DAT_RD_DLY;
        break;
        case E_MSDC_WRDAT_CRCS_TA_CNTR:
            *field = WRDAT_CRCS_TA_CNTR;
        break;
        case E_MSDC_IOCON_WDSPL:
            *field = WDATA_EDGE;
        break;
        case E_MSDC_PAD_TUNE_DATWRDLY:
            *field = DAT_WRD_DLY;
        break;

        default:
            AUTOK_PRINT("[ERR]Can NOT find the delay cell field!!!\r\n");
            *field = 0;
        break;
    }
}

static void autok_show_parameters(struct msdc_host *host, void *pData)
{
    unsigned int parm;
    unsigned int val;
    unsigned int field;
    U_AUTOK_INTERFACE_DATA *pAutok;

    pAutok = (U_AUTOK_INTERFACE_DATA *)pData;

    AUTOK_PRINT("=====Delay Params Show:=====\r\n");
    for(parm = 0; parm < E_AUTOK_DLY_PARAM_MAX; parm++) {
        autok_get_reg_field(parm, &field);
        msdc_autok_adjust_param(host, field, &val, MSDC_READ);
        
        if(val != pAutok[parm].data.sel) {
            AUTOK_PRINT("%s expect:%02d, real:%02d\r\n", autok_param_name[parm], pAutok[parm].data.sel, val);
        }else {
            AUTOK_PRINT("%s value:%02d\r\n", autok_param_name[parm], val);
        }
    }
}

static void autok_setup_envir(struct msdc_host *host)
{
    freq_mhz = host->mclk/1000000;

    #if defined(MT6582LTE)
    {
    unsigned int io_ring, core_ring;
    
        if (freq_mhz >= 200) {
            autok_vcore_scan_num = AUTOK_VCORE_SCAN_NUM;
            autok_rdat_fbound_th = 6;
        }
        else if (freq_mhz >= 150) {
            autok_vcore_scan_num = AUTOK_VCORE_SCAN_NUM;
            autok_rdat_fbound_th = 4;
        }
        else if (freq_mhz >= 100) {
            autok_vcore_scan_num = 1;
            autok_rdat_fbound_th = 3;
        }
        else if (freq_mhz >= 50) {
            autok_vcore_scan_num = 1;
            autok_rdat_fbound_th = 2;
        }

        sdio_get_rings(&io_ring, &core_ring);
        AUTOK_PRINT("Get IO_RING(%d) and CORE_RING(%d)\r\n", io_ring, core_ring);

        if (io_ring < AUTOK_SS_IO_RING_TH) {

            AUTOK_PRINT("SS IO corner has been found!\r\n");
            gfIOSS = 1;
        }
        else
            gfIOSS = 0;
        
        if (core_ring < AUTOK_TT_CORE_RING_TH) {

            AUTOK_PRINT("TT Core corner has been found!\r\n");
            gfCoreTT = 1;
        }
        else
            gfCoreTT = 0;
    }
    #elif (defined(MT6592LTE) || defined(MT6595WIFI) || defined(MT6752WIFI))
        if (freq_mhz >= 200) {
            autok_vcore_scan_num = AUTOK_VCORE_SCAN_NUM;
            autok_rdat_fbound_th = 2;
        }
        else if (freq_mhz >= 150) {
            autok_vcore_scan_num = AUTOK_VCORE_SCAN_NUM;
            autok_rdat_fbound_th = 1;
        }
        else if (freq_mhz >= 100) {
            autok_vcore_scan_num = 1;
            autok_rdat_fbound_th = 1;
        }
        else if (freq_mhz >= 50) {
            autok_vcore_scan_num = 1;
            autok_rdat_fbound_th = 1;
        }
    #endif

    AUTOK_PRINT("freq:%d, vcore_num:%d, rdat_th:%d\r\n", freq_mhz, 
        autok_vcore_scan_num, autok_rdat_fbound_th);
    return;
}

unsigned int 
msdc_autok_get_vcore(unsigned int vcore_uv, unsigned int *pfIdentical)
{
    unsigned int idx, size, vcore_sel = 0;
    unsigned int autok_vcore_sel[autok_vcore_scan_num*2 - 1];

    size = sizeof(autok_vcore_sel)/sizeof(autok_vcore_sel[0]);

    

    if (!gfTinyMar) {
        for (idx = 0; idx < size; idx++) {
            if ((idx % 2) == 0)
                autok_vcore_sel[idx] = g_autok_vcore_sel[idx/2];
            else
                autok_vcore_sel[idx] = g_autok_vcore_sel[idx/2] +
                    (g_autok_vcore_sel[idx/2 + 1] - g_autok_vcore_sel[idx/2])/2;

            
        }
        
        
        if (vcore_uv <= g_autok_vcore_sel[0])
            
            vcore_sel = 0;
        else if (vcore_uv >= g_autok_vcore_sel[autok_vcore_scan_num-1])
            
            vcore_sel = autok_vcore_scan_num-1;
        else {
            for (idx = 0; idx < size; idx++) {
                if ((vcore_uv > autok_vcore_sel[idx]) && 
                    (vcore_uv <= autok_vcore_sel[idx+1])) {

                    if ((idx % 2) == 0)
                        vcore_sel = idx/2;
                    else 
                        vcore_sel = idx/2 + 1;

                    break;
                }
            }
        }
    }
    else {
        
        
        
        
        if (vcore_uv <= g_autok_vcore_sel[0])
            
            vcore_sel = 0;
        else if (vcore_uv >= g_autok_vcore_sel[autok_vcore_scan_num-1])
            
            vcore_sel = autok_vcore_scan_num-1;
        else {
            for (idx = 0; idx < autok_vcore_scan_num; idx++) {
                if ((vcore_uv >= g_autok_vcore_sel[idx]) && 
                    (vcore_uv < g_autok_vcore_sel[idx+1])) {

                    vcore_sel = idx;
                    break;
                }
            }
        }

        if (g_autok_vcore_sel[vcore_sel] == vcore_uv)
            gfEqualVcore = 1;
        else
            gfEqualVcore = 0;
    }

    AUTOK_PRINT("Cur Vcore:%duV, Sel Vcore:%duV\r\n", vcore_uv, g_autok_vcore_sel[vcore_sel]);

    if ((vcore_uv == g_autok_vcore_sel[vcore_sel]) && pfIdentical)
        *pfIdentical = 1;
    else if ((vcore_uv != g_autok_vcore_sel[vcore_sel]) && pfIdentical)
        *pfIdentical = 0;

    return vcore_sel;
}

int 
msdc_autok_apply_param(struct msdc_host *host, unsigned int vcore_uv_off)
{
    U_AUTOK_INTERFACE_DATA *pAutok;
    unsigned int vcore_sel, vcore_uv, fIdent = 0;

    
    if (host == NULL)
        return -1;
    if (!g_pp_autok_data) {
        AUTOK_PRINT("NULL autok param pointer on param apply!\r\n");
        return -1;
    }

    
    vcore_uv = mt65x2_vcore_tbl[vcore_uv_off];
    vcore_sel = msdc_autok_get_vcore(vcore_uv, &fIdent);
    pAutok = *(g_pp_autok_data + vcore_sel);

    if((pAutok + E_AUTOK_VERSION)->version != AUTOK_VERSION_NO)
    {
        AUTOK_PRINT("autoK version wrong = %d\r\n", pAutok->version);
        return -2;
    }

    
    AUTOK_PRINT("msdc_autok_apply_param...%s\r\n", gfTinyMar ? "(TinyMargin)" : "");
    autok_tuning_parameter_init(host, TUNING_STG2, pAutok);
    autok_show_parameters(host, pAutok);
    
    return 0;
}

int msdc_autok_stg1_cal(
    struct msdc_host *host, 
    unsigned int offset_restore, 
    struct autok_predata *p_single_autok)
{
    E_RESULT_TYPE res= E_RESULT_ERR;
    U_AUTOK_INTERFACE_DATA *pAutok;
    
    
    if ((p_single_autok == NULL) || (host == NULL)) {
        AUTOK_PRINT("NULL autok param pointer on STG1!\r\n");
        return -1;
    }
    if (!p_single_autok->vol_list || !p_single_autok->ai_data) {
        AUTOK_PRINT("NULL autok param pointer on STG1!\r\n");
        return -1;
    }

    
    autok_setup_envir(host);
    
    AUTOK_PRINT("Stage1 statistic : start\r\n");

    
    autok_vcore_set(*p_single_autok->vol_list);
    autok_get_current_vcore_offset();

    pAutok = *p_single_autok->ai_data;
    memset((void *)pAutok, 0, sizeof(U_AUTOK_INTERFACE_DATA)*MAX_AUTOK_DAT_NUM);
    autok_tuning_parameter_init(host, TUNING_STG1, pAutok);
    res = autok_tune_algorithm(host, TUNING_STG1, pAutok);

    if (!res)
        autok_show_parameters(host, pAutok);
    else {
        AUTOK_PRINT("[ERR]msdc_autok_stg1_cal returns %d\r\n", res);
        goto exit;
    }

    pAutok[E_AUTOK_VERSION].version = AUTOK_VERSION_NO;
    pAutok[E_AUTOK_FREQ].freq = host->mclk;

exit:
    AUTOK_PRINT("Restore vcore to %duV\r\n", mt65x2_vcore_tbl[offset_restore]);
    autok_vcore_set(mt65x2_vcore_tbl[offset_restore]);
    
    AUTOK_PRINT("Stage1 statistic : end\r\n");

    return -res;
}


int msdc_autok_stg1_data_get(void **ppData, int *pLen)
{
    if(ppData == NULL || pLen == NULL)
        return -1;

    *ppData = (void *)(*g_pp_autok_data);
    *pLen = sizeof(U_AUTOK_INTERFACE_DATA)*(MAX_AUTOK_DAT_NUM*autok_vcore_scan_num);
    return 0;
}

int msdc_autok_stg2_cal(
    struct msdc_host *host, 
    struct autok_predata *p_autok_data, 
    unsigned int vcore_uv_off)
{
    unsigned int idx, vcore_sel, vcore_uv, fIdent = 0;
    U_AUTOK_INTERFACE_DATA *pAutok;
    
    E_RESULT_TYPE res= E_RESULT_ERR;

    
    autok_setup_envir(host);

    
    if (p_autok_data == NULL || host == NULL) {
        AUTOK_PRINT("NULL autok param pointer on STG2!\r\n");
        return -1;
    }
    if (!p_autok_data->ai_data) {
        AUTOK_PRINT("NULL autok param pointer on STG2!\r\n");
        return -1;
    }
    
    g_pp_autok_data = p_autok_data->ai_data;

    
    autok_vcore_scan_num = p_autok_data->vol_count;
    if (autok_vcore_scan_num > AUTOK_VCORE_SCAN_NUM) {
        AUTOK_PRINT("Exceeds the Vcore scan number range\r\n");
        return -1;
    }
    for (idx = 0; idx < autok_vcore_scan_num; idx++) {
        g_autok_vcore_sel[idx] = *(p_autok_data->vol_list + idx);

        
        pAutok = *(g_pp_autok_data + idx);
        if (pAutok[E_MSDC_F_TINY_MARGIN].data.sel == AUTOK_TINY_MAR_PAT) {
            
            gfTinyMar = 1;
            AUTOK_PRINT("Tiny margin found\r\n");
        }
    }
    
    
    vcore_uv = mt65x2_vcore_tbl[vcore_uv_off];
    vcore_sel = msdc_autok_get_vcore(vcore_uv, &fIdent);
    pAutok = *(g_pp_autok_data + vcore_sel);

    if((pAutok + E_AUTOK_VERSION)->version != AUTOK_VERSION_NO) {
        AUTOK_PRINT("autoK version wrong = %d\r\n", pAutok->version);
        return -2;
    }

    if((pAutok + E_AUTOK_FREQ)->freq != host->mclk) {
        AUTOK_PRINT("Now operation freq(%d) not meet autok freq(%d)\r\n", 
            host->mclk, (pAutok + E_AUTOK_FREQ)->freq);
        return -2;
    }

    autok_tuning_parameter_init(host, TUNING_STG2, pAutok);
    autok_show_parameters(host, pAutok);

    #if 0
    res = autok_tune_algorithm(host, TUNING_STG2, pAutok);
    #else
    AUTOK_PRINT("=======autok_stg%d_tune=======\r\n",TUNING_STG2+1);
    AUTOK_PRINT("stg%d is bypassed currently\r\n",TUNING_STG2+1);
    res = E_RESULT_PASS;
    #endif
    
    if (!res)
        autok_show_parameters(host, pAutok);
    else
        AUTOK_PRINT("[ERR]msdc_autok_stg2_cal returns %d\r\n", res);

    return -res;
}

int msdc_autok_get_suggetst_vcore(unsigned int **suggest_vol_tbl){
    unsigned int tbl_size=sizeof(unsigned int)*AUTOK_VCORE_SCAN_NUM;
    *suggest_vol_tbl = kzalloc(tbl_size, GFP_KERNEL);
    memcpy(*suggest_vol_tbl, g_autok_vcore_sel, tbl_size);
    return AUTOK_VCORE_SCAN_NUM;
}

