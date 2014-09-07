#!/bin/sh
set -e

# Make a local, clean libjpeg-turbo branch that tracks the remote libjpeg-turbo.
# This will allow pushing of imported libjpeg-turbo commits to the mozjpeg repository.
# The libjpeg-turbo branch must only contain imported SVN commits (with git-svn-id: in the message).
git branch -f -t libjpeg-turbo origin/libjpeg-turbo

# Configure git-svn. "git svn fetch" will rebuild remaining git-svn metadata.
git config svn-remote.svn.url svn://svn.code.sf.net/p/libjpeg-turbo/code
git config svn-remote.svn.fetch trunk:refs/heads/libjpeg-turbo

# Enable mapping of SVN usernames to git authors.
git config svn.authorsfile .gitauthors

# Mark which libjpeg-turbo commit has been used to start mozjpeg.
# Required for accurate merging and blame.
echo > .git/info/grafts "72b66f9c77b3e4ae363b21e48145f635cec0b193 540789427ccae8e9e778151cbc16ab8ee88ac6a8"

# To get changes from SVN:
# git svn fetch
# git push origin libjpeg-turbo
#
# To merge SVN changes with mozjpeg:
# git checkout master
# git merge libjpeg-turbo
