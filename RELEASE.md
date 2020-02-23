How to release the plugin for "unstable" or "stable" release
============================================================

To release a new version of the plugin using the _managed plugins_ is a two step
process:

1. Publish the "download files" on Cloudsmith.
2. Create a pull request for the OpenCPN plugin project.

There is actually a third step involved, where the OpenCPN developers merge the pull request,
but we don't need to document their work here.

## Publish the "download files" on Cloudsmith

Follow these steps:

1. Check that you are using the _ci_ branch and not _master_ or any other!

    git checkout ci

2. Edit the following lines in CMakeLists.txt to reflect the new status:

    #SET(CMAKE_BUILD_TYPE Debug)
    SET(VERSION_MAJOR "5")
    SET(VERSION_MINOR "0")
    SET(VERSION_PATCH "4-beta1")
    SET(VERSION_DATE "2019-09-08")

3. Add the file to the git staging area, commit this then push it to github:

    git add CMakeLists.txt
    git commit -m"v5.0.4-beta1"
    git push

   If you wait a while you will see builds turn up in Cloudsmith, built by Appveyor, Travis and Cloud CI. See below for the URLs.
4. For "stable" releases only: add a tag and push this tag to github:

    git tag v5.0.4
    git push --tags


The process of adding and pushing the tag will put the releases in the "stable" repository on Cloudsmith.

That concludes the building actions.

## Where can I find the builds

You can check the build progress here:

(Circle CI pipelines)[https://app.circleci.com/github/opencpn-radar-pi/radar_pi/pipelines] (Linux x86 and macOS builds)
(Travis)[https://travis-ci.org/opencpn-radar-pi/radar_pi] (ci branch: Raspberry Pi (armhf) builds)
(Appveyor)[https://ci.appveyor.com/project/keesverruijt/radar-pi] (Microsoft Windows)
(CloudSmith Stable Packages)[https://cloudsmith.io/~kees-verruijt/repos/ocpn-plugins-stable/packages/]
(CloudSmith Unstable Packages)[https://cloudsmith.io/~kees-verruijt/repos/ocpn-plugins-unstable/packages/]

## Create a pull request for `opencpn-plugins`

The (opencpn-radar-pi)[https://github.com/opencpn-radar-pi] organization already contains a fork 
of (OpenCPN/plugins)[https://github.com/OpenCPN/plugins) at
(opencpn-radar-pi)[https://github.com/opencpn-radar-pi/plugins).

1. Clone the _opencpn-radar-pi_ repo, and set upstream, if you have not already done so:
   (See (Configuring a remote for a fork)[https://help.github.com/en/github/collaborating-with-issues-and-pull-requests/configuring-a-remote-for-a-fork])

    git clone git@github.com:opencpn-radar-pi/plugins.git
    cd plugins
    git remote add upstream https://github.com/OpenCPN/plugins.git

2. Sync the fork with the latest info from upstream 
   (See (Syncing a fork)[https://help.github.com/en/github/collaborating-with-issues-and-pull-requests/syncing-a-fork])

    git fetch upstream
    git checkout master
    git merge upstream/master

3. Copy the XML files from CloudSmith to your local plugins repo:

    ./cloudsmith-sync.sh radar_pi kees-verruijt ocpn-plugins-unstable

   This assumes that an up-to-date version of _radar_pi_ is in the directory `../radar_pi`; e.g. you should have
   the `plugins` directory in the same directory as `radar_pi`, and `radar_pi` must be on an up-to-date `ci` branch.
   The script determines the last commit-id and then downloads those files from Cloudsmith.

4. Check that all radar_pi files are updated in the `metadata` subdirectory.

   Check that there are no missing platforms, certainly for a stable release.

5. Create a new combined plugin .xml:

    make

6. Create a Pull Request:

    git add .
    git commit -m"Radar plugin version X.YZ"
    git push

7. Go to github.com and create a Pull Request for `OpenCPN/plugins`




## Oops

_Wrong commit tagged?_

As long as you haven't done `git push --tags` yet you can move the tag to the correct commit by using `git tag -f <tagname> <commitid>` or if the desired commit is the new HEAD (last commit) then `git tag -f <tagname>`.

_Checking if the code builds on macOS + Linux + Windows_

Just push a version, just don't tag it -- Appveyor and Travis will still build it, but you need to go to their website to find the result.
* [https://ci.appveyor.com/project/keesverruijt/radar-pi]
* [https://travis-ci.org/opencpn-radar-pi/radar_pi]
