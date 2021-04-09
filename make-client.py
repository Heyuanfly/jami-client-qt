import tempfile
import re
import sys
import os
import subprocess
import platform
import argparse
import multiprocessing
import fileinput
from enum import Enum

# vs help
win_sdk_default = '10.0.16299.0'
win_toolset_default = '142'
qt_version_default = '5.15.0'

vs_where_path = os.path.join(
    os.environ['ProgramFiles(x86)'], 'Microsoft Visual Studio', 'Installer', 'vswhere.exe'
)

host_is_64bit = (False, True)[platform.machine().endswith('64')]
this_dir = os.path.dirname(os.path.realpath(__file__))
build_dir = this_dir + '\\build'

class QtVerison(Enum):
    Major = 0
    Minor = 1
    Micro = 2

def execute_cmd(cmd, with_shell=False, env_vars={}):
    if(bool(env_vars)):
        p = subprocess.Popen(cmd, shell=with_shell,
                             stdout=sys.stdout,
                             env=env_vars)
    else:
        p = subprocess.Popen(cmd, shell=with_shell)
    _, _ = p.communicate()
    return p.returncode


def getLatestVSVersion():
    args = [
        '-latest',
        '-products *',
        '-requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64',
        '-property installationVersion'
    ]
    cmd = [vs_where_path] + args
    output = subprocess.check_output(' '.join(cmd)).decode('utf-8')
    if output:
        return output.splitlines()[0].split('.')[0]
    else:
        return

def getCMakeGenerator(vs_version):
    if vs_version == '15':
        return 'Visual Studio 15 2017 Win64'
    else:
        return 'Visual Studio ' + vs_version + ' 2019'

def findVSLatestDir():
    args = [
        '-latest',
        '-products *',
        '-requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64',
        '-property installationPath'
    ]
    cmd = [vs_where_path] + args
    output = subprocess.check_output(
        ' '.join(cmd)).decode('utf-8', errors='ignore')
    if output:
        return output.splitlines()[0]
    else:
        return

def getQtVersionNumber(qt_version, version_type):
    version_list = qt_version.split('.')
    return version_list[version_type.value]

def findMSBuild():
    filename = 'MSBuild.exe'
    for root, _, files in os.walk(findVSLatestDir() + r'\\MSBuild'):
        if filename in files:
            return os.path.join(root, filename)

def getMSBuildArgs(arch, config_str, configuration_type, toolset):
    msbuild_args = [
        '/nologo',
        '/verbosity:minimal',
        '/maxcpucount:' + str(multiprocessing.cpu_count()),
        '/p:Platform=' + arch,
        '/p:Configuration=' + config_str,
        '/p:ConfigurationType=' + configuration_type,
        '/p:useenv=true']
    if (toolset != ''):
        msbuild_args.append('/p:PlatformToolset=' + toolset)
    return msbuild_args

def getVSEnv(arch='x64', platform='', version=''):
    env_cmd = 'set path=%path:"=% && ' + \
        getVSEnvCmd(arch, platform, version) + ' && set'
    p = subprocess.Popen(env_cmd,
                         shell=True,
                         stdout=subprocess.PIPE)
    stdout, _ = p.communicate()
    out = stdout.decode('utf-8', errors='ignore').split("\r\n")[5:-1]
    return dict(s.split('=', 1) for s in out)


def getVSEnvCmd(arch='x64', platform='', version=''):
    vcEnvInit = [findVSLatestDir() + r'\VC\Auxiliary\Build\"vcvarsall.bat']
    if platform != '':
        args = [arch, platform, version]
    else:
        args = [arch, version]
    if args:
        vcEnvInit.extend(args)
    vcEnvInit = 'call \"' + ' '.join(vcEnvInit)
    return vcEnvInit


def build_project(msbuild, msbuild_args, proj, env_vars):
    args = []
    args.extend(msbuild_args)
    args.append(proj)
    cmd = [msbuild]
    cmd.extend(args)
    if (execute_cmd(cmd, True, env_vars)):
        print("Build failed when building ", proj)
        sys.exit(1)


def replace_vs_prop(filename, prop, val):
    p = re.compile(r'(?s)<' + prop + r'\s?.*?>(.*?)<\/' + prop + r'>')
    val = r'<' + prop + r'>' + val + r'</' + prop + r'>'
    with fileinput.FileInput(filename, inplace=True) as file:
        for line in file:
            print(re.sub(p, val, line), end='')


def deps(arch, toolset, qtver):
    print('Deps Qt Client Release|' + arch)

    # Fetch QRencode
    print('Generate QRencode')
    apply_cmd = "git apply --reject --ignore-whitespace --whitespace=fix"
    qrencode_path = 'qrencode-win32'
    if (os.path.isdir(qrencode_path)):
        os.system('rmdir qrencode-win32 /s /q')
    if (execute_cmd("git clone https://github.com/BlueDragon747/qrencode-win32.git", True)):
        print("Git clone failed when cloning from https://github.com/BlueDragon747/qrencode-win32.git")
        sys.exit(1)
    if(execute_cmd("cd qrencode-win32 && git checkout d6495a2aa74d058d54ae0f1b9e9e545698de66ce && " + apply_cmd + ' ..\\qrencode-win32.patch', True)):
        print("qrencode-win32 set up error")
        sys.exit(1)

    print('Building qrcodelib')

    vs_env_vars = {}
    vs_env_vars.update(getVSEnv())

    msbuild = findMSBuild()
    if not os.path.isfile(msbuild):
        raise IOError('msbuild.exe not found. path=' + msbuild)
    msbuild_args = getMSBuildArgs(arch, 'Release-Lib', 'StaticLibrary', toolset)

    this_dir = os.path.dirname(os.path.realpath(__file__))
    proj_path = this_dir + '\\qrencode-win32\\qrencode-win32\\vc8\\qrcodelib\\qrcodelib.vcxproj'

    build_project(msbuild, msbuild_args, proj_path, vs_env_vars)


def build(arch, toolset, sdk_version, config_str, project_path_under_current_path, qtver, force_option=True):
    print("Building with Qt " + qtver)

    configuration_type = 'StaticLibrary'

    vs_env_vars = {}
    vs_env_vars.update(getVSEnv())

    qt_dir = 'C:\\Qt\\' + qtver
    cmake_gen = getCMakeGenerator(getLatestVSVersion())
    qt_minor_version = getQtVersionNumber(qtver, QtVerison.Minor)
    msvc_folder = '\\msvc2017_64' if int(qt_minor_version) <= 14 else '\\msvc2019_64'

    qt_cmake_dir = qt_dir + msvc_folder + '\\lib\\cmake\\'
    cmake_prefix_path = qt_dir + msvc_folder

    cmake_options = [
        '-DCMAKE_PREFIX_PATH=' + cmake_prefix_path,
        '-DQt5_DIR=' + qt_cmake_dir + 'Qt5',
        '-DQt5Core_DIR=' + qt_cmake_dir + 'Qt5Core',
        '-DQt5Sql_DIR=' + qt_cmake_dir + 'Qt5Sql',
        '-DQt5LinguistTools_DIR=' + qt_cmake_dir + 'Qt5LinguistTools',
        '-DQt5Concurrent_DIR=' + qt_cmake_dir + 'Qt5Concurrent',
        '-DQt5Gui_DIR=' + qt_cmake_dir + 'Qt5Gui',
        '-DCMAKE_SYSTEM_VERSION=' + sdk_version
    ]
    if not os.path.exists(build_dir):
        os.makedirs(build_dir)
    os.chdir(build_dir)

    if (config_str == 'Release'):
        print('Generating project using cmake ' + config_str + '|' + arch)
        cmd = ['cmake', '..', '-G', cmake_gen]
        cmd.extend(cmake_options)
        if(execute_cmd(cmd, False, vs_env_vars)):
            print("Cmake vcxproj file generate error")
            sys.exit(1)
        configuration_type = 'Application'
    elif (config_str == 'Beta'):
        print('Generating project using cmake ' + config_str + '|' + arch)
        cmake_options.append('-DBETA=1')
        cmd = ['cmake', '..', '-G', cmake_gen]
        cmd.extend(cmake_options)
        if(execute_cmd(cmd, False, vs_env_vars)):
            print("Beta: Cmake vcxproj file generate error")
            sys.exit(1)
        config_str = 'Release'
        configuration_type = 'Application'
    elif (config_str == 'ReleaseCompile'):
        print('Generating project using qmake ' + config_str + '|' + arch)
        cmake_options.append('-DReleaseCompile=1')
        cmd = ['cmake', '..', '-G', cmake_gen]
        cmd.extend(cmake_options)
        if(execute_cmd(cmd, False, vs_env_vars)):
            print("ReleaseCompile: Cmake vcxproj file generate error")
            sys.exit(1)
        config_str = 'Release'

    # Note: If project is configured to Beta or ReleaseCompile, the configuration name is still release,
    # but will be outputted into x64/Beta folder (for Beta Only)

    print('Building projects in ' + config_str + '|' + arch)
    qt_client_proj_path = build_dir + project_path_under_current_path

    msbuild = findMSBuild()
    if not os.path.isfile(msbuild):
        raise IOError('msbuild.exe not found. path=' + msbuild)
    msbuild_args = getMSBuildArgs(arch, config_str, configuration_type, toolset)

    if (force_option):
        # force toolset
        replace_vs_prop(qt_client_proj_path,
                        'PlatformToolset',
                        toolset)
        # force unicode
        replace_vs_prop(qt_client_proj_path,
                        'CharacterSet',
                        'Unicode')
        # force sdk_version
        replace_vs_prop(qt_client_proj_path,
                        'WindowsTargetPlatformVersion',
                        sdk_version)

    build_project(msbuild, msbuild_args, qt_client_proj_path, vs_env_vars)


def parse_args():
    ap = argparse.ArgumentParser(description="Client qt build tool")
    ap.add_argument(
        '-b', '--build', action='store_true',
        help='Build Qt Client')
    ap.add_argument(
        '-a', '--arch', default='x64',
        help='Sets the build architecture')
    ap.add_argument(
        '-d', '--deps', action='store_true',
        help='Build Deps for Qt Client')
    ap.add_argument(
        '-bt', '--beta', action='store_true',
        help='Build Qt Client in Beta Config')
    ap.add_argument(
        '-c', '--releasecompile', action='store_true',
        help='Build Qt Client in ReleaseCompile Config')
    ap.add_argument(
        '-s', '--sdk', default=win_sdk_default, type=str,
        help='Use specified windows sdk version')
    ap.add_argument(
        '-t', '--toolset', default=win_toolset_default, type=str,
        help='Use specified platform toolset version')
    ap.add_argument(
        '-q', '--qtver', default=qt_version_default,
        help='Sets the version of Qmake')

    parsed_args = ap.parse_args()

    if parsed_args.toolset:
        if parsed_args.toolset[0] != 'v':
            parsed_args.toolset = 'v' + parsed_args.toolset

    return parsed_args


def main():
    if not host_is_64bit:
        print('These scripts will only run on a 64-bit Windows system for now!')
        sys.exit(1)

    if int(getLatestVSVersion()) < 15:
        print('These scripts require at least Visual Studio v15 2017!')
        sys.exit(1)

    parsed_args = parse_args()

    if parsed_args.deps:
        deps(parsed_args.arch, parsed_args.toolset, parsed_args.qtver)

    if parsed_args.build:
        build(parsed_args.arch, parsed_args.toolset, parsed_args.sdk,
              'Release', '\\jami-qt.vcxproj', parsed_args.qtver)

    if parsed_args.beta:
        build(parsed_args.arch, parsed_args.toolset, parsed_args.sdk,
              'Beta', '\\jami-qt.vcxproj', parsed_args.qtver)

    if parsed_args.releasecompile:
        build(parsed_args.arch, parsed_args.toolset, parsed_args.sdk,
              'ReleaseCompile', '\\jami-qt.vcxproj', parsed_args.qtver)


if __name__ == '__main__':
    main()
