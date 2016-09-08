import os
import sys
import shutil
import platform
import glob

import SCons
from SCons.Errors import UserError

# ----------------------------------------------------------------------
# Local python path and imports

sys.path += [os.path.join(os.getcwd(), 'scripts'),]

# ----------------------------------------------------------------------
# Defaults

# The publish_dir is a directory where release packages are copied.
# This is used internally for Dust development.
DEFAULT_PUBLISH_DIR = ''  # defaults to current directory

# The repository_dir is a Debian repository directory where release packages
# are copied. This is used internally for Dust development.
DEFAULT_REPOSITORY_DIR = ''

# ----------------------------------------------------------------------
# Base setup, command line options

command_line_vars = Variables()
command_line_vars.AddVariables(
    ('debug', 'Build with debug symbols', True),
    ('opt', 'Build with optimization', False),
    ('dry_run', 'Just print commands that will be run', False),
    ('verbose', 'Verbose build output', True),
    ('build_name', 'Debug string to append to build versions', ''),
    # where to find executables
    ('user_path', "Allow scons to use user's PATH", 1),
    # specific details -- defaults for each build platform are defined below
    ('toolchain_dir', "Path to compiler toolchain", ''),
    ('tools_prefix', "Path to headers and libraries for runtime components", ''),
    ('boost_prefix', "Path for the Boost headers and libraries", ''),
    ('boost_lib_suffix', "Suffix for the Boost libraries", ''),
    ('python', '''Path to python. Defaults to python from scons PATH.''', 'python'),
    ('protoc', '''Path to protobuf compiler, protoc. Defaults to protoc from scons PATH.''', 'protoc'),
    ('publish_dir', 'Directory for publishing released artifacts', DEFAULT_PUBLISH_DIR),
    ('repository_dir', "Directory to store the .deb file", DEFAULT_REPOSITORY_DIR),
    EnumVariable('target', 'Choose target platform', 'i686',
                 allowed_values=('i686', 'x86_64', 'armpi')),
)

# for SCons to find the right compiler on Windows, TARGET_ARCH must be set 
# when the Environment is created
baseEnv = Environment(variables = command_line_vars, TARGET_ARCH='i686-linux')

if str(baseEnv['user_path']) == '1':
    baseEnv.AppendENVPath('PATH', os.environ['PATH'])

# Setup the default Help message
Help("""
To build the AP Bridge software:

 $ scons apbridge                      # build the AP Bridge for i386
 $ scons apbridge target=armpi         # build the AP Bridge for Raspberry Pi

 $ scons apbridge_pkg                  # Create AP Bridge package for i386
 $ scons apbridge_pkg target=armpi     # Create AP Bridge package for Raspbery Pi

For internal use, to build an AP Bridge release:

 $ scons apbridge_release

Options for building:
""")

# Append descriptions for the command line options
Help(command_line_vars.GenerateHelpText(baseEnv))

# Ensure that no default targets exist, so you have to specify a target.
# Give some help about what targets are available.
def default(env, target, source): print SCons.Script.help_text
Default(baseEnv.Command('default', None, default))

try:
    os.mkdir('gen')
except:
    pass

# ----------------------------------------------------------------------
# Platform environment setup

def findBoostDirs(baseEnv):
    'Verify the Boost include and lib dirs'
    boost_prefix = baseEnv.subst('$boost_prefix')
    boost_incdir = None
    for incdir in (os.path.join(boost_prefix, 'include'),
                   boost_prefix):
        if os.path.exists(os.path.join(incdir, 'boost', 'version.hpp')):
            boost_incdir = incdir
    if not boost_incdir:
        raise UserError('can not find include directory for Boost')
    
    boost_libdir = None
    for libdir in (os.path.join(boost_prefix, 'lib'),
                   os.path.join(boost_prefix, 'stage', 'lib')):
        if os.path.isdir(libdir):
            boost_libdir = libdir
    if not boost_libdir:
        raise UserError('can not find library directory for Boost')

    return boost_incdir, boost_libdir

def getBuildDir(env, generated_file_dir = 'gen'):
    buildDir = os.path.join(generated_file_dir, env['PLATFORM'])
    build_dir_suffix = ''
    if int(env['debug']):
        build_dir_suffix += '-g'
    if int(env['opt']):
        build_dir_suffix += '-o'

    buildDir += build_dir_suffix
    return buildDir

BOOST_LIBS = [
    'boost_timer', 
    'boost_chrono', 
    'boost_thread', 
    'boost_system',
    'boost_program_options',
    'boost_filesystem',
]

TOOL_LIBS = [
    'log4cxx', 
    'czmq', 
    'zmq', 
    'protobuf', 
    'pthread', 
]

def getLinuxEnv(baseEnv):
    'Construct the Linux build environment'
    boost_incdir, boost_libdir = findBoostDirs(baseEnv)
    env = baseEnv.Clone(
        HOST_ARCH = platform.machine() + '-linux',
        TOOLS_DIR=baseEnv['tools_prefix'],
        BOOST_BASE=baseEnv['boost_prefix'],

        # Compile flags
        CCFLAGS=['-Wall'],
        CXXFLAGS=['-std=c++11'],
        CPPPATH=[
            '#',
            '$TOOLS_DIR/include',
            boost_incdir,
        ],
        # Linker flags
        LIBPATH=[
            '$TOOLS_DIR/lib',
            boost_libdir,
        ],
        # the --start-group and --end-group directives let gcc resolve library ordering
        LINKCOM='$LINK -o $TARGET $SOURCES $LINKFLAGS $_LIBDIRFLAGS -Wl,--start-group $_LIBFLAGS -Wl,--end-group',
        # rpath finds shared libraries at runtime without LD_LIBRARY_PATH
        LINKFLAGS=['-Wl,-rpath,/opt/dust-apc/lib',
                   '-Wl,-rpath,$TOOLS_DIR/lib'],  # needed to find sub-libs at build time
    )

    # Compile actions for protobuf messages
    env['PROTOC'] = env['protoc']
    env['PROTOBUFCOM_DST'] = '$BUILD_DIR'
    env['PROTOBUFCOM'] = 'LD_LIBRARY_PATH=$TOOLS_DIR $PROTOC -I. -I$BUILD_DIR --proto_path=$BUILD_DIR --cpp_out=$PROTOBUFCOM_DST $SOURCE' # -I$SOURCE.dir --cpp_out=$TARGET.dir

    # Libraries
    env['BOOST_LIBS'] = [b + env['boost_lib_suffix'] for b in BOOST_LIBS]
    env['TOOL_LIBS'] = env['BOOST_LIBS'] + TOOL_LIBS
    env['GPS_LIBS'] = ['gps']
    
    # Conditional build flags
    if int(env['debug']):
        env.Append(CCFLAGS = ['-g'])
    if int(env['opt']):
        env.Append(CCFLAGS = ['-O2'])
    
    return env


def get_i686_platform(env):
    'Set architecture-specific flags for i686'
    env.Append(CCFLAGS = ['-m32'])
    env.Append(LINKFLAGS = ['-m32'])
    env['PLATFORM'] = 'i686-linux'

def get_x86_64_platform(env):
    'Set architecture-specific flags for x86-64'
    env.Append(CCFLAGS = ['-m64'])
    env.Append(LINKFLAGS = ['-m64'])
    env['PLATFORM'] = 'x86_64-linux'

def get_raspi2_platform(env):
    'Set architecture-specific flags for raspberry pi 2'

    # compiler location
    env['CXX']  = '$toolchain_dir/gcc-linaro-arm-linux-gnueabihf-raspbian/bin/arm-linux-gnueabihf-g++'
    env['LINK'] = '$toolchain_dir/gcc-linaro-arm-linux-gnueabihf-raspbian/bin/arm-linux-gnueabihf-g++'

    if int(env['debug']):
        env.Append(CCFLAGS = ['-gdwarf-3'])

    # Libraries
    env['BOOST_LIBS'] = [b + env['boost_lib_suffix'] for b in BOOST_LIBS]
    env['TOOL_LIBS'] = env['BOOST_LIBS'] + TOOL_LIBS + ['rt']
    env['GPS_LIBS'] = ['gps']

    env['PLATFORM'] = 'armpi-linux'
    return env


if platform.system() in 'Linux':
    # fill in defaults if not set
    dev_dir = os.path.join(os.environ['HOME'], 'dev')
    if not baseEnv['tools_prefix']:
        # the tools_prefix is where to find the locally installed headers and libs
        # for the runtime dependencies
        # by default, this is in $HOME/dev/opt/dust-apc
        baseEnv['tools_prefix'] = os.path.join(dev_dir, 'opt', 'dust-apc')
    if not baseEnv['boost_prefix']:
        # the boost_prefix is where to find the base of the boost sources
        # by default, this is in $HOME/dev/boost_1_60_0
        baseEnv['boost_prefix'] = os.path.join(dev_dir, "boost_1_60_0")
    if not baseEnv['toolchain_dir']:
        baseEnv['toolchain_dir'] = os.path.join(dev_dir, 'arm-bcm2708')
    
    env = getLinuxEnv(baseEnv)

    if env['target'] == 'armpi':
        get_raspi2_platform(env)
    elif env['target'] == 'i686':
        get_i686_platform(env)
    elif env['target'] == 'x86_64':
        get_x86_64_platform(env)
    else: 
        raise UserError('Unknown target architecture')
else:
    print 'Unknown platform:', platform.system()
    Exit(1)


# ----------------------------------------------------------------------
# Common environment

env['BUILD_DIR'] = getBuildDir(env)
# DUST_LIB_DIR is the location of generated libraries
env['DUST_LIB_DIR'] = "#/" + env['BUILD_DIR']+ "/lib" 
env.Append(LIBPATH=env['DUST_LIB_DIR'])

# Build output
# Windows is somewhat different, so we don't change the default behavior there
if not platform.system().startswith('Windows'):
    if not int(env['verbose']):
        env['CCCOMSTR'] = ' [COMPILE] $SOURCE.file'
        env['CXXCOMSTR'] = ' [COMPILE] $SOURCE.file'
        env['LINKCOMSTR'] = ' [LINK] $TARGET.file'
        env['ARCOMSTR'] = ' [ARCHIVE] $TARGET.file'
        env['RANLIBCOMSTR'] = ' [INDEX] $TARGET.file'
    else:
        env['CCCOMSTR'] = ' [COMPILE] $SOURCE.file\n' + env['CCCOM']
        env['CXXCOMSTR'] = ' [COMPILE] $SOURCE.file\n' + env['CXXCOM']
        env['LINKCOMSTR'] = ' [LINK] $TARGET.file\n' + env['LINKCOM']
        env['ARCOMSTR'] = ' [ARCHIVE] $TARGET.file\n' + env['ARCOM']
        if 'RANLIBCOM' in env:
            env['RANLIBCOMSTR'] = ' [INDEX] $TARGET.file\n' + env['RANLIBCOM']

# Common sets of libraries
env['COMMON_LIBS'] = ['logger', 'rpccommon', 'common', ]
env.Append(CPPPATH=['#', 
                    '#/common',
                    '#/shared/include',
                    os.path.join('#', env['BUILD_DIR']),
                    os.path.join('#', env['BUILD_DIR'], 'APInterface'),
                    os.path.join('#', env['BUILD_DIR'], 'logging'),
                    os.path.join('#', env['BUILD_DIR'], 'rpc'),
                    os.path.join('#', env['BUILD_DIR'], 'public'),
                    ])

# Configure the destination directory for published artifacts
if not env['publish_dir']:
    env['publish_dir'] = DEFAULT_PUBLISH_DIR

# Targets for release distribution
env['DIST_TARGETS'] = {}

# ----------------------------------------------------------------------
# Tools initialization    
import gen_enum
import gen_protobuf
import gen_version

gen_enum.init(env)
gen_protobuf.init(env)
gen_version.init(env)

# ----------------------------------------------------------------------
# Read all SConscripts
dirs = [
    'APInterface',
    'common',
    'logging',
    'rpc',
    'watchdog',
]

for d in dirs:
    build_dir = os.path.join(env['BUILD_DIR'], d)
    SConscript(os.path.join(d, 'SConscript.apc'),
               variant_dir = build_dir,
               duplicate = 0,
               exports = {"env": env})

# don't use variant BUILD_DIR with python
SConscript(os.path.join('python', 'SConscript.apc'),
           exports = {"env": env})

# include package / release targets last
build_dir = os.path.join(env['BUILD_DIR'], 'pkg')
SConscript(os.path.join('pkg', 'SConscript'),
           variant_dir = build_dir,
           duplicate = 0,
           exports = {"env": env})
           
# ----------------------------------------------------------------------
# Useful aliases

Alias('all', ['apbridge_pkg'])
# apbridge_publish and apbridge_release are the publishing and release targets
