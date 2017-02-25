PROJECTDIR=$(pwd)
cd ${PROJECTDIR}

source build.sh
export WEBRTC_DEBUG=false
export WEBRTC_ARCH=armv7 #or armv8, x86, or x86_64
prepare_gyp_defines &&
execute_build
