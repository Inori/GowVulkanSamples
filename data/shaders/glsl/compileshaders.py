import argparse
import fileinput
import os
import subprocess
import sys

parser = argparse.ArgumentParser(description='Compile all GLSL shaders')
parser.add_argument('--compiler', type=str, help='path to glslangvalidator/glslc executable')
parser.add_argument('--g', action='store_true', help='compile with debug symbols')
args = parser.parse_args()

def findCompiler(compiler_name):
    def isExe(path):
        return os.path.isfile(path) and os.access(path, os.X_OK)

    if args.compiler != None and isExe(args.compiler):
        return args.compiler

    exe_name = compiler_name
    if os.name == "nt":
        exe_name += ".exe"

    for exe_dir in os.environ["PATH"].split(os.pathsep):
        full_path = os.path.join(exe_dir, exe_name)
        if isExe(full_path):
            return full_path

    sys.exit("Could not find DXC executable on PATH, and was not specified with --dxc")

# compiler_path = findCompiler("glslangvalidator")
compiler_path = findCompiler("glslc")
dir_path = os.path.dirname(os.path.realpath(__file__))
dir_path = dir_path.replace('\\', '/')
for root, dirs, files in os.walk(dir_path):
    for file in files:
        if file.endswith(".vert") or file.endswith(".frag") or file.endswith(".comp") or file.endswith(".geom") or file.endswith(".tesc") or file.endswith(".tese") or file.endswith(".rgen") or file.endswith(".rchit") or file.endswith(".rmiss"):
            input_file = os.path.join(root, file)
            output_file = input_file + ".spv"

            add_params = ""
            if args.g:
                add_params = "-g -O0"

            if file.endswith(".rgen") or file.endswith(".rchit") or file.endswith(".rmiss"):
               add_params = add_params + " --target-env vulkan1.2"

            # res = subprocess.call("%s -V %s -o %s %s" % (compiler_path, input_file, output_file, add_params), shell=True)
            cmd = [compiler_path, input_file, "-o", output_file]
            if add_params:
                cmd.insert(1, add_params)
            res = subprocess.run([compiler_path, input_file, "-o", output_file], capture_output=True, text=True, encoding='utf-8')
            if res.returncode == 0:
                print('compile succeed: {}'.format(input_file))
            else:
                print(res.stderr)
                sys.exit()