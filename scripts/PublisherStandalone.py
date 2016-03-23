'''PublisherStandalone.py

Define standard distribute action and add release actions 
and build targets to the environment. 
'''
import sys
import os
import shutil

from VersionStandalone import generate_pyversion, generate_cversion
from VersionStandalone import _get_target_name  
try:
    # it's OK to skip the SVN object when using GIT
    from SvnStandalone import generate_svn
except ImportError: 
    pass
try:
    # it's OK to skip the GIT object when using SVN
    from GitStandalone import generate_git
except ImportError:
    pass


def distribute_action(target, source, env):
    '''
    Publish the files to the release directory
    '''
    # Using env.Install causes scons to break the target path. Scons assumes
    # the target is on the current drive (even with a \\ prefix).
    dry_run = env.has_key('dry_run') and int(env['dry_run'])

    if not os.path.isdir(env['DEST_DIR']) and not dry_run:
        os.makedirs(env['DEST_DIR'])

    # TODO: PUBLIC_NAME_REMAP
    # { '_name_': [ addVersion, addPlatform ]
    #   '_DEFAULT_': }

    version_obj = env['VERSION_OBJECT']

    if env['USE_VERSIONED_DEST']: 
        # create a versioned destination directory
        version_dir = env['USE_VERSIONED_DEST'] + '-' + version_obj.get_version_str()
        dest_dir = os.path.join(env['DEST_DIR'], version_dir)
        # TODO: error if this already exists
        if not os.path.isdir(dest_dir) and not dry_run:
            os.mkdir(dest_dir)
    else:
        dest_dir = env['DEST_DIR']

    for src in source:
        print "Publishing %s to %s" % (src.path, dest_dir)
        if env['APPEND_VERSION']:
            src_tail, src_ext = os.path.splitext(os.path.split(src.path)[1])
            dest_file = '%s_%s%s' % (src_tail, version_obj.get_version_str(), src_ext)
        else:
            dest_file = os.path.split(src.path)[1]
        if not dry_run:
            shutil.copyfile(src.path, os.path.join(dest_dir, dest_file))
        else:
            print 'DRY RUN: copying %s to %s' % (src.path, os.path.join(dest_dir, dest_file))

        # make additional copy to Repository script's incoming direcrtory if it was set
        src_tail, src_ext = os.path.splitext(src.path)
        if env.has_key('REPO_DIR') and len(env['REPO_DIR']) > 0 and src_ext == '.deb':
            repo_dir = env['REPO_DIR']
            print "Publishing %s to %s" % (src.path, repo_dir)
            if not dry_run:
                shutil.copyfile(src.path, os.path.join(repo_dir, dest_file))
                # repo scan scripts requires file mode 0777 to remove the file after adding to repository
                os.chmod(os.path.join(repo_dir, dest_file), 0777)
            else:
                print 'DRY RUN: copying %s to %s' % (src.path, os.path.join(repo_dir, dest_file))
    return 0


def generate_distribute(env, artifacts, dest_dir,
                        use_versioned_dest = False,
                        append_version = False, 
                        repository_dir = '',
                        alias_prefix = ''):
    if alias_prefix:
        version_obj = env['Version'][alias_prefix]
    else:
        version_obj = env['Version']['_default_']
    # TODO: create dict for options
    dummy_target = _get_target_name('publish', alias_prefix)
    publish = env.Command('always.' + dummy_target, artifacts,
                          env.Action(distribute_action),
                          DEST_DIR=dest_dir,
                          USE_VERSIONED_DEST=use_versioned_dest,
                          APPEND_VERSION=append_version,
                          REPO_DIR = repository_dir,
                          VERSION_OBJECT=version_obj)
    env.AlwaysBuild(publish)
    env.Alias(dummy_target, publish)
    return publish

def generate_release(env, version_module = '', version_file = '', build_file = '', version_attr = '',
                     svn_project_path = '', svn_label_prefix = '', svn_binary = '', git_binary = '',
                     release_artifacts = None, release_dest_dir = '', release_use_versioned_dest = False, release_append_version = False,
                     repository_dir = '',
                     alias_prefix = ''):
    'Generate the Version and Svn objects and release build targets'
    # TODO: merge default arguments

    # generate the Version object
    if version_module:
        version_obj = generate_pyversion(env, version_module, version_file, 
                                         build_file,
                                         version_attr, alias_prefix = alias_prefix)
    elif build_file:
        version_obj = generate_cversion(env, version_file, build_file,
                                        alias_prefix = alias_prefix)
    else:
        raise RuntimeError('error: can not construct Version object')

    if git_binary:
        # generate the Git object
        tag_incr = generate_git(env, label_prefix = svn_label_prefix,
                                git_binary = git_binary, alias_prefix = alias_prefix)
    else:
        # generate the Svn object
        tag_incr = generate_svn(env, project_path = svn_project_path, 
                                label_prefix = svn_label_prefix,
                                svn_binary = svn_binary, alias_prefix = alias_prefix)

    # generate the publish target
    publish = generate_distribute(env, release_artifacts, release_dest_dir,
                                  release_use_versioned_dest,
                                  release_append_version, 
                                  repository_dir,
                                  alias_prefix = alias_prefix)

    # create the master release target
    env.Alias(_get_target_name('release', alias_prefix), [publish, tag_incr])
