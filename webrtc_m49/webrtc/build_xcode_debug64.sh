#dir define
PROJECTDIR=$(pwd)
SRCDIR=${PROJECTDIR}/src

#build libs for armv7 device
cd ${SRCDIR}
export GYP_CROSSCOMPILE=1
export GYP_DEFINES="OS=ios target_arch=arm64 target_subarch=arm64"
export GYP_GENERATOR_FLAGS="xcode_project_version=3.2 xcode_ninja_target_pattern=All_iOS xcode_ninja_executable_target_pattern=AppRTCDemo|libjingle_peerconnection_unittest|libjingle_peerconnection_objc_test output_dir=out_ios64"
export GYP_GENERATORS="ninja,xcode-ninja"
webrtc/build/gyp_webrtc
ninja -C out_ios64/Debug-iphoneos AppRTCDemo

