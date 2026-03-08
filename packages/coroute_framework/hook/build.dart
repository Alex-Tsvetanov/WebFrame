import 'dart:io';
import 'package:hooks/hooks.dart';
import 'package:code_assets/code_assets.dart';
import 'package:path/path.dart' as p;

void main(List<String> args) async {
  await build(args, (input, output) async {
    // Config files are written by CorouteApp.cmake POST_BUILD, one per project:
    //   .coroute_lib_path.<TARGET_NAME>
    //   Line 1: absolute path to the built libcoroute_app shared library.
    //   Line 2: absolute path to the Flutter project root (.flutter/ dir).
    //
    // Multiple projects can build concurrently without collision since each
    // writes to its own file. The hook identifies which config belongs to the
    // current invocation by matching line 2 against the Flutter project root
    // derived from input.outputDirectory:
    //   <flutter_root>/.dart_tool/hooks_runner/shared/coroute_framework/build/<hash>/
    //   => 6 levels up = flutter_root
    final flutterRoot = _deriveFlutterRoot(input.outputDirectory);

    // Scan all .coroute_lib_path.* sibling files in the package root.
    final packageDir = Directory.fromUri(input.packageRoot);
    final configFiles = packageDir
        .listSync()
        .whereType<File>()
        .where((f) => p.basename(f.path).startsWith('.coroute_lib_path.'))
        .toList();

    // Track all config files as dependencies so the hook reruns if any change.
    for (final f in configFiles) {
      output.addDependency(f.uri);
    }

    if (configFiles.isEmpty) {
      // Not a Coroute-CMake build (package used standalone). Skip.
      return;
    }

    // Find the config whose flutter project root (line 2) matches ours.
    File? matchedConfig;
    String? libPath;
    String? flutterProjectDir;

    for (final f in configFiles) {
      final lines = f.readAsStringSync().trim().split('\n');
      if (lines.length < 2) continue;
      final candidateDir = lines[1].trim();
      // Normalise both paths before comparing (resolve symlinks if needed).
      if (_samePath(candidateDir, flutterRoot)) {
        matchedConfig = f;
        libPath = lines[0].trim();
        flutterProjectDir = candidateDir;
        break;
      }
    }

    if (matchedConfig == null) {
      // No config matches this flutter project — not yet built via CMake. Skip.
      return;
    }

    // -------------------------------------------------------------------------
    // 1. NativeAssets: bundle the pre-built libcoroute_app
    // -------------------------------------------------------------------------
    if (input.config.buildCodeAssets) {
      final libFile = File(libPath!);
      if (!libFile.existsSync()) {
        throw Exception(
          '[coroute_framework hook] Library not found at: $libPath\n'
          'Ensure the CMake target has been built before running flutter.',
        );
      }

      final os = input.config.code.targetOS;
      // libName = on-disk filename (platform-prefixed/suffixed as the OS requires)
      final libName = os == OS.windows
          ? 'coroute_app.dll'
          : (os == OS.macOS || os == OS.iOS)
              ? 'libcoroute_app.dylib'
              : 'libcoroute_app.so';

      final outLib = File.fromUri(input.outputDirectory.resolve(libName));
      libFile.copySync(outLib.path);

      // Copy @loader_path-relative dependencies (e.g. OpenSSL dylibs rewritten
      // by CorouteApp.cmake) into the same output directory so Flutter bundles
      // them inside coroute_app.framework/Versions/A/ alongside the binary.
      // This makes @loader_path resolution work inside the sandboxed app bundle.
      if (os == OS.macOS || os == OS.iOS) {
        await _copyLoaderPathDeps(libFile, input.outputDirectory);
      }

      // Asset name has no extension — the Dart runtime appends the platform
      // extension (.dylib/.so/.dll) when matching against @Native(assetId:).
      output.assets.code.add(
        CodeAsset(
          package: 'coroute_framework',
          name: 'src/coroute_app',
          linkMode: DynamicLoadingBundled(),
          file: outLib.uri,
        ),
      );

      output.addDependency(libFile.uri);
    }

    // -------------------------------------------------------------------------
    // 2. Platform permission injection (idempotent, runs every build)
    // -------------------------------------------------------------------------
    // Note: macOS entitlements are patched at CMake configure time via
    // file(WRITE ...) in CorouteApp.cmake — NOT here. Patching entitlements
    // inside a Flutter/Xcode build phase causes a build error:
    //   "Entitlements file was modified during the build"
    // Android manifest patching is safe here since it is not inside Xcode.
    if (flutterProjectDir != null && flutterProjectDir.isNotEmpty) {
      final projectDir = Directory(flutterProjectDir);
      if (projectDir.existsSync()) {
        await _patchAndroidManifest(projectDir);
      }
    }
  });
}

// Derives the Flutter project root from the hook's outputDirectory.
// outputDirectory is: <flutter_root>/.dart_tool/hooks_runner/shared/coroute_framework/build/<hash>/
// Walking up 6 levels yields <flutter_root>.
String _deriveFlutterRoot(Uri outputDirectory) {
  var dir = Directory.fromUri(outputDirectory);
  for (var i = 0; i < 6; i++) {
    dir = dir.parent;
  }
  return dir.path;
}

// Case-insensitive, trailing-separator-normalised path comparison.
bool _samePath(String a, String b) {
  final na = p.normalize(a);
  final nb = p.normalize(b);
  return na == nb || na.toLowerCase() == nb.toLowerCase();
}

// Parses `otool -L` output of [libFile] to find @loader_path-relative
// dependencies and copies each one from the same directory as [libFile]
// into [outputDir]. This makes them available inside the framework bundle
// (alongside the main binary) so @loader_path resolution works in the sandbox.
Future<void> _copyLoaderPathDeps(File libFile, Uri outputDir) async {
  final result = await Process.run('otool', ['-L', libFile.path]);
  if (result.exitCode != 0) return;

  final libDir = libFile.parent.path;
  for (final line in (result.stdout as String).split('\n')) {
    final trimmed = line.trim();
    if (!trimmed.startsWith('@loader_path/')) continue;
    final depName = trimmed.split(' ').first.replaceFirst('@loader_path/', '');
    final depSrc = File(p.join(libDir, depName));
    if (!depSrc.existsSync()) continue;
    final depDst = File.fromUri(outputDir.resolve(depName));
    depSrc.copySync(depDst.path);
  }
}

// Ensures android.permission.INTERNET is declared in the Android manifest.
// Inserts the permission tag if absent (idempotent).
Future<void> _patchAndroidManifest(Directory projectDir) async {
  final manifest = File(
    '${projectDir.path}/android/app/src/main/AndroidManifest.xml',
  );
  if (!manifest.existsSync()) return;

  var content = manifest.readAsStringSync();
  const permission =
      '<uses-permission android:name="android.permission.INTERNET"/>';

  if (!content.contains('android.permission.INTERNET')) {
    // Insert before the <application> tag (standard placement).
    content = content.replaceFirst(
      '<application',
      '$permission\n    <application',
    );
    manifest.writeAsStringSync(content);
  }
}
