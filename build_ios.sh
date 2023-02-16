if [ -z "$APPLE_TEAM_ID" ]; then
  echo "Please set $APPLE_TEAM_ID with your team id"
  exit 1
fi

function build_for_ios {
  PLATFORM=$1

  mkdir -p build_${PLATFORM} && cd build_${PLATFORM}
  cmake .. -G Xcode -DBUILD_PLATFORM=ios -DCMAKE_TOOLCHAIN_FILE=../ios-cmake/ios.toolchain.cmake -DPLATFORM=${PLATFORM} -DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM=${APPLE_TEAM_ID}
  cmake --build . --config Release
  cd ..
}

function clean_build_platform {
  PLATFORM=$1

  rm -rf build_${PLATFORM}
}

build_for_ios OS64
build_for_ios SIMULATOR64
build_for_ios MAC

mkdir -p build_ios && cd build_ios
xcodebuild \
	-create-xcframework \
	-library ../build_OS64/Release-iphoneos/libnice7z_arm64.dylib \
	-library ../build_SIMULATOR64/Release-iphonesimulator/libnice7z_x86_64.dylib \
	-library ../build_MAC/Release/libnice7z_x86_64.dylib \
	-output nice7z.xcframework
cd ..

clean_build_platform OS64
clean_build_platform SIMULATOR64
clean_build_platform MAC
