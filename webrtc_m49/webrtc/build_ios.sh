PROJECTDIR=$(pwd)

cd ${PROJECTDIR}
./build_libraries.sh
cd ${PROJECTDIR}
source build_framework.sh
archive_lib