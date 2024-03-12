#!/bin/bash
# set -ax
GIT_LINUX_PATH=".linux"
GIT_REPO=https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git

if [ ! -d $GIT_LINUX_PATH ]; then
    echo " * Cloning $GIT_REPO"
    git clone --depth 1 --sparse $GIT_REPO $GIT_LINUX_PATH
fi

pushd $GIT_LINUX_PATH > /dev/null

# Update the official Linus' tree
echo " * Updating $GIT_REPO"
git pull

GIT_LIB_THERMAL="tools/lib/thermal"
GIT_THERMAL_LIB="tools/thermal/lib"
GIT_SPARSE_SET="$GIT_LIB_THERMAL $GIT_THERMAL_LIB"

echo " * Sparse checkout $GIT_SPARSE_SET"
git sparse-checkout set $GIT_SPARSE_SET

popd > /dev/null

INCLUDE_PATH=src/include
LIB_PATH=src/lib
THERMAL_MANAGER_PATH=src/thermal-manager

declare -A TARGETS

TARGETS["include"]="$GIT_LINUX_PATH/$GIT_LIB_THERMAL/include/*.[ch]"
TARGETS["src"]="$GIT_LINUX_PATH/$GIT_LIB_THERMAL/*.[ch] $GIT_LINUX_PATH/$GIT_LIB_THERMAL/Makefile"

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
    patch -p1 < patches/$PATCH
done

echo "Done"
