#!/bin/bash
# set -ax
GIT_LINUX_PATH=".linux"
GIT_LINUX_LIB_THERMAL="tools/lib/thermal"
GIT_REPO=https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git
GIT_REFERENCE=""

while getopts ":r:h" o; do
    case "${o}" in
        r)
            GIT_REFERENCE="--reference=${OPTARG}"
            ;;
	h)
	    echo "$0 [-r <git reference>]"
	    exit 0
	    ;;
        *)
	    echo "Unknown parameter"
	    exit 1
            ;;
    esac
done

shift $((OPTIND-1))

if [ ! -d $GIT_LINUX_PATH ]; then
    echo " * Cloning $GIT_REPO"
    git clone -j 8 $GIT_REFERENCE $GIT_REPO $GIT_LINUX_PATH
fi

pushd $GIT_LINUX_PATH || exit 1

# Update the official Linus' tree
echo " * Updating $GIT_REPO"
git pull

popd > /dev/null

ln -sf $PWD/.linux/include/uapi/linux include

INCLUDE_PATH=src/include
LIB_PATH=src/lib

declare -A TARGETS

TARGETS["include"]="$GIT_LINUX_PATH/$GIT_LINUX_LIB_THERMAL/include/*.[ch]"
TARGETS["src"]="$GIT_LINUX_PATH/$GIT_LINUX_LIB_THERMAL/*.[ch]"

echo " * Updating source files"
for TARGET in ${!TARGETS[@]}; do
    for SRC in $(ls ${TARGETS[${TARGET}]}); do
	    diff -q $SRC $TARGET 2> /dev/null
	    if [ "$?" != "0" ]; then
		echo " - Copying $SRC --> $TARGET"
		cp $SRC $TARGET
	    fi
    done
done

echo " * Patching source files"
for PATCH in $(cat patches/patches.list); do
    patch -p1 -N -r - < patches/$PATCH
done

echo " * Installing kernel headers in 'include'"
make -C .linux ARCH=arm64 INSTALL_HDR_PATH=$PWD headers_install

echo "Done"
