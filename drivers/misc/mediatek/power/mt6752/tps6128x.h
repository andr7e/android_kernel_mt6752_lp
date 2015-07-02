
#ifndef _tps6128x_SW_H_
#define _tps6128x_SW_H_

#define tps6128x_REG_NUM 6

extern void tps6128x_dump_register(void);
extern kal_uint32 tps6128x_read_interface (kal_uint8 RegNum, kal_uint8 *val, kal_uint8 MASK, kal_uint8 SHIFT);
extern kal_uint32 tps6128x_config_interface (kal_uint8 RegNum, kal_uint8 val, kal_uint8 MASK, kal_uint8 SHIFT);

#endif 

