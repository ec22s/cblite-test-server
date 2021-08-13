#!/usr/bin/env python3

from genericpath import isdir
from pathlib import Path
from progressbar import ProgressBar
import json
import argparse
import urllib.request
import tarfile
import zipfile
import os
import shutil
import subprocess
import glob

SCRIPT_DIR=os.path.dirname(os.path.realpath(__file__))
DOWNLOAD_DIR=f'{SCRIPT_DIR}/../downloaded'
BUILD_DIR=f'{SCRIPT_DIR}/../build'
ZIPS_DIR=f'{SCRIPT_DIR}/../zips'

json_data={}
def read_manifest():
    global json_data
    if len(json_data) == 0:
        with open(str(Path(SCRIPT_DIR) / 'cross_manifest.json'), 'r') as fin:
            data=fin.read()
        
        json_data=json.loads(data)

    return json_data

def copy_and_overwrite(from_path, to_path):
    if os.path.exists(to_path):
        shutil.rmtree(to_path)
    shutil.copytree(from_path, to_path)

pbar=None
def show_download_progress(block_num, block_size, total_size):
    global pbar
    if pbar is None:
        pbar = ProgressBar(maxval=total_size)
        pbar.start()
    
    downloaded = block_size * block_num
    if downloaded < total_size:
        pbar.update(downloaded)
    else:
        pbar.finish()
        pbar = None

def tar_extract_callback(archive : tarfile.TarFile):
    global pbar
    count=0
    pbar = ProgressBar(maxval=len(archive.getnames()))
    pbar.start()

    for member in archive:
        count += 1
        pbar.update(count)
        yield member

    pbar.finish()
    pbar = None

def check_toolchain(name: str):
    toolchain_path = Path.home() / '.cbl_cross' / f'{name}-toolchain'
    if toolchain_path.exists() and toolchain_path.is_dir() and len(os.listdir(toolchain_path)) > 0:
        print(f'{toolchain_path} found, not downloading...')
        return toolchain_path

    json_data=read_manifest()
    if not name in json_data:
        raise ValueError(f'Unknown target {name}')

    if json_data[name]['toolchain']:
        # For now, assume that the toolchain is tar.gz
        print(f'Downloading {name} toolchain...')
        urllib.request.urlretrieve(json_data[name]['toolchain'], "toolchain.tar.gz", show_download_progress)
        os.makedirs(toolchain_path, 0o755, True)
        print(f'Extracting {name} toolchain to {toolchain_path}...')
        with tarfile.open('toolchain.tar.gz', 'r:gz') as tar:
            tar.extractall(toolchain_path, members=tar_extract_callback(tar))
        
        outer_dir = toolchain_path / os.listdir(toolchain_path)[0]
        files_to_move = outer_dir.glob("**/*")
        for file in files_to_move:
            relative = file.relative_to(outer_dir)
            os.makedirs(toolchain_path / relative.parent, 0o755, True)
            shutil.move(str(file), toolchain_path / relative.parent)

        os.rmdir(outer_dir)
        os.remove("toolchain.tar.gz")
        return toolchain_path
    else:
        print("No toolchain specified, using generic installed...")
        return ""

def check_sysroot(name: str):
    sysroot_path = Path.home() / '.cbl_cross' / f'{name}-sysroot'
    if sysroot_path.exists() and sysroot_path.is_dir() and len(os.listdir(sysroot_path)) > 0:
        print(f'{sysroot_path} found, not downloading...')
        return

    json_data=read_manifest()
    if not name in json_data:
        raise ValueError(f'Unknown target {name}')

    print(f'Downloading {name} sysroot...')
    sysroot_name=json_data[name]['sysroot']
    urllib.request.urlretrieve(f'http://downloads.build.couchbase.com/mobile/sysroot/{sysroot_name}', 'sysroot.tar.gz', show_download_progress)
    os.makedirs(sysroot_path, 0o755, True)
    print(f'Extracting {name} sysroot to {sysroot_path}...')
    with tarfile.open("sysroot.tar.gz", 'r:gz') as tar:
            tar.extractall(sysroot_path, members=tar_extract_callback(tar))

    os.remove("sysroot.tar.gz")

if __name__ == '__main__':
    print("Downloading latest cross compilation manifest...")
    os.chdir(SCRIPT_DIR)
    urllib.request.urlretrieve('https://raw.githubusercontent.com/couchbase/couchbase-lite-C/master/jenkins/cross_manifest.json', 'cross_manifest.json', show_download_progress)

    parser = argparse.ArgumentParser(description='Perform a cross compilation of Couchbase Lite C')
    parser.add_argument('version', type=str, help='The version of the build')
    parser.add_argument('bld_num', type=int, help='The build number for this build')
    parser.add_argument('edition', type=str, choices=["community", "enterprise"], help='The edition to build (community or enterprise)')
    parser.add_argument('os', type=str, help="The target OS to compile for")
    parser.add_argument('toolchain', type=str, help='The CMake toolchain file to use for building')
    args = parser.parse_args()
    
    toolchain_path = check_toolchain(args.os)
    check_sysroot(args.os)

    print(f"====  Cross Building Release binary using {args.toolchain} ====")

    sysroot_path = Path.home() / '.cbl_cross' / f'{args.os}-sysroot'

    if toolchain_path:
        existing_path = os.environ['PATH']
        os.environ['PATH'] = f'{str(toolchain_path)}/bin:{existing_path}'

    os.environ['ROOTFS'] = str(sysroot_path)

    shutil.rmtree(DOWNLOAD_DIR, ignore_errors=True)
    os.makedirs(DOWNLOAD_DIR, 0o755)
    os.chdir(DOWNLOAD_DIR)

    print(f"==== Downloading Couchbase Lite C [{args.os}] {args.version}-{args.bld_num} ====")
    zip_filename=f'couchbase-lite-c-{args.os}-{args.version}-{args.bld_num}-{args.edition}.tar.gz'
    urllib.request.urlretrieve(f'http://latestbuilds.service.couchbase.com/builds/latestbuilds/couchbase-lite-c/{args.version}/{args.bld_num}/{zip_filename}', zip_filename, show_download_progress)
    with tarfile.open(zip_filename, 'r:gz') as tar:
        tar.extractall(DOWNLOAD_DIR)
    os.remove(zip_filename)

    os.makedirs(BUILD_DIR, 0o755, True)
    os.chdir(BUILD_DIR)

    cmake_args=['cmake', '..', f'-DCMAKE_PREFIX_PATH={DOWNLOAD_DIR}', '-DCMAKE_BUILD_TYPE=Release', 
        f'-DCMAKE_TOOLCHAIN_FILE={args.toolchain}']
    if args.os == "raspbian9" or args.os == "debian9_x64":
        cmake_args.append('-DCBL_STATIC_CXX=ON')
    elif args.os == "raspios10_arm64":
        cmake_args.append('-D64_BIT=ON')

    subprocess.run(cmake_args)
    subprocess.run(['make', '-j8', 'install'])

    for lib_file in glob.glob(f'{DOWNLOAD_DIR}/lib/**/libcblite.so*'):
        shutil.copy2(lib_file, 'out/bin')
   
    print("==== Copying resources to output folder ====")
    zip_filename=f'testserver_{args.os}_{args.edition}.zip'
    shutil.copy2(f'{SCRIPT_DIR}/../../CBLTestServer-Dotnet/TestServer/sg_cert.pem', 'out/bin')
    pbar = ProgressBar(3)
    pbar.start()
    copy_and_overwrite(f'{SCRIPT_DIR}/../../CBLTestServer-Dotnet/TestServer.NetCore/certs', 'out/bin/certs')
    pbar.update(1)
    copy_and_overwrite(f'{SCRIPT_DIR}/../../CBLTestServer-Dotnet/TestServer.NetCore/Databases', 'out/bin/Databases')
    pbar.update(2)
    copy_and_overwrite(f'{SCRIPT_DIR}/../../CBLTestServer-Dotnet/TestServer.NetCore/Files', 'out/bin/Files')
    pbar.update(3)
    pbar.finish()
    os.chdir('out/bin')

    print("==== Compressing artifact ====")
    files_to_add = []
    cwd_path = Path(os.getcwd())
    for root,_,files in os.walk(os.getcwd()):
        root_path = Path(root)
        for f in files:
            file_path = (root_path / f).relative_to(cwd_path)
            files_to_add.append(str(file_path))

    os.makedirs(ZIPS_DIR, 0o755, True)
    with zipfile.ZipFile(f'{ZIPS_DIR}/{zip_filename}', 'w', zipfile.ZIP_DEFLATED, compresslevel=4) as zip:    
        count = 0
        for file in files_to_add:
            zip.write(file)
            count += 1
            print(f'{file} ({int(count / len(files_to_add) * 100)}%)')
