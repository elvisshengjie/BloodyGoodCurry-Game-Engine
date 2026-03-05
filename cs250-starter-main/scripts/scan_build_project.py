"""
file: scan_build_project.py
author: Rudy Castan
date: 2026 Spring
course: CS250 Computer Graphics II
copyright: DigiPen Institute of Technology

Scans project for TODO comments and bad OpenGL usage,
then builds all CMake presets.
"""

import argparse
import re
import sys
import platform
import subprocess
from concurrent.futures import ThreadPoolExecutor, as_completed, Future
from dataclasses import dataclass
from pathlib import Path


# Module-level constants
TODO_PATTERN = re.compile(r'TODO|FIXME', re.IGNORECASE)
OPENGL_PATTERN = re.compile(r'\b(gl[A-Z][a-zA-Z0-9_]*)\(')
BUILD_TYPES = ('debug', 'developer-release', 'release')
BASE_TARGETS = ('web',)
SEPARATOR = '=' * 60
SCANNABLE_EXTENSIONS = (".cpp", ".hpp", ".h", ".vert", ".frag", ".cmake", ".txt", ".html")


@dataclass
class ScanResult:
    """Result of scanning a single file."""
    file_path: Path
    todos: list[tuple[int, str]]
    opengl_issues: list[tuple[int, str]]
    
    @property
    def has_issues(self) -> bool:
        return bool(self.todos or self.opengl_issues)


@dataclass
class BuildResult:
    """Result of building a specific target and build type."""
    target: str
    build_type: str
    output: str
    error: str = ""
    
    @property
    def success(self) -> bool:
        return not self.error


def find_todo_comments(file_path: Path) -> list[tuple[int, str]]:
    """Find all TODO/FIXME comments in a file."""
    with open(file_path, 'r', encoding='utf-8') as file:
        lines = file.readlines()
    return [(i + 1, line.strip()) for i, line in enumerate(lines) if TODO_PATTERN.search(line)]


def find_opengl_usage(file_path: Path) -> list[tuple[int, str]]:
    """Find raw OpenGL function calls that should be wrapped."""
    if file_path.name == 'GL.cpp':
        return []
    with open(file_path, 'r', encoding='utf-8') as file:
        lines = file.readlines()
    return [(i + 1, match) for i, line in enumerate(lines) for match in OPENGL_PATTERN.findall(line)]


def scan_file(file_path: Path) -> ScanResult:
    """Scan a single file for issues."""
    todos = find_todo_comments(file_path)
    opengl_issues = []
    
    if file_path.suffix in (".cpp", ".hpp", ".h"):
        opengl_issues = find_opengl_usage(file_path)
    
    return ScanResult(file_path, todos, opengl_issues)


def scan_directory(directory: Path, skip_todos: bool = False, skip_opengl: bool = False) -> int:
    """Scan directory for TODO comments and OpenGL issues. Returns 0 on success, 1 if issues found."""
    if skip_todos and skip_opengl:
        print("[SCAN SKIPPED] All scans disabled.\n")
        return 0
    
    print(SEPARATOR)
    print("  SCANNING PROJECT")
    print(SEPARATOR)
    print(f"Directory: {directory}")
    if skip_todos:
        print("[INFO] TODO/FIXME scanning disabled")
    if skip_opengl:
        print("[INFO] OpenGL usage scanning disabled")
    print()
    
    results: list[ScanResult] = []
    
    with ThreadPoolExecutor() as executor:
        futures: list[Future[ScanResult]] = []
        
        for file_path in directory.rglob('*'):
            if 'build' in file_path.parts:
                continue
            if file_path.suffix in SCANNABLE_EXTENSIONS:
                futures.append(executor.submit(scan_file, file_path))
        
        for future in as_completed(futures):
            result = future.result()
            if result.has_issues:
                results.append(result)
    
    # Report all issues
    has_todo = False
    has_opengl_issue = False
    
    for result in results:
        if result.todos and not skip_todos:
            has_todo = True
            print(f"[TODO Found] {result.file_path}", file=sys.stderr)
            for line, comment in result.todos:
                print(f"  Line {line}: {comment}", file=sys.stderr)
            print(file=sys.stderr)
        
        if result.opengl_issues and not skip_opengl:
            has_opengl_issue = True
            print(f"[Bad OpenGL Usage] {result.file_path}", file=sys.stderr)
            for line, func in result.opengl_issues:
                print(f"  Line {line}: {func} should be wrapped (e.g., GL::{func[2:]})", file=sys.stderr)
            print(file=sys.stderr)
    
    # Summary
    if has_todo or has_opengl_issue:
        print(SEPARATOR, file=sys.stderr)
        if has_todo:
            print("[ERROR] TODO comments found. Fix them before proceeding.", file=sys.stderr)
        if has_opengl_issue:
            print("[ERROR] Bad OpenGL usage found. Fix them before proceeding.", file=sys.stderr)
        print(SEPARATOR, file=sys.stderr)
        return 1
    
    print("[SCAN OK] No issues found.\n")
    return 0


def windows_to_wsl_path(win_path: Path) -> str:
    """Convert Windows path to WSL path format."""
    win_path = win_path.resolve()
    match = re.match(r"([A-Za-z]):\\(.*)", str(win_path))
    if not match:
        raise ValueError("Invalid Windows path format")
    drive, path = match.groups()
    return "/mnt/" + drive.lower() + "/" + path.replace("\\", "/")


def configure_preset(folder: Path, build_type: str, target: str) -> BuildResult:
    """Configure a specific CMake preset. Returns BuildResult with configure output."""
    linux_prefix: list[str] = []
    folder_path: Path | str = folder
    
    if platform.system() == "Windows" and target in ("web", "linux"):
        linux_prefix = ['wsl']
        folder_path = windows_to_wsl_path(folder)
    else:
        folder_path = str(folder)
    
    log_content: list[str] = []
    log_content.append(SEPARATOR)
    log_content.append(f"Configure: {target.upper()} {build_type}")
    log_content.append(SEPARATOR)
    
    configure = subprocess.run(
        linux_prefix + ['cmake', '--preset', f'{target}-{build_type.lower()}', '-S', folder_path],
        capture_output=True, text=True, encoding="utf-8"
    )
    log_content.append(configure.stdout)
    if configure.stderr:
        log_content.append(configure.stderr)
    
    if configure.returncode != 0:
        log_content.append(f"\n[CONFIGURE FAILED] Return code: {configure.returncode}")
        return BuildResult(target, build_type, '\n'.join(log_content), configure.stderr + '\n' + configure.stdout)
    
    log_content.append("\n[CONFIGURE SUCCESS]")
    return BuildResult(target, build_type, '\n'.join(log_content))


def build_preset(folder: Path, build_type: str, target: str) -> BuildResult:
    """Build a specific CMake preset. Assumes configure has already been run."""
    linux_prefix: list[str] = []
    build_dir: Path | str = folder / 'build' / f'{target}-{build_type.lower()}'
    
    if platform.system() == "Windows" and target in ("web", "linux"):
        linux_prefix = ['wsl']
        build_dir = windows_to_wsl_path(build_dir)
    else:
        build_dir = str(build_dir)
    
    log_content: list[str] = []
    log_content.append(SEPARATOR)
    log_content.append(f"Build: {target.upper()} {build_type}")
    log_content.append(SEPARATOR)
    
    config_build_type: str = build_type if build_type != "developer-release" else "Release"
    build = subprocess.run(
        linux_prefix + ['cmake', '--build', build_dir, '--config', config_build_type],
        capture_output=True, text=True, encoding="utf-8"
    )
    log_content.append(build.stdout)
    if build.stderr:
        log_content.append(build.stderr)
    
    if build.returncode != 0:
        log_content.append(f"\n[BUILD FAILED] Return code: {build.returncode}")
        return BuildResult(target, build_type, '\n'.join(log_content), build.stderr + '\n' + build.stdout)
    
    log_content.append("\n[BUILD SUCCESS]")
    return BuildResult(target, build_type, '\n'.join(log_content))


def build_all(folder: Path, target_filter: list[str] | None = None, build_type_filter: list[str] | None = None) -> int:
    """Build all presets and return exit code (0=success, 1=failure)."""
    print(SEPARATOR)
    print("  BUILD ALL PRESETS")
    print(SEPARATOR)
    
    # Determine targets
    if target_filter:
        targets = target_filter
    else:
        targets = list(BASE_TARGETS)
        if platform.system() == "Windows":
            targets.append('windows')
    
    # Determine build types
    build_types = build_type_filter if build_type_filter else list(BUILD_TYPES)
    
    print(f"Targets: {', '.join(targets)}")
    print(f"Build types: {', '.join(build_types)}")
    print()
    
    total: int = len(targets) * len(build_types)
    
    # Step 1: Configure all presets sequentially (to avoid data races with caching dependencies)
    print("\nSTEP 1: Configuring all presets...")
    print(SEPARATOR)
    
    configure_errors: list[BuildResult] = []
    count: int = 0
    
    for target in targets:
        for build_type in build_types:
            count += 1
            print(f"\n[{count}/{total}] Configuring {target.upper()} {build_type}...")
            
            result = configure_preset(folder, build_type, target)
            print(result.output)
            
            if result.success:
                print(f"[OK] Configure: {target.upper()} {build_type}")
            else:
                print(f"[FAIL] Configure: {target.upper()} {build_type}")
                configure_errors.append(result)
    
    # If any configure failed, stop here
    if configure_errors:
        print()
        print(SEPARATOR)
        print("  CONFIGURE SUMMARY")
        print(SEPARATOR)
        print(f"Total: {total}, Failed: {len(configure_errors)}")
        print("\nFailed configurations:")
        for result in configure_errors:
            print(f"  - {result.target.upper()} {result.build_type}")
        print("\nStopping - cannot build without successful configuration.")
        return 1
    
    print()
    print(SEPARATOR)
    print(f"All {total} configurations succeeded!")
    print(SEPARATOR)
    
    # Step 2: Build all presets in parallel (safe after all are configured)
    print("\nSTEP 2: Building all presets...")
    print(SEPARATOR)
    
    build_errors: list[BuildResult] = []
    build_successes: list[BuildResult] = []
    count = 0
    
    with ThreadPoolExecutor() as executor:
        futures: list[Future[BuildResult]] = [
            executor.submit(build_preset, folder, build_type, target)
            for target in targets for build_type in build_types
        ]
        
        for future in as_completed(futures):
            result = future.result()
            count += 1
            
            print(result.output)
            
            if result.success:
                print(f"[{count}/{total}] [OK] Build: {result.target.upper()} {result.build_type}")
                build_successes.append(result)
            else:
                print(f"[{count}/{total}] [FAIL] Build: {result.target.upper()} {result.build_type}")
                build_errors.append(result)
            print()
    
    # Final summary
    print()
    print(SEPARATOR)
    print("  BUILD SUMMARY")
    print(SEPARATOR)
    print(f"Total: {total}, Passed: {len(build_successes)}, Failed: {len(build_errors)}")
    print()
    
    if build_errors:
        print("Failed builds:")
        for result in build_errors:
            print(f"  - {result.target.upper()} {result.build_type}")
        print()
        return 1
    else:
        print(f"All {total} builds succeeded!")
        return 0


def main() -> int:
    """Main entry point."""
    try:
        parser = argparse.ArgumentParser(
            description="Scans a project directory for TODOs and builds it using CMake.",
            formatter_class=argparse.RawDescriptionHelpFormatter,
            epilog="""Examples:
  %(prog)s                                    # Scan and build all configurations
  %(prog)s --skip-todos                       # Skip TODO scan, build all
  %(prog)s --skip-opengl                      # Skip OpenGL scan, build all
  %(prog)s --skip-build                       # Scan only, no building
  %(prog)s --skip-todos --skip-opengl         # Skip all scans, build only
  %(prog)s --skip-build --skip-opengl         # TODO scan only
  %(prog)s --target web                       # Build only web target
  %(prog)s --build-type debug release         # Build only debug and release configurations
  %(prog)s --target web --build-type debug    # Build only web debug configuration
            """
        )
        parser.add_argument(
            "directory",
            nargs="?",
            type=Path,
            default=Path.cwd(),
            help="Project directory (default: current working directory)"
        )
        parser.add_argument(
            "--skip-todos",
            action="store_true",
            help="Skip TODO/FIXME scanning"
        )
        parser.add_argument(
            "--skip-opengl",
            action="store_true",
            help="Skip OpenGL usage scanning"
        )
        parser.add_argument(
            "--skip-build",
            action="store_true",
            help="Skip building the project (scan only)"
        )
        parser.add_argument(
            "--target",
            nargs="+",
            choices=['windows', 'web', 'linux'],
            help="Build only specified targets"
        )
        parser.add_argument(
            "--build-type",
            nargs="+",
            choices=['debug', 'developer-release', 'release'],
            help="Build only specified build types"
        )
        args = parser.parse_args()
        
        # Scan first (unless all scans are skipped)
        if not (args.skip_todos and args.skip_opengl):
            scan_result = scan_directory(args.directory, args.skip_todos, args.skip_opengl)
            if scan_result != 0:
                return scan_result
        
        # Then build (unless skipped)
        if not args.skip_build:
            return build_all(args.directory, args.target, args.build_type)
        
        return 0
    
    except Exception as e:
        print(f"\nFATAL ERROR: {e}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    if sys.stdout.encoding.lower() != "utf-8":
        sys.stdout.reconfigure(encoding="utf-8")
    if sys.stderr.encoding.lower() != "utf-8":
        sys.stderr.reconfigure(encoding="utf-8")
    sys.exit(main())

