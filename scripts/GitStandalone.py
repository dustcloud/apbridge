import os
from subprocess import *

def reduce_dir(base, offset, dir):
    '''
    Utility function that uses offset to remove elements from the base
    directory.
    Returns: rewritten base + '/' + dir
    
    Offset should have the form of zero or more .. separated with /
    '''
    up_dirs = len(offset.split('/')) - 1
    if up_dirs > 0:
        base = '/'.join(base.split('/')[0:-up_dirs])  
    return "%s/%s" % (base, dir)


class GitStandalone(object):
    """
    Functions related to using Git from within builds.
    This class does not require the Release heirarchy. 

    This class requires the variable GIT_BINARY to be defined in env.

    Methods pay attention to the env variable 'dry_run'. If dry_run is True,
    commands that make modifications to the repository are only printed.
    """
    DEFAULT_REMOTE = 'origin'
    DEFAULT_BRANCH = 'master'

    def __init__(self, version_object = None, 
                 remote_name = None, branch_name = None):
        self.version_object = version_object
        self.remote_name = remote_name if remote_name else self.DEFAULT_REMOTE
        self.branch_name = branch_name if branch_name else self.DEFAULT_BRANCH

    def run_cmd(self, cmd, dry_run=False):
        if dry_run:
            print "DRY RUN:", cmd
            return (0, "")
        print cmd

        p = Popen(cmd, stdin=None, stdout=PIPE, stderr=STDOUT)
        output = p.communicate()[0]
        status = p.returncode
        return status, output
    
    def tag(self, label, env):
        '''
        Tag the current repository with the given label. Return 1 on error.
        Current directory is where SConstruct is, i.e. top

        Note that this does not tag files changed during the build.

        env requirements:
        * GIT_BINARY must be the path to the git command
        (previous versions used SHARED_DIR to get the path to the shared directory)
        '''
        try:
            dry_run = int(env['dry_run'])
        except KeyError:
            dry_run = 0

        status, output = self.run_cmd([env['GIT_BINARY'], 'status'])
        if status:
            print "Unable to determine local git information"
            return status

        # TODO: anything to extract? current branch? rev id?

        print "Start tagging with label: %s" % (label)
        cmd = [ env['GIT_BINARY'], "tag", "-a",
                '-m', "Tagging release build %s" % label,
                label]
        status, output = self.run_cmd(cmd, dry_run)
        if output:
            print output
        if status:
            # TODO: failure conditions?
            return status

        return self.push(env['GIT_BINARY'], dry_run, push_tags=True)

    def tag_exists(self, label, env):
        """If the tag already exists in git, return True.

        env requirements:
        * GIT_BINARY must be the path to the git command
        """
        status, output = self.run_cmd([env['GIT_BINARY'], 'tag', '-l', label])
        return not status

    def commit_build_number(self, build_number_file, from_build, env):
        """
        Commit the changes to the build file. Return non-zero on error.

        env requirements:
        * GIT_BINARY must be the path to the git command
        """
        try:
            dry_run = int(env['dry_run'])
        except KeyError:
            dry_run = 0

        # verify we are on a branch, not in a detached state
        cmd = [env['GIT_BINARY'], 'status']
        status, output = self.run_cmd(cmd)
        branch_status = output.split('\n')[0].lower()
        if status or not ('on branch' in branch_status):
            print 'error: can not commit to non-branch: %s' % (branch_status)
            return 1

        # commit the build number locally
        next_build_number = int(from_build) + 1
        msg = "Automatic increment of the build number from %s to %s" % (from_build, next_build_number)
        status = self.commit(build_number_file, msg, env['GIT_BINARY'], dry_run)
        if status:
            # TODO: failure conditions?
            return status

        return self.push(env['GIT_BINARY'], dry_run)

    def commit(self, filepath, commit_msg, git_binary, dry_run):
        'Commit a file to the local repository'
        cmd = [git_binary, 'commit', '-m', commit_msg, filepath]
        status, output = self.run_cmd(cmd, dry_run)
        if output:
            print output
        return status

    def push(self, git_binary, dry_run, push_tags = False):
        'Push local changes to the remote repository'
        print "Pushing to remote repository"
        cmd = [git_binary, 'push', self.remote_name, self.branch_name]
        if push_tags:
            cmd.append('--follow-tags')
        status, output = self.run_cmd(cmd, dry_run)
        if output:
            print output
        return status


    # Scons actions
    
    def increment_and_commit_action(self, target, source, env):
        '''Increment the build number, commit and push

        The version_object is used to perform the version-related tasks
        '''
        if not self.version_object:
            raise RuntimeError('No version object specified to bump version')

        from_build = self.version_object.get_current_build_number()

        rc = self.version_object.increment_version_action(target, source, env)
        if rc != 0:
            return 1

        build_file = self.version_object.get_build_file()
        return self.commit_build_number(build_file, from_build, env)

    def tag_and_increment_action(self, target, source, env):
        '''Tag, increment the build number, commit and push

        The version_object is used to perform the version-related tasks
        '''
        if not self.version_object:
            raise RuntimeError('No version object specified to bump version')

        label = '{0}_{1}'.format(env['label_prefix'], 
                                 self.version_object.get_version_str())
        rc = self.tag(label, env)
        if rc != 0:
            return 1

        return self.increment_and_commit_action(target, source, env)


def _get_target_name(base_name, prefix = ''):
    target_name = base_name
    if prefix:
        target_name = prefix + '_' + base_name
    return target_name

def generate_git(env, label_prefix, git_binary = '', alias_prefix = ''): 
    '''Create the Git object in the environment
    Generate the tag_incr target.
    '''
    # don't depend on 'git' in environment, allow command line parameter
    if git_binary:
        env['GIT_BINARY'] = git_binary
    else:
        env['GIT_BINARY'] = env['git']

    obj_name = alias_prefix
    if not alias_prefix:
        obj_name = '_default_'

    if not 'Version' in env and not obj_name in env['Version']:
        raise RuntimeError('Version object not created. Generate the Version before generating the Git object.')

    remote_name = None
    branch_name = None
    if 'GIT_BRANCH' in os.environ:
        # GIT_BRANCH has the format 'origin/master'
        remote_branch = os.environ['GIT_BRANCH']
        branch_idx = remote_branch.find('/')
        if branch_idx > 0:
            remote_name = remote_branch[:branch_idx]
            branch_name = remote_branch[branch_idx+1:]

    if not 'Git' in env:
        env['Git'] = {}
    if not env['Git'].has_key(obj_name):
        git_obj = GitStandalone(version_object = env['Version'][obj_name],
                                remote_name = remote_name, branch_name = branch_name)
        env['Git'][obj_name] = git_obj
    # define the tag_incr build target
    dummy_target = _get_target_name('tag_incr', alias_prefix)
    tag_incr = env.Command('always.' + dummy_target, [  ],
                           git_obj.tag_and_increment_action,
                           label_prefix=label_prefix)
    env.AlwaysBuild(tag_incr)
    env.Alias(dummy_target, tag_incr)
    return tag_incr
