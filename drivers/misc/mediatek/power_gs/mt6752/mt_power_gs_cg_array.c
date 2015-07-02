
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <mach/mt_typedefs.h>
#include <mach/mt_power_gs.h>

const unsigned int AP_CG_gs_dpidle_data[] = {
 
    0xF0000040, 0xFFFFFFFF, 0x00810106,
    0xF0000050, 0xFFFFFFFF, 0x80818180,
    0xF0000060, 0xFFFFFFFF, 0x81818180,
    0xF0000070, 0xFFFFFFFF, 0x81808181,
    0xF0000080, 0xFFFFFFFF, 0x81810300,
    0xF0000090, 0x00009787, 0x00008181,
    0xF0000200, 0x000003FF, 0x000003FF,
    0xF0000204, 0x00000011, 0x00000011,
    0xF0001040, 0x0081BFE3, 0x0000B1E2,
    0xF0001044, 0x00C1BFE3, 0x00000000,
    0xF0001048, 0x00C1BFE3, 0x0000B1E2,
    0xF0003008, 0x3FFFFFFF, 0x2F77EFFD,
    0xF0003010, 0x3FFFFFFF, 0x00000000,
    0xF0003018, 0x0C000000, 0x0C000000,
    0xF000F640, 0x000000B4, 0x00000094,
    0xF000F690, 0x00000084, 0x00000084,
    0xF0012640, 0x00000084, 0x00000084,
    0xF0209008, 0x00000007, 0x00000000,
    0xF020900C, 0x0007FFFF, 0x00000000,
    0xF0209010, 0x0000000F, 0x00000000,
    0xF0209018, 0x0007FFFF, 0x00000000,
    0xF0209020, 0xFFFFFFFF, 0x05010501,
    0xF0209024, 0x0000000F, 0x00000003,
    0xF0209028, 0x0000000F, 0x00000001,
    0xF020902C, 0x0000000F, 0x00000008,
    0xF0209030, 0x0000000F, 0x00000001,
    0xF0209034, 0x0000000F, 0x00000006,
    0xF0209038, 0x000000FF, 0x00000096,
    0xF020903C, 0x0000000F, 0x00000001,
    0xF0209200, 0x00000001, 0x00000000,
    0xF020920C, 0x00000003, 0x00000002,
    0xF0209240, 0x00000001, 0x00000000,
    0xF020924C, 0x00000003, 0x00000002,
    0xF0209250, 0x00000001, 0x00000000,
    0xF020925C, 0x00000003, 0x00000002,
    0xF0209260, 0x00000001, 0x00000000,
    0xF020926C, 0x00000003, 0x00000002,
    0xF0209270, 0x00000001, 0x00000000,
    0xF020927C, 0x00000003, 0x00000002,
    0xF0209290, 0x00000001, 0x00000000,
    0xF020929C, 0x00000003, 0x00000002,
    0xF02092A0, 0x00000001, 0x00000000,
    0xF02092B0, 0x00000003, 0x00000002,
    0xF02092B4, 0x00000001, 0x00000000,
    0xF02092C4, 0x00000003, 0x00000002,
    0xF1220000, 0x00000044, 0x00000044,
    0xF4000100, 0xFFFFFFFF, 0xFFFFFFFF,
    0xF4000110, 0x000003FF, 0x000003FF,
    0xF5000000, 0x00000BE1, 0x00000BE1,
    0xF6000000, 0x00000001, 0x00000000,
    0xF6000004, 0x00000001, 0x00000000,
    0xF6000008, 0x00000001, 0x00000000,
    0xF600000C, 0x00000001, 0x00000000,
    0xF6000010, 0x00000001, 0x00000001,
    0xF6000014, 0x00000001, 0x00000001,
    0xF7000000, 0xFFFFFFFF, 0xFFFFFFFF,
    0xF7000004, 0xFFFFFFFF, 0x00000000,
    0xF7000008, 0xFFFFFFFF, 0x00000000,
    0xF8000000, 0x00001111, 0x00000000,
    0xF80020EC, 0x00000001, 0x00000000 
};

const unsigned int *AP_CG_gs_dpidle = AP_CG_gs_dpidle_data;

unsigned int AP_CG_gs_dpidle_len = 192;

const unsigned int AP_CG_gs_suspend_data[] = {
 
    0xF0000040, 0xFFFFFFFF, 0x00810106,
    0xF0000050, 0xFFFFFFFF, 0x80808080,
    0xF0000060, 0xFFFFFFFF, 0x80808080,
    0xF0000070, 0xFFFFFFFF, 0x80808080,
    0xF0000080, 0xFFFFFFFF, 0x81810300,
    0xF0000090, 0x00009787, 0x00008181,
    0xF0000200, 0x000003FF, 0x000003FF,
    0xF0000204, 0x00000011, 0x00000011,
    0xF0001040, 0x0081BFE3, 0x0080BFE2,
    0xF0001044, 0x00C1BFE3, 0x00000000,
    0xF0001048, 0x00C1BFE3, 0x0080BFE2,
    0xF0003008, 0x3FFFFFFF, 0x2FFFFFFF,
    0xF0003010, 0x3FFFFFFF, 0x00000000,
    0xF0003018, 0x0C000000, 0x0C000000,
    0xF000F640, 0x000000B4, 0x00000094,
    0xF000F690, 0x00000084, 0x00000084,
    0xF0012640, 0x00000084, 0x00000084,
    0xF0209008, 0x00000007, 0x00000000,
    0xF020900C, 0x0007FFFF, 0x00000000,
    0xF0209010, 0x0000000F, 0x00000000,
    0xF0209018, 0x0007FFFF, 0x00000000,
    0xF0209020, 0xFFFFFFFF, 0x05010501,
    0xF0209024, 0x0000000F, 0x00000003,
    0xF0209028, 0x0000000F, 0x00000001,
    0xF020902C, 0x0000000F, 0x00000008,
    0xF0209030, 0x0000000F, 0x00000001,
    0xF0209034, 0x0000000F, 0x00000006,
    0xF0209038, 0x000000FF, 0x00000096,
    0xF020903C, 0x0000000F, 0x00000001,
    0xF0209200, 0x00000001, 0x00000000,
    0xF020920C, 0x00000003, 0x00000002,
    0xF0209240, 0x00000001, 0x00000000,
    0xF020924C, 0x00000003, 0x00000002,
    0xF0209250, 0x00000001, 0x00000000,
    0xF020925C, 0x00000003, 0x00000002,
    0xF0209260, 0x00000001, 0x00000000,
    0xF020926C, 0x00000003, 0x00000002,
    0xF0209270, 0x00000001, 0x00000000,
    0xF020927C, 0x00000003, 0x00000002,
    0xF0209290, 0x00000001, 0x00000000,
    0xF020929C, 0x00000003, 0x00000002,
    0xF02092A0, 0x00000001, 0x00000000,
    0xF02092B0, 0x00000003, 0x00000002,
    0xF02092B4, 0x00000001, 0x00000000,
    0xF02092C4, 0x00000003, 0x00000002,
    0xF1220000, 0x00000044, 0x00000044,
    0xF4000100, 0xFFFFFFFF, 0xFFFFFFFF,
    0xF4000110, 0x000003FF, 0x000003FF,
    0xF5000000, 0x00000BE1, 0x00000BE1,
    0xF6000000, 0x00000001, 0x00000000,
    0xF6000004, 0x00000001, 0x00000000,
    0xF6000008, 0x00000001, 0x00000000,
    0xF600000C, 0x00000001, 0x00000000,
    0xF6000010, 0x00000001, 0x00000001,
    0xF6000014, 0x00000001, 0x00000001,
    0xF7000000, 0xFFFFFFFF, 0xFFFFFFFF,
    0xF7000004, 0xFFFFFFFF, 0x00000000,
    0xF7000008, 0xFFFFFFFF, 0x00000000,
    0xF8000000, 0x00001111, 0x00001000,
    0xF80020EC, 0x00000001, 0x00000000 
};

const unsigned int *AP_CG_gs_suspend = AP_CG_gs_suspend_data;

unsigned int AP_CG_gs_suspend_len = 192;

const unsigned int AP_CG_gs_vp_data[] = {
 
    0xF0000040, 0xFFFFFFFF, 0x01810101,
    0xF0000050, 0xFFFFFFFF, 0x80010100,
    0xF0000060, 0xFFFFFFFF, 0x81818180,
    0xF0000070, 0xFFFFFFFF, 0x01008101,
    0xF0000080, 0xFFFFFFFF, 0x81010300,
    0xF0000090, 0x00009787, 0x00000181,
    0xF0000200, 0x000003FF, 0x000003FF,
    0xF0000204, 0x00000011, 0x00000011,
    0xF0001040, 0x0081B1E3, 0x0081B0C0,
    0xF0001044, 0x00C1BFE3, 0x00000000,
    0xF0001048, 0x00C1B1E3, 0x0081B0C0,
    0xF0003008, 0x3FFFFFFF, 0x2FF7EFFD,
    0xF0003010, 0x3FFFFFFF, 0x00000000,
    0xF0003018, 0x0C000000, 0x0C000000,
    0xF000F640, 0x000000B4, 0x00000000,
    0xF000F690, 0x00000084, 0x00000000,
    0xF0012640, 0x00000084, 0x00000000,
    0xF0209008, 0x00000007, 0x00000000,
    0xF020900C, 0x0007FFFF, 0x00000000,
    0xF0209010, 0x0000000F, 0x00000000,
    0xF0209018, 0x0007FFFF, 0x00000000,
    0xF0209020, 0xFFFFFFFF, 0x05010501,
    0xF0209024, 0x0000000F, 0x00000003,
    0xF0209028, 0x0000000F, 0x00000001,
    0xF020902C, 0x0000000F, 0x00000008,
    0xF0209030, 0x0000000F, 0x00000001,
    0xF0209034, 0x0000000F, 0x00000006,
    0xF0209038, 0x000000FF, 0x00000096,
    0xF020903C, 0x0000000F, 0x00000001,
    0xF0209200, 0x00000001, 0x00000000,
    0xF020920C, 0x00000003, 0x00000002,
    0xF0209240, 0x00000001, 0x00000000,
    0xF020924C, 0x00000003, 0x00000002,
    0xF0209250, 0x00000001, 0x00000001,
    0xF020925C, 0x00000003, 0x00000001,
    0xF0209260, 0x00000001, 0x00000001,
    0xF020926C, 0x00000003, 0x00000001,
    0xF0209270, 0x00000001, 0x00000001,
    0xF020927C, 0x00000003, 0x00000001,
    0xF0209290, 0x00000001, 0x00000000,
    0xF020929C, 0x00000003, 0x00000002,
    0xF02092A0, 0x00000001, 0x00000000,
    0xF02092B0, 0x00000003, 0x00000002,
    0xF02092B4, 0x00000001, 0x00000000,
    0xF02092C4, 0x00000003, 0x00000002,
    0xF1220000, 0x00000044, 0x00000000,
    0xF4000100, 0xFFFFFFFF, 0xFFFF7BFC,
    0xF4000110, 0x000003FF, 0x0000030C,
    0xF5000000, 0x00000BE1, 0x00000BE1,
    0xF6000000, 0x00000001, 0x00000001,
    0xF6000004, 0x00000001, 0x00000000,
    0xF6000008, 0x00000001, 0x00000001,
    0xF600000C, 0x00000001, 0x00000000,
    0xF6000010, 0x00000001, 0x00000001,
    0xF6000014, 0x00000001, 0x00000001,
    0xF7000000, 0xFFFFFFFF, 0xFFFFFFF0,
    0xF7000004, 0xFFFFFFFF, 0x00000000,
    0xF7000008, 0xFFFFFFFF, 0x00000000,
    0xF8000000, 0x00001111, 0x00000000,
    0xF80020EC, 0x00000001, 0x00000000 
};

const unsigned int *AP_CG_gs_vp = AP_CG_gs_vp_data;

unsigned int AP_CG_gs_vp_len = 192;

const unsigned int AP_CG_gs_paging_data[] = {
 
    0xF0000040, 0xFFFFFFFF, 0x00810106,
    0xF0000050, 0xFFFFFFFF, 0x80818180,
    0xF0000060, 0xFFFFFFFF, 0x81818180,
    0xF0000070, 0xFFFFFFFF, 0x81808181,
    0xF0000080, 0xFFFFFFFF, 0x81810300,
    0xF0000090, 0x00009787, 0x00008181,
    0xF0000200, 0x000003FF, 0x000003FF,
    0xF0000204, 0x00000011, 0x00000011,
    0xF0001040, 0x0081B1E3, 0x0001B1E2,
    0xF0001044, 0x00C1BFE3, 0x00000000,
    0xF0001048, 0x00C1B1E3, 0x0001B1E2,
    0xF0003008, 0x3FFFFFFF, 0x2FF7EFFD,
    0xF0003010, 0x3FFFFFFF, 0x00000000,
    0xF0003018, 0x0C000000, 0x0C000000,
    0xF000F640, 0x000000B4, 0x00000000,
    0xF000F690, 0x00000084, 0x00000000,
    0xF0012640, 0x00000084, 0x00000000,
    0xF0209008, 0x00000007, 0x00000000,
    0xF020900C, 0x0007FFFF, 0x00000000,
    0xF0209010, 0x0000000F, 0x00000000,
    0xF0209018, 0x0007FFFF, 0x00000000,
    0xF0209020, 0xFFFFFFFF, 0x05010501,
    0xF0209024, 0x0000000F, 0x00000003,
    0xF0209028, 0x0000000F, 0x00000001,
    0xF020902C, 0x0000000F, 0x00000008,
    0xF0209030, 0x0000000F, 0x00000001,
    0xF0209034, 0x0000000F, 0x00000006,
    0xF0209038, 0x000000FF, 0x00000096,
    0xF020903C, 0x0000000F, 0x00000001,
    0xF0209200, 0x00000001, 0x00000000,
    0xF020920C, 0x00000003, 0x00000002,
    0xF0209240, 0x00000001, 0x00000000,
    0xF020924C, 0x00000003, 0x00000002,
    0xF0209250, 0x00000001, 0x00000000,
    0xF020925C, 0x00000003, 0x00000002,
    0xF0209260, 0x00000001, 0x00000000,
    0xF020926C, 0x00000003, 0x00000002,
    0xF0209270, 0x00000001, 0x00000000,
    0xF020927C, 0x00000003, 0x00000002,
    0xF0209290, 0x00000001, 0x00000000,
    0xF020929C, 0x00000003, 0x00000002,
    0xF02092A0, 0x00000001, 0x00000000,
    0xF02092B0, 0x00000003, 0x00000002,
    0xF02092B4, 0x00000001, 0x00000000,
    0xF02092C4, 0x00000003, 0x00000002,
    0xF1220000, 0x00000044, 0x00000044,
    0xF4000100, 0xFFFFFFFF, 0xFFFFFFFF,
    0xF4000110, 0x000003FF, 0x000003FF,
    0xF5000000, 0x00000BE1, 0x00000BE1,
    0xF6000000, 0x00000001, 0x00000000,
    0xF6000004, 0x00000001, 0x00000000,
    0xF6000008, 0x00000001, 0x00000000,
    0xF600000C, 0x00000001, 0x00000000,
    0xF6000010, 0x00000001, 0x00000001,
    0xF6000014, 0x00000001, 0x00000001,
    0xF7000000, 0xFFFFFFFF, 0xFFFFFFFF,
    0xF7000004, 0xFFFFFFFF, 0x00000000,
    0xF7000008, 0xFFFFFFFF, 0x00000000,
    0xF8000000, 0x00001111, 0x00000000,
    0xF80020EC, 0x00000001, 0x00000000 
};

const unsigned int *AP_CG_gs_paging = AP_CG_gs_paging_data;

unsigned int AP_CG_gs_paging_len = 192;

const unsigned int AP_CG_gs_mp3_play_data[] = {
 
    0xF0000040, 0xFFFFFFFF, 0x00810106,
    0xF0000050, 0xFFFFFFFF, 0x80818180,
    0xF0000060, 0xFFFFFFFF, 0x81818180,
    0xF0000070, 0xFFFFFFFF, 0x00008101,
    0xF0000080, 0xFFFFFFFF, 0x81010300,
    0xF0000090, 0x00009787, 0x00008181,
    0xF0000200, 0x000003FF, 0x000003FF,
    0xF0000204, 0x00000011, 0x00000011,
    0xF0001040, 0x0081B1E3, 0x0001B1C2,
    0xF0001044, 0x00C1BFE3, 0x00000000,
    0xF0001048, 0x00C1B1E3, 0x0001B1C2,
    0xF0003008, 0x3FFFFFFF, 0x2FF7EFFD,
    0xF0003010, 0x3FFFFFFF, 0x00000000,
    0xF0003018, 0x0C000000, 0x0C000000,
    0xF000F640, 0x000000B4, 0x00000000,
    0xF000F690, 0x00000084, 0x00000000,
    0xF0012640, 0x00000084, 0x00000000,
    0xF0209008, 0x00000007, 0x00000000,
    0xF020900C, 0x0007FFFF, 0x00000000,
    0xF0209010, 0x0000000F, 0x00000000,
    0xF0209018, 0x0007FFFF, 0x00000000,
    0xF0209020, 0xFFFFFFFF, 0x05010501,
    0xF0209024, 0x0000000F, 0x00000003,
    0xF0209028, 0x0000000F, 0x00000001,
    0xF020902C, 0x0000000F, 0x00000008,
    0xF0209030, 0x0000000F, 0x00000001,
    0xF0209034, 0x0000000F, 0x00000006,
    0xF0209038, 0x000000FF, 0x00000096,
    0xF020903C, 0x0000000F, 0x00000001,
    0xF0209200, 0x00000001, 0x00000000,
    0xF020920C, 0x00000003, 0x00000002,
    0xF0209240, 0x00000001, 0x00000000,
    0xF020924C, 0x00000003, 0x00000002,
    0xF0209250, 0x00000001, 0x00000000,
    0xF020925C, 0x00000003, 0x00000002,
    0xF0209260, 0x00000001, 0x00000000,
    0xF020926C, 0x00000003, 0x00000002,
    0xF0209270, 0x00000001, 0x00000000,
    0xF020927C, 0x00000003, 0x00000002,
    0xF0209290, 0x00000001, 0x00000000,
    0xF020929C, 0x00000003, 0x00000002,
    0xF02092A0, 0x00000001, 0x00000000,
    0xF02092B0, 0x00000003, 0x00000002,
    0xF02092B4, 0x00000001, 0x00000000,
    0xF02092C4, 0x00000003, 0x00000002,
    0xF1220000, 0x00000044, 0x00000000,
    0xF4000100, 0xFFFFFFFF, 0xFFFFFFFF,
    0xF4000110, 0x000003FF, 0x000003FF,
    0xF5000000, 0x00000BE1, 0x00000BE1,
    0xF6000000, 0x00000001, 0x00000000,
    0xF6000004, 0x00000001, 0x00000000,
    0xF6000008, 0x00000001, 0x00000000,
    0xF600000C, 0x00000001, 0x00000000,
    0xF6000010, 0x00000001, 0x00000001,
    0xF6000014, 0x00000001, 0x00000001,
    0xF7000000, 0xFFFFFFFF, 0xFFFFFFFF,
    0xF7000004, 0xFFFFFFFF, 0x00000000,
    0xF7000008, 0xFFFFFFFF, 0x00000000,
    0xF8000000, 0x00001111, 0x00000000,
    0xF80020EC, 0x00000001, 0x00000000 
};

const unsigned int *AP_CG_gs_mp3_play = AP_CG_gs_mp3_play_data;

unsigned int AP_CG_gs_mp3_play_len = 192;

const unsigned int AP_CG_gs_idle_data[] = {
 
    0xF0000040, 0xFFFFFFFF, 0x00810106,
    0xF0000050, 0xFFFFFFFF, 0x80818180,
    0xF0000060, 0xFFFFFFFF, 0x81818180,
    0xF0000070, 0xFFFFFFFF, 0x81808181,
    0xF0000080, 0xFFFFFFFF, 0x81810300,
    0xF0000090, 0x00009787, 0x00008181,
    0xF0000200, 0x000003FF, 0x000003FF,
    0xF0000204, 0x00000011, 0x00000011,
    0xF0001040, 0x0081BFE3, 0x0000B1E2,
    0xF0001044, 0x00C1BFE3, 0x00000000,
    0xF0001048, 0x00C1BFE3, 0x0000B1E2,
    0xF0003008, 0x3FFFFFFF, 0x2F77EFFD,
    0xF0003010, 0x3FFFFFFF, 0x00000000,
    0xF0003018, 0x0C000000, 0x0C000000,
    0xF000F640, 0x000000B4, 0x00000094,
    0xF000F690, 0x00000084, 0x00000084,
    0xF0012640, 0x00000084, 0x00000084,
    0xF0209008, 0x00000007, 0x00000000,
    0xF020900C, 0x0007FFFF, 0x00000000,
    0xF0209010, 0x0000000F, 0x00000000,
    0xF0209018, 0x0007FFFF, 0x00000000,
    0xF0209020, 0xFFFFFFFF, 0x05010501,
    0xF0209024, 0x0000000F, 0x00000003,
    0xF0209028, 0x0000000F, 0x00000001,
    0xF020902C, 0x0000000F, 0x00000008,
    0xF0209030, 0x0000000F, 0x00000001,
    0xF0209034, 0x0000000F, 0x00000006,
    0xF0209038, 0x000000FF, 0x00000096,
    0xF020903C, 0x0000000F, 0x00000001,
    0xF0209200, 0x00000001, 0x00000000,
    0xF020920C, 0x00000003, 0x00000002,
    0xF0209240, 0x00000001, 0x00000000,
    0xF020924C, 0x00000003, 0x00000002,
    0xF0209250, 0x00000001, 0x00000000,
    0xF020925C, 0x00000003, 0x00000002,
    0xF0209260, 0x00000001, 0x00000000,
    0xF020926C, 0x00000003, 0x00000002,
    0xF0209270, 0x00000001, 0x00000000,
    0xF020927C, 0x00000003, 0x00000002,
    0xF0209290, 0x00000001, 0x00000000,
    0xF020929C, 0x00000003, 0x00000002,
    0xF02092A0, 0x00000001, 0x00000000,
    0xF02092B0, 0x00000003, 0x00000002,
    0xF02092B4, 0x00000001, 0x00000000,
    0xF02092C4, 0x00000003, 0x00000002,
    0xF1220000, 0x00000044, 0x00000044,
    0xF4000100, 0xFFFFFFFF, 0xFFFFFFFF,
    0xF4000110, 0x000003FF, 0x000003FF,
    0xF5000000, 0x00000BE1, 0x00000BE1,
    0xF6000000, 0x00000001, 0x00000000,
    0xF6000004, 0x00000001, 0x00000000,
    0xF6000008, 0x00000001, 0x00000000,
    0xF600000C, 0x00000001, 0x00000000,
    0xF6000010, 0x00000001, 0x00000001,
    0xF6000014, 0x00000001, 0x00000001,
    0xF7000000, 0xFFFFFFFF, 0xFFFFFFFF,
    0xF7000004, 0xFFFFFFFF, 0x00000000,
    0xF7000008, 0xFFFFFFFF, 0x00000000,
    0xF8000000, 0x00001111, 0x00000000,
    0xF80020EC, 0x00000001, 0x00000000 
};

const unsigned int *AP_CG_gs_idle = AP_CG_gs_idle_data;

unsigned int AP_CG_gs_idle_len = 192;

const unsigned int AP_CG_gs_clkon_data[] = {
 
    0xF0003008, 0x3FFFFFFF, 0x00000000,
    0xF0003010, 0x3FFFFFFF, 0x00000000,
    0xF0003018, 0x0C000000, 0x00000000,
    0xF000F640, 0x000000B4, 0x00000000,
    0xF000F690, 0x00000084, 0x00000000,
    0xF0012640, 0x00000084, 0x00000000,
    0xF8000000, 0x00001111, 0x00001111,
    0xF80020EC, 0x00000001, 0x00000001 
};

const unsigned int *AP_CG_gs_clkon = AP_CG_gs_clkon_data;

unsigned int AP_CG_gs_clkon_len = 192;

const unsigned int AP_CG_gs_talk_data[] = {
 
    0xF0000040, 0xFFFFFFFF, 0x00810106,
    0xF0000050, 0xFFFFFFFF, 0x80818180,
    0xF0000060, 0xFFFFFFFF, 0x81818180,
    0xF0000070, 0xFFFFFFFF, 0x01008181,
    0xF0000080, 0xFFFFFFFF, 0x81810300,
    0xF0000090, 0x00009787, 0x00000181,
    0xF0000200, 0x000003FF, 0x000003FF,
    0xF0000204, 0x00000011, 0x00000011,
    0xF0001040, 0x0081B1E3, 0x0001B1C2,
    0xF0001044, 0x00C1BFE3, 0x00000000,
    0xF0001048, 0x00C1B1E3, 0x0001B1C2,
    0xF0003008, 0x3FFFFFFF, 0x2FF7EFFD,
    0xF0003010, 0x3FFFFFFF, 0x00000000,
    0xF0003018, 0x0C000000, 0x0C000000,
    0xF000F640, 0x000000B4, 0x00000000,
    0xF000F690, 0x00000084, 0x00000000,
    0xF0012640, 0x00000084, 0x00000000,
    0xF0209008, 0x00000007, 0x00000000,
    0xF020900C, 0x0007FFFF, 0x00000000,
    0xF0209010, 0x0000000F, 0x00000000,
    0xF0209018, 0x0007FFFF, 0x00000000,
    0xF0209020, 0xFFFFFFFF, 0x05010501,
    0xF0209024, 0x0000000F, 0x00000003,
    0xF0209028, 0x0000000F, 0x00000001,
    0xF020902C, 0x0000000F, 0x00000008,
    0xF0209030, 0x0000000F, 0x00000001,
    0xF0209034, 0x0000000F, 0x00000006,
    0xF0209038, 0x000000FF, 0x00000096,
    0xF020903C, 0x0000000F, 0x00000001,
    0xF0209200, 0x00000001, 0x00000000,
    0xF020920C, 0x00000003, 0x00000002,
    0xF0209240, 0x00000001, 0x00000000,
    0xF020924C, 0x00000003, 0x00000002,
    0xF0209250, 0x00000001, 0x00000000,
    0xF020925C, 0x00000003, 0x00000002,
    0xF0209260, 0x00000001, 0x00000000,
    0xF020926C, 0x00000003, 0x00000002,
    0xF0209270, 0x00000001, 0x00000000,
    0xF020927C, 0x00000003, 0x00000002,
    0xF0209290, 0x00000001, 0x00000000,
    0xF020929C, 0x00000003, 0x00000002,
    0xF02092A0, 0x00000001, 0x00000000,
    0xF02092B0, 0x00000003, 0x00000002,
    0xF02092B4, 0x00000001, 0x00000000,
    0xF02092C4, 0x00000003, 0x00000002,
    0xF1220000, 0x00000044, 0x00000000,
    0xF4000100, 0xFFFFFFFF, 0xFFFF7BFC,
    0xF4000110, 0x000003FF, 0x0000030C,
    0xF5000000, 0x00000BE1, 0x00000BE1,
    0xF6000000, 0x00000001, 0x00000000,
    0xF6000004, 0x00000001, 0x00000000,
    0xF6000008, 0x00000001, 0x00000000,
    0xF600000C, 0x00000001, 0x00000000,
    0xF6000010, 0x00000001, 0x00000001,
    0xF6000014, 0x00000001, 0x00000001,
    0xF7000000, 0xFFFFFFFF, 0xFFFFFFFF,
    0xF7000004, 0xFFFFFFFF, 0x00000000,
    0xF7000008, 0xFFFFFFFF, 0x00000000,
    0xF8000000, 0x00001111, 0x00000000,
    0xF80020EC, 0x00000001, 0x00000000 
};

const unsigned int *AP_CG_gs_talk = AP_CG_gs_talk_data;

unsigned int AP_CG_gs_talk_len = 192;

const unsigned int AP_CG_gs_connsys_data[] = {
 
    0xF0000040, 0xFFFFFFFF, 0x00810106,
    0xF0000050, 0xFFFFFFFF, 0x80818180,
    0xF0000060, 0xFFFFFFFF, 0x81818180,
    0xF0000070, 0xFFFFFFFF, 0x81808181,
    0xF0000080, 0xFFFFFFFF, 0x81810300,
    0xF0000090, 0x00009787, 0x00008181,
    0xF0000200, 0x000003FF, 0x000003FF,
    0xF0000204, 0x00000011, 0x00000011,
    0xF0001040, 0x0081B1E3, 0x0001B1E2,
    0xF0001044, 0x00C1BFE3, 0x00000000,
    0xF0001048, 0x00C1B1E3, 0x0001B1E2,
    0xF0003008, 0x3FFFFFFF, 0x2FF7EFFD,
    0xF0003010, 0x3FFFFFFF, 0x00000000,
    0xF0003018, 0x0C000000, 0x0C000000,
    0xF000F640, 0x000000B4, 0x00000000,
    0xF000F690, 0x00000084, 0x00000000,
    0xF0012640, 0x00000084, 0x00000000,
    0xF0209008, 0x00000007, 0x00000000,
    0xF020900C, 0x0007FFFF, 0x00000000,
    0xF0209010, 0x0000000F, 0x00000000,
    0xF0209018, 0x0007FFFF, 0x00000000,
    0xF0209020, 0xFFFFFFFF, 0x05010501,
    0xF0209024, 0x0000000F, 0x00000003,
    0xF0209028, 0x0000000F, 0x00000001,
    0xF020902C, 0x0000000F, 0x00000008,
    0xF0209030, 0x0000000F, 0x00000001,
    0xF0209034, 0x0000000F, 0x00000006,
    0xF0209038, 0x000000FF, 0x00000096,
    0xF020903C, 0x0000000F, 0x00000001,
    0xF0209200, 0x00000001, 0x00000000,
    0xF020920C, 0x00000003, 0x00000002,
    0xF0209240, 0x00000001, 0x00000000,
    0xF020924C, 0x00000003, 0x00000002,
    0xF0209250, 0x00000001, 0x00000000,
    0xF020925C, 0x00000003, 0x00000002,
    0xF0209260, 0x00000001, 0x00000000,
    0xF020926C, 0x00000003, 0x00000002,
    0xF0209270, 0x00000001, 0x00000000,
    0xF020927C, 0x00000003, 0x00000002,
    0xF0209290, 0x00000001, 0x00000000,
    0xF020929C, 0x00000003, 0x00000002,
    0xF02092A0, 0x00000001, 0x00000000,
    0xF02092B0, 0x00000003, 0x00000002,
    0xF02092B4, 0x00000001, 0x00000000,
    0xF02092C4, 0x00000003, 0x00000002,
    0xF1220000, 0x00000044, 0x00000044,
    0xF4000100, 0xFFFFFFFF, 0xFFFFFFFF,
    0xF4000110, 0x000003FF, 0x000003FF,
    0xF5000000, 0x00000BE1, 0x00000BE1,
    0xF6000000, 0x00000001, 0x00000000,
    0xF6000004, 0x00000001, 0x00000000,
    0xF6000008, 0x00000001, 0x00000000,
    0xF600000C, 0x00000001, 0x00000000,
    0xF6000010, 0x00000001, 0x00000001,
    0xF6000014, 0x00000001, 0x00000001,
    0xF7000000, 0xFFFFFFFF, 0xFFFFFFFF,
    0xF7000004, 0xFFFFFFFF, 0x00000000,
    0xF7000008, 0xFFFFFFFF, 0x00000000,
    0xF8000000, 0x00001111, 0x00000000,
    0xF80020EC, 0x00000001, 0x00000000 
};

const unsigned int *AP_CG_gs_connsys = AP_CG_gs_connsys_data;

unsigned int AP_CG_gs_connsys_len = 192;

const unsigned int AP_CG_gs_mne_data[] = {
 
    0xF0001040, 0x0040CF82, 0x00000000,
    0xF0001044, 0x0040CF82, 0x00000000,
    0xF0001048, 0x0040CF82, 0x00000000,
    0xF3FFF000, 0x0000000E, 0x00000000,
    0xF3FFF004, 0x0000000E, 0x00000000,
    0xF3FFF008, 0x0000000E, 0x00000000,
    0xF3FFF00C, 0x00000001, 0x00000000,
    0xF4000100, 0xFFFFFFFF, 0x00000000,
    0xF4000110, 0x000003FF, 0x00000000,
    0xF6000000, 0x00000001, 0x00000000,
    0xF6000004, 0x00000001, 0x00000000,
    0xF6000008, 0x00000001, 0x00000000,
    0xF600000C, 0x00000001, 0x00000000,
    0xF7000000, 0xFFFFFFFF, 0x00000000,
    0xF7000004, 0xFFFFFFFF, 0x00000000,
    0xF7000008, 0xFFFFFFFF, 0x00000000,
    0xF8000000, 0x00001111, 0x00000000,
    0xF80020EC, 0x00000001, 0x00000000 
};

const unsigned int *AP_CG_gs_mne = AP_CG_gs_mne_data;

unsigned int AP_CG_gs_mne_len = 192;

const unsigned int AP_CG_gs_datalink_data[] = {
 
    0xF0000040, 0xFFFFFFFF, 0x00810106,
    0xF0000050, 0xFFFFFFFF, 0x80818180,
    0xF0000060, 0xFFFFFFFF, 0x81818180,
    0xF0000070, 0xFFFFFFFF, 0x81808181,
    0xF0000080, 0xFFFFFFFF, 0x81810300,
    0xF0000090, 0x00009787, 0x00008181,
    0xF0000200, 0x000003FF, 0x000003FF,
    0xF0000204, 0x00000011, 0x00000011,
    0xF0001040, 0x0081B1E3, 0x0001B1E2,
    0xF0001044, 0x00C1BFE3, 0x00000000,
    0xF0001048, 0x00C1B1E3, 0x0001B1E2,
    0xF0003008, 0x3FFFFFFF, 0x2FF7EFFD,
    0xF0003010, 0x3FFFFFFF, 0x00000000,
    0xF0003018, 0x0C000000, 0x0C000000,
    0xF000F640, 0x000000B4, 0x00000000,
    0xF000F690, 0x00000084, 0x00000000,
    0xF0012640, 0x00000084, 0x00000000,
    0xF0209008, 0x00000007, 0x00000000,
    0xF020900C, 0x0007FFFF, 0x00000000,
    0xF0209010, 0x0000000F, 0x00000000,
    0xF0209018, 0x0007FFFF, 0x00000000,
    0xF0209020, 0xFFFFFFFF, 0x05010501,
    0xF0209024, 0x0000000F, 0x00000003,
    0xF0209028, 0x0000000F, 0x00000001,
    0xF020902C, 0x0000000F, 0x00000008,
    0xF0209030, 0x0000000F, 0x00000001,
    0xF0209034, 0x0000000F, 0x00000006,
    0xF0209038, 0x000000FF, 0x00000096,
    0xF020903C, 0x0000000F, 0x00000001,
    0xF0209200, 0x00000001, 0x00000000,
    0xF020920C, 0x00000003, 0x00000002,
    0xF0209240, 0x00000001, 0x00000000,
    0xF020924C, 0x00000003, 0x00000002,
    0xF0209250, 0x00000001, 0x00000000,
    0xF020925C, 0x00000003, 0x00000002,
    0xF0209260, 0x00000001, 0x00000000,
    0xF020926C, 0x00000003, 0x00000002,
    0xF0209270, 0x00000001, 0x00000000,
    0xF020927C, 0x00000003, 0x00000002,
    0xF0209290, 0x00000001, 0x00000000,
    0xF020929C, 0x00000003, 0x00000002,
    0xF02092A0, 0x00000001, 0x00000000,
    0xF02092B0, 0x00000003, 0x00000002,
    0xF02092B4, 0x00000001, 0x00000000,
    0xF02092C4, 0x00000003, 0x00000002,
    0xF1220000, 0x00000044, 0x00000044,
    0xF4000100, 0xFFFFFFFF, 0xFFFFFFFF,
    0xF4000110, 0x000003FF, 0x000003FF,
    0xF5000000, 0x00000BE1, 0x00000BE1,
    0xF6000000, 0x00000001, 0x00000000,
    0xF6000004, 0x00000001, 0x00000000,
    0xF6000008, 0x00000001, 0x00000000,
    0xF600000C, 0x00000001, 0x00000000,
    0xF6000010, 0x00000001, 0x00000001,
    0xF6000014, 0x00000001, 0x00000001,
    0xF7000000, 0xFFFFFFFF, 0xFFFFFFFF,
    0xF7000004, 0xFFFFFFFF, 0x00000000,
    0xF7000008, 0xFFFFFFFF, 0x00000000,
    0xF8000000, 0x00001111, 0x00000000,
    0xF80020EC, 0x00000001, 0x00000000 
};

const unsigned int *AP_CG_gs_datalink = AP_CG_gs_datalink_data;

unsigned int AP_CG_gs_datalink_len = 192;

const unsigned int AP_CG_gs_vr_data[] = {
 
    0xF0000040, 0xFFFFFFFF, 0x01810101,
    0xF0000050, 0xFFFFFFFF, 0x00018100,
    0xF0000060, 0xFFFFFFFF, 0x81818180,
    0xF0000070, 0xFFFFFFFF, 0x01008101,
    0xF0000080, 0xFFFFFFFF, 0x81010300,
    0xF0000090, 0x00009787, 0x00000181,
    0xF0000200, 0x000003FF, 0x000003FF,
    0xF0000204, 0x00000011, 0x00000011,
    0xF0001040, 0x0081B1E3, 0x0001B040,
    0xF0001044, 0x00C1BFE3, 0x00000000,
    0xF0001048, 0x00C1B1E3, 0x0001B040,
    0xF0003008, 0x3FFFFFFF, 0x2FF7EFFD,
    0xF0003010, 0x3FFFFFFF, 0x00000000,
    0xF0003018, 0x0C000000, 0x0C000000,
    0xF000F640, 0x000000B4, 0x00000000,
    0xF000F690, 0x00000084, 0x00000000,
    0xF0012640, 0x00000084, 0x00000000,
    0xF0209008, 0x00000007, 0x00000000,
    0xF020900C, 0x0007FFFF, 0x00000000,
    0xF0209010, 0x0000000F, 0x00000000,
    0xF0209018, 0x0007FFFF, 0x00000000,
    0xF0209020, 0xFFFFFFFF, 0x05010501,
    0xF0209024, 0x0000000F, 0x00000003,
    0xF0209028, 0x0000000F, 0x00000001,
    0xF020902C, 0x0000000F, 0x00000008,
    0xF0209030, 0x0000000F, 0x00000001,
    0xF0209034, 0x0000000F, 0x00000006,
    0xF0209038, 0x000000FF, 0x00000096,
    0xF020903C, 0x0000000F, 0x00000001,
    0xF0209200, 0x00000001, 0x00000000,
    0xF020920C, 0x00000003, 0x00000002,
    0xF0209240, 0x00000001, 0x00000000,
    0xF020924C, 0x00000003, 0x00000002,
    0xF0209250, 0x00000001, 0x00000001,
    0xF020925C, 0x00000003, 0x00000001,
    0xF0209260, 0x00000001, 0x00000001,
    0xF020926C, 0x00000003, 0x00000001,
    0xF0209270, 0x00000001, 0x00000000,
    0xF020927C, 0x00000003, 0x00000002,
    0xF0209290, 0x00000001, 0x00000000,
    0xF020929C, 0x00000003, 0x00000002,
    0xF02092A0, 0x00000001, 0x00000000,
    0xF02092B0, 0x00000003, 0x00000002,
    0xF02092B4, 0x00000001, 0x00000000,
    0xF02092C4, 0x00000003, 0x00000002,
    0xF1220000, 0x00000044, 0x00000000,
    0xF4000100, 0xFFFFFFFF, 0xFFFF7BFC,
    0xF4000110, 0x000003FF, 0x0000030C,
    0xF5000000, 0x00000BE1, 0x00000000,
    0xF6000000, 0x00000001, 0x00000000,
    0xF6000004, 0x00000001, 0x00000000,
    0xF6000008, 0x00000001, 0x00000000,
    0xF600000C, 0x00000001, 0x00000000,
    0xF6000010, 0x00000001, 0x00000001,
    0xF6000014, 0x00000001, 0x00000001,
    0xF7000000, 0xFFFFFFFF, 0xFFFFFFFF,
    0xF7000004, 0xFFFFFFFF, 0x00000000,
    0xF7000008, 0xFFFFFFFF, 0x00000000,
    0xF8000000, 0x00001111, 0x00000011,
    0xF80020EC, 0x00000001, 0x00000001 
};

const unsigned int *AP_CG_gs_vr = AP_CG_gs_vr_data;

unsigned int AP_CG_gs_vr_len = 192;

