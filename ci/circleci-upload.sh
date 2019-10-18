#!/usr/bin/env bash

#
# Upload the .tar.gz and .xml artifacts to cloudsmith.
#


REPO_TEST='alec-leamas/opencpn-plugins-unstable'
REPO_RELEASE='alec-leamas/opencpn-plugins-stable'


if [ -z "$CIRCLECI" ]; then
    exit 0;
fi

#branch=$(git symbolic-ref --short HEAD)
#if [ "$branch" != 'master' ]; then
#    echo "Not on master branch, skipping deployment."
#    exit 0
#fi

if [ -z "$CLOUDSMITH_API_KEY" ]; then
    echo 'Cannot deploy to cloudsmith, missing $CLOUDSMITH_API_KEY'
    exit 0
fi

echo "Using \$CLOUDSMITH_API_KEY: ${CLOUDSMITH_API_KEY:0:4}..."

set -xe

if pyenv versions 2>&1 >/dev/null; then
    pyenv global 3.7.0
    python -m pip install cloudsmith-cli
    pyenv rehash
elif dnf --version 2>&1 >/dev/null; then
    sudo dnf install python3-pip python3-setuptools
    sudo python3 -m pip install -q cloudsmith-cli
elif apt-get --version 2>&1 >/dev/null; then
    sudo apt-get install python3-pip python3-setuptools
    sudo python3 -m pip install -q cloudsmith-cli
else
    sudo -H python3 -m ensurepip
    sudo -H python3 -m pip install -q setuptools
    sudo -H python3 -m pip install -q cloudsmith-cli
fi


BUILD_ID=${CIRCLE_BUILD_NUM:-1}
commit=$(git rev-parse --short=7 HEAD) || commit="unknown"
now=$(date --rfc-3339=seconds) || now=$(date)

tarball=$(ls $HOME/project/build/*.tar.gz)
xml=$(ls $HOME/project/build/*.xml)
sudo chmod 666 $xml
echo '<!--'" Date: $now Commit: $commit Build nr: $BUILD_ID -->" >> $xml

HERE=$(dirname $(readlink -fn $0))
source ${HERE}/../build/pkg_version.sh
VERSION="${VERSION}+${BUILD_ID}.${commit}"

REPO="$REPO_TEST"
TAG=$(git tag --points-at HEAD)
if [ -n "$TAG" ]; then
    VERSION="$TAG"
    REPO="$REPO_RELEASE"
fi
CS_STD_OPTS="--republish --no-wait-for-sync --version $VERSION"
cloudsmith push raw $CS_STD_OPTS \
    --name squiddio-${PKG_TARGET}-${PKG_TARGET_VERSION}-tarball \
    --summary "Squiddio installation tarball for automatic installations." \
    $REPO $tarball
cloudsmith push raw $CS_STD_OPTS \
    --name squiddio-${PKG_TARGET}-${PKG_TARGET_VERSION}-metadata \
    --summary "Squiddio installation metadata for automatic installations." \
    $REPO $xml

if [ "${PKG_TARGET}" = "ubuntu" ]; then
    package=$(ls $HOME/project/build/radar*.deb)
    cloudsmith push deb $CS_STD_OPTS \
        --name squiddio-debian-package \
        --summary "Squiddio .deb package for manual installations." \
    $REPO/${PKG_TARGET}/${PKG_TARGET_VERSION} $package
elif [ "${PKG_TARGET}" = "mingw" ]; then
    package=$(ls $HOME/project/build/*.exe)
    cloudsmith push raw  $CS_STD_OPTS \
        --name squiddio-windows-mingw-installer \
        --summary "Squiddio windows mingw installer for manual installations" \
    $REPO $package
fi
