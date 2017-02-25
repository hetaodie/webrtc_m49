PROJECTDIR=$(pwd)
FRAMEWORK_VERSION="1.0.0"
LIBS_DIR="$PROJECTDIR/src/out_ios_arm64_armv7/Release-iphoneos"
FRAMEWORK_DIR="$PROJECTDIR/src/out_ios_arm64_armv7/Release-iphoneos/framework"
LIB_NAME="libWebRTC"
FRAMEWORK_NAME="WebRTC.framework"

function exec_libtool() {
  echo "Running libtool"
  libtool -static -v -o $@
}

function exec_strip() {
  echo "Running strip"
  strip -S -X $@
} 

function create_directory_if_not_found() {
    if [ ! -d "$1" ];
    then
        mkdir -v "$1"
    fi
} 

function create_ios_framework_for_configuration () {
    rm -rf $FRAMEWORK_DIR/$FRAMEWORK_NAME
    mkdir -p $FRAMEWORK_DIR/$FRAMEWORK_NAME/Versions/A/Headers
    cp $PROJECTDIR/src/talk/app/webrtc/objc/public/*.h $FRAMEWORK_DIR/$FRAMEWORK_NAME/Versions/A/Headers
    cp $FRAMEWORK_DIR/$LIB_NAME.a $FRAMEWORK_DIR/$FRAMEWORK_NAME/Versions/A/WebRTC

    echo $FRAMEWORK_VERSION >> $FRAMEWORK_DIR/$FRAMEWORK_NAME/Version.txt

    pushd $FRAMEWORK_DIR/$FRAMEWORK_NAME/Versions
    ln -sfh A Current
    popd
    pushd $FRAMEWORK_DIR/$FRAMEWORK_NAME
    ln -sfh Versions/Current/Headers Headers
    ln -sfh Versions/Current/WebRTC WebRTC
    popd
}

function archive_lib() {
    echo "archive_lib begin..."
    create_directory_if_not_found $FRAMEWORK_DIR
    rm -f $LIBS_DIR/libapprtc_common.a
    rm -f $LIBS_DIR/libapprtc_signaling.a
    rm -f $LIBS_DIR/libsocketrocket.a
    exec_libtool "$FRAMEWORK_DIR/$LIB_NAME.a" $LIBS_DIR/*.a
    exec_strip "$FRAMEWORK_DIR/$LIB_NAME.a"
    echo "archive_lib archive ok"
} 

function archive_framework() {
	echo "archive_lib begin..."
	create_directory_if_not_found $FRAMEWORK_DIR
	exec_libtool "$FRAMEWORK_DIR/$LIB_NAME.a" $LIBS_DIR/*.a
	exec_strip "$FRAMEWORK_DIR/$LIB_NAME.a"
	echo "archive_lib archive ok"

    echo "archive_framework create framework..."
    create_ios_framework_for_configuration $SDK_CONFIGURATION
    echo "archive_framework create framework ok"
    echo "archive_framework end."
} 