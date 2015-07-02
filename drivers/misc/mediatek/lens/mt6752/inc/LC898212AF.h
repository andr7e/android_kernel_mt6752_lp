#ifndef _LC898212AF_H
#define _LC898212AF_H

#include <linux/ioctl.h>

#define LC898212AF_MAGIC 'A'


typedef struct {
u32 u4CurrentPosition;
u32 u4MacroPosition;
u32 u4InfPosition;
bool          bIsMotorMoving;
bool          bIsMotorOpen;
bool          bIsSupportSR;
} stLC898212AF_MotorInfo;

#define LC898212AFIOC_G_MOTORINFO _IOR(LC898212AF_MAGIC,0,stLC898212AF_MotorInfo)

#define LC898212AFIOC_T_MOVETO _IOW(LC898212AF_MAGIC,1,u32)

#define LC898212AFIOC_T_SETINFPOS _IOW(LC898212AF_MAGIC,2,u32)

#define LC898212AFIOC_T_SETMACROPOS _IOW(LC898212AF_MAGIC,3,u32)

#else
#endif
