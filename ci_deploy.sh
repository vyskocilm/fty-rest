#!/usr/bin/env bash

################################################################################
#  This file is copied from fty-core/ci_deploy.sh                              #
################################################################################

set -x
set -e

if [ "$BUILD_TYPE" == "default" ]; then
    # Tell travis to deploy all files in dist
    mkdir dist
    export FTY_REST_DEPLOYMENT=dist/*
    # Move archives to dist
    mv *.tar.gz dist
    mv *.zip dist
    # Generate hash sums
    cd dist
    md5sum *.zip *.tar.gz > MD5SUMS
    sha1sum *.zip *.tar.gz > SHA1SUMS
    cd -
elif [ "$BUILD_TYPE" == "bindings" ] && [ "$BINDING" == "jni" ]; then
    ( cd bindings/jni && TERM=dumb PKG_CONFIG_PATH=/tmp/lib/pkgconfig ./gradlew clean bintrayUpload )
    cp bindings/jni/android/fty_rest-android.jar fty_rest-android-1.0.0.jar
    export FTY_REST_DEPLOYMENT=fty_rest-android-1.0.0.jar
else
    export FTY_REST_DEPLOYMENT=""
fi
