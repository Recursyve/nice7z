# Nice 7z

Nice 7z is a wrapper of https://github.com/7zip-dev/7zip source code to extract a .7z archive to a specific folder. This was build to be used on Android and iOS.

## Build

### Android

You can build the source code as shared libraries for Android with `build_android.sh` script.

You need to provide `ANDROID_NDK` as an environment variable.

```sh
$ ./build_android.sh
```

or

```sh
env ANDROID_NDK=/path/to/android/ndk ./build_android.sh
```

The shared libraries will be in the **prebuilt/release** directory.

### iOS

You can build the source code for iOS with build_ios.sh script.

You need to provide `APPLE_TEAM_ID` as an environment variable. You can find it on your apple developper account.

```sh
env APPLE_TEAM_ID=my_team_id ./build_ios.sh
```

The **xcframework** will be in the **build_ios** directory

## How to use it

### Dart ffi (Flutter)

```dart
typedef ProgressCallback = Void Function(Double);
typedef Extract7zArchiveC = Int32 Function(
  Pointer<Utf8> source,
  Pointer<Utf8> dest,
  Pointer<NativeFunction<ProgressCallback>> progress,
);
typedef Extract7zArchiveDart = int Function(
  Pointer<Utf8> source,
  Pointer<Utf8> dest,
  Pointer<NativeFunction<ProgressCallback>> progress,
);

class Nice7z {
  static late DynamicLibrary _nice7z;
  static late Extract7zArchiveDart _nice7zExtract;

  static Nice7z? _instance;

  Nice7z._();

  factory Nice7z() => _instance ??= Nice7z._();

  void init() {
    _nice7z = Platform.isAndroid ? DynamicLibrary.open('libnice7z.so') : DynamicLibrary.process();

    _nice7zExtract =
        _nice7z.lookup<NativeFunction<Extract7zArchiveC>>("extract_7z_archive").asFunction<Extract7zArchiveDart>();
  }
}

```
