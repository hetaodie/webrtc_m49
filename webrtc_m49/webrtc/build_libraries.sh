#dir define
PROJECTDIR=$(pwd)
SRCDIR=${PROJECTDIR}/src
ARMV7DIR=${SRCDIR}/out_ios/Release-iphoneos
ARM64DIR=${SRCDIR}/out_ios64/Release-iphoneos
UNIVERSALDIR=${SRCDIR}/out_ios_arm64_armv7/Release-iphoneos

#build libs for armv7 device
cd ${SRCDIR}
export GYP_CROSSCOMPILE=1
export GYP_DEFINES="OS=ios target_arch=arm arm_version=7"
export GYP_GENERATOR_FLAGS="output_dir=out_ios"
export GYP_GENERATORS=ninja
webrtc/build/gyp_webrtc
ninja -C out_ios/Release-iphoneos AppRTCDemo

#build libs for arm64 device
cd ${SRCDIR}
export GYP_CROSSCOMPILE=1
export GYP_DEFINES="OS=ios target_arch=arm64 target_subarch=arm64"
export GYP_GENERATOR_FLAGS="output_dir=out_ios64"
export GYP_GENERATORS=ninja
webrtc/build/gyp_webrtc
ninja -C out_ios64/Release-iphoneos AppRTCDemo

#merge armv7 and arm64 libs
if [ -d "${UNIVERSALDIR}" ]; then
	rm -rf ${UNIVERSALDIR}
fi
mkdir -p ${UNIVERSALDIR}

echo "//////////////////////////////////////////"
echo "Start to generate armv7 and arm64 lib files for webrtc."

count=0

cd ${ARMV7DIR}
for file in *.a
do
	let "count+=1"
	if [ -f "${ARM64DIR}/$file" ]; then
		xcrun -sdk iphoneos lipo -output ${UNIVERSALDIR}/$file  -create -arch armv7 ${ARMV7DIR}/$file -arch arm64 ${ARM64DIR}/$file
	else
		cp ${ARMV7DIR}/$file ${UNIVERSALDIR}
	fi
done

cd ${ARM64DIR}
for file in *.a
do
	if [ ! -f "${ARMV7DIR}/$file" ]; then
		let "count+=1"
		cp ${ARM64DIR}/$file ${UNIVERSALDIR}
	fi
done

echo "There are $count lib files generated."
