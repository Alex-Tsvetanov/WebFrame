import 'dart:io';
import 'package:native_assets_cli/native_assets_cli.dart';
import 'package:native_assets_cli/code_assets.dart';
import 'package:path/path.dart' as p;

void main(List<String> args) async {
  try {
    await build(args, (input, output) async {
      // In native_assets_cli 0.18.0, config is accessed via .config.code
      if (!input.config.buildCodeAssets) {
        return;
      }

      final os = input.config.code.targetOS;
      final packageRoot = input.packageRoot.toFilePath();

      File('native_assets_build.log').writeAsStringSync(
          'Starting build for $os in $packageRoot\n',
          mode: FileMode.append);

      // The library path is stored in a hidden file by CorouteApp.cmake.
      final rootDir = Directory(packageRoot);
      final configFiles = rootDir
          .listSync()
          .whereType<File>()
          .where((f) => p.basename(f.path).startsWith('.coroute_lib_path.'));

      if (configFiles.isEmpty) {
        final msg =
            'No .coroute_lib_path.* configuration files found in $packageRoot\n';
        File('native_assets_build.log')
            .writeAsStringSync(msg, mode: FileMode.append);
        print(msg);
        return;
      }

      File? bestConfig;
      if (configFiles.length == 1) {
        bestConfig = configFiles.first;
      } else {
        final sorted = configFiles.toList()
          ..sort(
              (a, b) => b.lastModifiedSync().compareTo(a.lastModifiedSync()));
        bestConfig = sorted.first;
      }

      if (bestConfig != null) {
        final lines = bestConfig.readAsLinesSync();
        if (lines.isEmpty) return;

        final libPath = lines[0];
        final libFile = File(libPath);

        if (!libFile.existsSync()) {
          final msg =
              'Native library not found at $libPath (referenced by ${bestConfig.path})\n';
          File('native_assets_build.log')
              .writeAsStringSync(msg, mode: FileMode.append);
          print(msg);
          return;
        }

        final libName = p.basename(libPath);
        final outLib = File.fromUri(input.outputDirectory.resolve(libName));
        libFile.copySync(outLib.path);

        File('native_assets_build.log').writeAsStringSync(
            'Copied $libName to ${outLib.path}\n',
            mode: FileMode.append);

        if (os == OS.macOS || os == OS.iOS) {
          await _copyLoaderPathDeps(libFile, input.outputDirectory);
        } else if (os == OS.windows) {
          await _copySiblingDlls(libFile, input.outputDirectory);
        }

        // Use CodeAsset for 0.18.0
        output.assets.code.add(CodeAsset(
          package: input.packageName,
          name: 'src/coroute_app',
          linkMode: DynamicLoadingBundled(),
          file: outLib.uri,
        ));

        File('native_assets_build.log').writeAsStringSync(
            'Added asset src/coroute_app\n',
            mode: FileMode.append);
      }

      if (os == OS.android) {
        await _patchAndroidManifest(Directory(packageRoot));
      }
    });
  } catch (e, stack) {
    File('native_assets_build.log')
        .writeAsStringSync('ERROR: $e\n$stack\n', mode: FileMode.append);
    rethrow;
  }
}

// Parses `otool -L` output of [libFile] to find @loader_path-relative
// dependencies and copies each one from the same directory as [libFile]
// into [outputDir].
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

// Scans the directory containing [libFile] for other .dll files and copies
// them into [outputDir].
Future<void> _copySiblingDlls(File libFile, Uri outputDir) async {
  final libDir = libFile.parent;
  final entities = libDir.listSync();
  for (final entity in entities) {
    if (entity is File &&
        p.extension(entity.path).toLowerCase() == '.dll' &&
        p.basename(entity.path).toLowerCase() !=
            p.basename(libFile.path).toLowerCase()) {
      final dst = File.fromUri(outputDir.resolve(p.basename(entity.path)));
      entity.copySync(dst.path);
    }
  }
}

// Ensures android.permission.INTERNET is declared in the Android manifest.
Future<void> _patchAndroidManifest(Directory projectDir) async {
  final manifest = File(
    '${projectDir.path}/android/app/src/main/AndroidManifest.xml',
  );
  if (!manifest.existsSync()) return;

  var content = manifest.readAsStringSync();
  const permission =
      '<uses-permission android:name="android.permission.INTERNET"/>';

  if (!content.contains('android.permission.INTERNET')) {
    content = content.replaceFirst(
      '<application',
      '$permission\n    <application',
    );
    manifest.writeAsStringSync(content);
  }
}
