include $(srctree)/drivers/misc/mediatek/Makefile.custom

obj-$(CONFIG_MTK_VIBRATOR) := vibrator_drv.o
obj-$(CONFIG_VIB_TRIGGERS) += vib-triggers.o

obj-y += $(subst ",,$(CONFIG_MTK_PLATFORM))/

#ccflags-y := -Idrivers/staging/android/
