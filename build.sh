#script compile kernel by andr7e

toolchain="$HOME/kernel_build/aarch64-linux-android-4.9/bin/aarch64-linux-android-"
source_path=`pwd`
#kernel_path="$source_path/kernel-3.10"
kernel_path="$source_path"
output_path="$source_path/out"

mtktools_path="$source_path/mtktools"

projectName="$1"

export CROSS_COMPILE=$toolchain

export ARCH=arm64
export TARGET_ARCH=arm64
export ARCH_MTK_PLATFORM=mt6752

build_kernel()
{ 
	echo "$projectName";

	cd "$kernel_path"

	if [ ! -d "$output_path" ]; then
	   mkdir "$output_path"
	fi

	echo "${output_path}"

	makeflags+=" O=${output_path}"

	#make clean

	make ${makeflags} "${projectName}_defconfig"

	make ${makeflags} Image.gz-dtb

	echo "**** Generate download images ****"

	mkimg="${kernel_path}/tools/mkimage"

	if [ ! -x ${mkimg} ]; then chmod a+x ${mkimg}; fi

	${mkimg} "${output_path}/arch/arm64/boot/Image.gz-dtb" KERNEL > "${output_path}/kernel_${projectName}.bin"
}

repack_recovery()
{
   projectName="$1";
   type="$2";

   echo "$projectName";

   projectKernelName="${output_path}/kernel_${projectName}.bin"

   echo  "repacking $type..."
   dest_path="$source_path/build/recovery"

   if [ ! -d "$dest_path" ]; then
      mkdir "$dest_path"
   fi

   cd "$mtktools_path"

   if [ -f "$mtktools_path/recovery-$type.img" ]; then
      ./unpack.pl "recovery-$type.img"
      ./repack.pl -recovery "$projectKernelName" ramdisk/ "$dest_path/recovery.img"

      cd "$dest_path"
      zip -r recovery .
      mv "$dest_path/recovery.zip" "$source_path/build/$projectName-$type-recovery.zip"
      rm "$dest_path/recovery.img"
   else
      echo "File $mtktools_path/recovery-$type.img does not exist."
   fi
}

recovery_param="recovery"

if [ "$1" = "list" ];
then
   echo "Available projects:"

else
   if [ "$2" = "$recovery_param" ];
   then
       repack_recovery "$1" "cwm"
   else
       build_kernel "$1"
   fi
fi
