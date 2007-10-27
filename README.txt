Audit files for Drupal 5.x
==========================

Audit files is a module that is designed to help keep your upload files in
check. It can run two reports, which are access from Administer > Logs >
Audit Files:


Audit files not on the server
-----------------------------
This report lists files that are named in the {files} table in the database
but that do not exist on the server. These missing files may mean that
nodes do not display as expected, for example, images may not display or
downloads may not be available.

From this report you can view or edit the related node to try and discover
what is wrong and fix it by editing the node.


Audit files not in the database
-------------------------------
This report lists files that are on the server but are not referred to by
the {files} table. These may be orphan files whose parent node has been
deleted, or they may be the result of a module not tidying up after itself.

From this report you can mark files for deletion. There is intentionally no
"select all" checkbox because you probably don't want to accidentally get rid
of everything in one hit!

Be careful with the delete feature - the deletion is permanent - be sure the
file is no longer needed before erasing it!

If you're not sure what the file is then you can click on the filename to
open the file to examine it.


Configuration
-------------
There may be some files, folders or extensions that are reported by the audit
that you do not want to be included. You can set exclusions at Administer >
Site configuration > Audit files. By default the audit excludes .htaccess files
and the contents of the color directory.


; $Id$