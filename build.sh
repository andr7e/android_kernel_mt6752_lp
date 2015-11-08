#script compile kernel by andr7e
#setting build

toolchain="$HOME/kernel_build/aarch64-linux-android-4.9/bin/aarch64-linux-android-"
source_path=`pwd`
output_path="$source_path/out"

projectName="$1"

export CROSS_COMPILE=$toolchain

export ARCH=arm64
export TARGET_ARCH=arm64
export ARCH_MTK_PLATFORM=mt6752

echo "$projectName";


if [ ! -d "$output_path" ]; then
   mkdir "$output_path"
fi

echo "${output_path}"

makeflags+=" O=${output_path}"

#make clean

make ${makeflags} "${projectName}_defconfig"

make ${makeflags} Image.gz-dtb

echo "**** Generate download images ****"

mkimg="${source_path}/tools/mkimage"

if [ ! -x ${mkimg} ]; then chmod a+x ${mkimg}; fi

${mkimg} "${output_path}/arch/arm64/boot/Image.gz-dtb" KERNEL > "${output_path}/kernel_${projectName}.bin"