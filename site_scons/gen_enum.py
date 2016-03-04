import os
from subprocess import Popen, PIPE, STDOUT
import SCons

# ----------------------------------------------------------------------
# Enum to string code generation

ENUM_TO_STRING_SCRIPT = 'scripts/genEnumToString.py'
FIND_DEPENDS_SCRIPT = 'scripts/genEnumToStringDependence.py'

# ----------------------------------------------------------------------
# Special builder for libraries
# - find generated sources
# - create libraries in a central directory

# Find generated sources

def read_generated_dependency(env, src_dir):
    top_dir = env.Dir('#')
    cmd = [env['python'], os.path.join(str(top_dir), FIND_DEPENDS_SCRIPT), src_dir, '', str(top_dir)]
    proc = Popen(cmd, stdout=PIPE)
    out, err = proc.communicate()
    rc = proc.returncode
    # TODO: check rc
    results = []
    for line in out.split('\n'):
        if line.strip():
            gen_file, src_file = [s.strip() for s in line.split(':')]
            results.append((gen_file, src_file))
    return results

# Build and register a library in the central lib/ directory

def dust_library_emitter(target, source, env):
    '''Copy Library targets to a central build directory'''
    gen_target = os.path.join(env['DUST_LIB_DIR'], os.path.split(target[0].path)[1])
    src_dir = source[0].srcnode().dir.abspath
    gen_sources = []
    depends = read_generated_dependency(env, src_dir)
    for gen_file, src_file in depends:
        gen_src = env.EnumToString(gen_file, os.path.join('#', src_file))
        env.Depends(gen_file, os.path.join('#', ENUM_TO_STRING_SCRIPT))
        env.Alias('codegen', gen_file)
        gen_sources.append(env.Object(gen_src))
    return gen_target, source + gen_sources

def init(env) :
    s = '{0} {1} $SOURCE.abspath $TARGET.dir "{2}"'.format(env['python'], ENUM_TO_STRING_SCRIPT, str(env.Dir('#').abspath))
    enum_to_string_bld = SCons.Builder.Builder(action = SCons.Action.Action(s, ' [CODEGEN] $SOURCE'),
                             suffix = '_enum.cpp',
                             src_suffix = '.h',
                             )
    env.Append(BUILDERS = {'EnumToString' : enum_to_string_bld})
    env.Append(BUILDERS = {'EnumToString' : enum_to_string_bld})

    dustlib = SCons.Builder.Builder(action = env['BUILDERS']['Library'].action,
                      prefix = '$LIBPREFIX',
                      suffix = '$LIBSUFFIX',
                      src_suffix = '$OBJSUFFIX',
                      src_builder = 'StaticObject',
                      emitter = dust_library_emitter)
    env.Append(BUILDERS = {'DustLibrary' : dustlib})

   