About Audit Files
=================
Audit Files is a module that is designed to help keep files on your server in
sync with those used by your Drupal site. It can run four reports, which are
accessed from Administer > Reports > Audit Files:

Not in the database
-------------------
This report lists the files that are on the server but are not in the
{files_managed} database table. These may be orphan files whose parent node has
been deleted, or they may be the result of a module not tidying up after itself,
or they may be the result of uploading files outside of Drupal (e.g., via FTP).
You can sort the table by node number or by filename as you prefer.

From this report you can mark files for deletion. There is intentionally no
"select all" check box because you probably don't want to accidentally get rid
of everything in one hit!

Be careful with the delete feature - the deletion is permanent - be sure the
file is no longer needed before erasing it!

If you're not sure what the file is then you can click on the filename to
open the file in your browser.

Not on the server
-----------------
This report lists the files that are in the {files_managed} database table but
do not exist on the server. These missing files may mean that nodes do not
display as expected, for example, images may not display or downloads may not be
available.

From this report you can view or edit the related node to try and discover
what is wrong and fix it by editing the node.

Unreferenced
------------
The files listed here are in the {file_managed} table but no nodes are recorded as
referencing them (i.e., there's no entry in the {file_usage} table). This might mean
the node has been deleted without deleting the file, or that the files were uploaded
by some means other than the upload module (e.g., via FTP) and the relationships between
files and nodes have not been made. If you have used the File references report and
accounted for all files that should be referenced, and are sure that the files below
are not needed, you can delete them.

Missing references
------------------
Listed here are file references embedded in node bodies which do not have
exact correspondences in the {files_managed} and {file_usage} tables. If there is a
file in the {files_managed} table with a corresponding base name, that is listed.
Scenarios are:

* No match at all in the {files_managed} table. Go to Files not in database and make
  sure any files that exist have been added to the database. If they have, you 
  should either find and upload the missing file and run this report again, or 
  remove the reference from the node.
* Multiple matches in the {files_managed} table. This can happen when the same filename
  is in multiple directories in the uploded file hierarchy. You can review the 
  alternate files and delete any that are true duplicates. When different files 
  have the same basename, you can select the one that goes with the given node 
  and choose Attach selected files. This will rewrite the reference in the node 
  to use the canonical relative URL to the file, and if necessary add the reference
  to the {file_usage} table.
* A single match in the {files_managed} table. You can make the attachments between these
  nodes and the corresponding files one-by-one by selecting them and choosing Attach 
  selected files, or automatically apply it to all single-match cases with Attach 
  all unique matches.

Configuration
-------------
There may be some files, folders or file extensions that are reported by Audit
Files that you do not want to be included. You can set exclusions at Administer >
Site configuration > Audit Files. By default the audit excludes .htaccess files
and the contents of the color directory.

Migration
---------
In typical usage, the reports can be used independently and the occasional issues
revealed dealt with one-by-one. Another application is the migration of content and
images from another web site (for example, using the node_import module or pasting
HTML content manually into a node creation form), which typically will break any
embedded image references. The file audit tools can automate much of rectifying this
situation.

A typical workflow for migrating content containing embedded images would be:

1. Copy all referenced images to the Drupal files directory (typically sites/default/files).
2. Go to the Not In Database report, and after sanity-checking the list execute the Add All 
   Files to Database action. This will add each file you've copied in to the {files_managed} table.
3. Go to the Missing References report, and after sanity-checking the list execute the
   Attach All Unique Matches action. This will rewrite the image references in your content
   to properly point to their path on the Drupal server, in every case where there is a
   single matching filename.
4. The Missing References report will now show you image references which don't match any
   files in the Drupal files directory - review these to see if you missed any. Some cases
   may be offsite references that can be safely ignored - in other cases, where you can't
   track down the missing image, you can go to the node and edit it to remove the image reference.
5. The Unreferenced report will now show you files that are not in the {files_managed} and {file_usage}
   tables. Review these carefully to make sure they aren't being used in some way not picked 
   up by the other tools (e.g., through Javascript). Suggestion: take some sample filenames
   and query the Drupal database directly: 
   SELECT * FROM node_revisions WHERE body LIKE '%file.jpg%'
   If you find any files you're sure are not being used, you may (after making sure you have
   a fresh backup of the directory tree) delete them from the Unreferenced report.
   
Issues
------
Files are associated with nodes using the {file_usage} table, behind the upload module's back. The
main issue here is that we may associate a single file with multiple nodes, but the upload
module assumes that each file is only associated with a single node and thus deletes the file
when the node is deleted. This suggests that our generated associations should be saved in our
own table, which would require every place that now checks the {file_usage} table to check two
tables. 

The newer functionality is untested with private downloads.
