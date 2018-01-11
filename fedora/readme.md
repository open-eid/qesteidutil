# Guide
## to create rpm package and update distribution package

### General Information

These instructions assume, that you have most/all of the tools needed for the compilation of software generally and qesteidutil specifically already installed. For the dependencies of qesteidutil, see the main Readme of the repository.
RPM based distributions such as fedora create packages on the basis of a 'spec file' that contains metadata about the package and instructions on how to compile it.


### Updating the spec file

If a new version of the package is released or a change to the rpm package is desired, the 'spec file' should be updated to reflect the new version and/or necessary changes. Generally fields in the spec file use natural language and should be easy to identify.
Generally this follows this process:

* Increase the version number in the spec file
* If it is just a new release of an existing version, increase that instead
* The file in the repo and that included with fedora reference the url of the relevant release, it will be downloaded for packaging. If that does not work or for testing, change 'source:' to a local tar.gz archive of the linux source downloaded manuall from this' repo releases page, then copy the source archive of the new version into the directory of the project
* For certificates and similar cryptographic checks, config.json and the other config* files also might have to be updated. Please download these and replace the older files. These can be downloaded das described here https://github.com/open-eid/qesteidutil/wiki/DeveloperTips#building. (due to restrictions of the build environment, it is not possible to download them automatically for spec files compatible with the official build environment)
* If you downloaded these additional file, place them in the directory with the spec file.
* Depending on what changed in the new version, other changes might be necessary, such as the inclusion of a new dependency. In this case, the output of the build process should describe which package is missing, this can then be added to the relevant field in the spec file.
* To check if there is already a newer version of the spec file in the offical fedora repositories, please check here: https://admin.fedoraproject.org/pkgdb/package/rpms/qesteidutil/ (this can save a lot of work) which has a link to a small repo with only the files for packaging: http://pkgs.fedoraproject.org/cgit/rpms/qesteidutil.git/ (Package Source). You can then compare the spec files the find necessary changes or just use the other version if it is newer.

### Manually building the package

If all files are in place, packages can be built using fpm or obs. These might be included with your distribution, otherwise you can install fpm with `sudo gem install fpm` or obs from http://openbuildservice.org/ - it can build locall or remote such as on build.opensuse.org, but requires some extra configration if you never used it before.

* run fpm with `fpm -s dir -n qesteidutil -t rpm .` in the directory of the specfile, this will create a corresponding rpm file.
* For obs, please refer to its website.


### Maintaining inclusion of the package with fedora

* At the time of writing, the package is in the process of being included into fedora again. See here: https://bugzilla.redhat.com/show_bug.cgi?id=1519323
* This should conclude shortly, for Fedora 27. If in the future the package should become abandoned again, you can follow the provided instructions to update the package spec file.
* You can then submit a request to revive the package for inclusion in the next version of the distribution.
* For this, create an account on the Fedora Bugzilla (https://bugzilla.redhat.com) and submit a new review request, asking for a reivew of the spec file/package and in the process unretiring the package if necessary. For this, the spec file and resulting package will be reviewed.
* The general procedure for unretiring a package is outlined here: https://fedoraproject.org/wiki/Orphaned_package_that_need_new_maintainers#Claiming_Ownership_of_a_Retired_Package
* For the review, the maintainers of the distribution might have questions or changes, depending on the changes made. For checking, often a tool called Fedora Review is used which generates a nice report and which you can also use to check yourself: https://pagure.io/FedoraReview

### Other RPM based distributions

* As far as known, these instructions and resulting packages should work on all/most RPM based distributions. Exceptions may apply. In case you need more documentation on RPM and packaging, this is a good place to start: https://fedoraproject.org/wiki/How_to_create_an_RPM_package#.25files_section