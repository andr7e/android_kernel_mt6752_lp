#include <asm/uaccess.h>

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/err.h>
#include <mach/mt_typedefs.h>

unsigned long (*mtk_thermal_get_gpu_loading_fp)(void) = NULL;


static int __init mtk_thermal_platform_init(void)
{
    int err = 0;
    return err;
}

static void __exit mtk_thermal_platform_exit(void)
{

}



EXPORT_SYMBOL(mtk_thermal_get_gpu_loading_fp);
module_init(mtk_thermal_platform_init);
module_exit(mtk_thermal_platform_exit);


