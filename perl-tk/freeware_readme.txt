Perl/Tk, Programming, Perl/Tk-804.026 for OpenVMS

This is an OpenVMS port of Perl/Tk-804.026. 

As part of the build of this port, HP C for OpenVMS version V7.1-015
and Perl for VMS Alpha V5.8.6 were utilized on OpenVMS V7.3-2.

When rebuilding, unzip and untar the contents into the directory
ddcu:[Tk-804_026...].  Then issue the following commands:

$ define ptk disk:[Tk-804_026.ptk]
$ define mtk disk:[Tk-804_026.ptk.mtk]
$ perl Makefile.PL
$ mms install

To check the results of the build, issue the command:
$ perl demos/hello

Porting work done by Philippe Vouters (HP France), using OpenVMS Alpha V7.3-2.

