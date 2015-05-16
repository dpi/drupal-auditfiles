CONTENTS
--------
 * Introduction
 * Reports
 * Migration
 * Issues
 * Troubleshooting (currently empty)
 * FAQ (currently empty)
 * Maintainers (currently empty)

INTRODUCTION
------------
Audit Files is a module that is designed to help keep the files on your server
in sync with those used by your Drupal site.

This module avoids using the Drupal API when dealing with the files and their
references, so that more or different problems are not created when attempting
to fix the existing ones.

The module does use the Drupal API (as much as possible) to reduce the load on
the server, including (but not necessarily limited to) paging the reports and
using the Batch API to perform the various operations.

It includes six reports, which are accessed from Administer > Reports > Audit
Files (admin/reports/auditfiles).

REPORTS
-------
Not in database
---------------
This report lists the files that are on the server but are not in the
file_managed database table. These may be orphan files whose parent node has
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

Not on server
-------------
This report lists the files that are in the file_managed database table but
do not exist on the server. These missing files may mean that nodes do not
display as expected, for example, images may not display or downloads may not be
available.

From this report you can view or edit the related node to try and discover
what is wrong and fix it by editing the node.

Managed not used
----------------
The files listed in this report are in the file_managed database table but not
in the file_usage table. Usually, this is normal and acceptable. This report
exists for completeness, so you may verify what is here is correct.

Used not managed
----------------
The files listed in this report are in the file_usage database table but not in
the file_managed table. Files listed here have had their Drupal management
removed, but are still being listed as used somewhere.

You should verify the file's existence in the server and in the objects it is
listed as being used in, and either delete the reference in this report, or add
it to the file_managed table.

Used not referenced
-------------------
The files listed here are in the file_managed table but no nodes are recorded as
referencing them (i.e., there's no entry in the file_usage table). This might mean
the node has been deleted without deleting the file, or that the files were uploaded
by some means other than the upload module (e.g., via FTP) and the relationships between
files and nodes have not been made. If you have used the File references report and
accounted for all files that should be referenced, and are sure that the files below
are not needed, you can delete them.

Referenced not used
-------------------
Listed here are file references embedded in node bodies which do not have
exact correspondences in the file_managed and file_usage tables. If there is a
file in the file_managed table with a corresponding base name, that is listed.
Scenarios are:

* No match at all in the file_managed table. Go to Files not in database and make
  sure any files that exist have been added to the database. If they have, you 
  should either find and upload the missing file and run this report again, or 
  remove the reference from the node.
* Multiple matches in the file_managed table. This can happen when the same filename
  is in multiple directories in the uploded file hierarchy. You can review the 
  alternate files and delete any that are true duplicates. When different files 
  have the same basename, you can select the one that goes with the given node 
  and choose Attach selected files. This will rewrite the reference in the node 
  to use the canonical relative URL to the file, and if necessary add the reference
  to the file_usage table.
* A single match in the file_managed table. You can make the attachments between these
  nodes and the corresponding files one-by-one by selecting them and choosing Attach 
  selected files, or automatically apply it to all single-match cases with Attach 
  all unique matches.

LIMITING FEATURES EXPLAINED
---------------------------
There are two administrative configuration setttings that help with limiting the
reocrds displayed, for when a report times out or exceeds the available memory.
They are: "Maximum records" and "Batch size." They are both found in the "Report
options" fieldset.

There are four possible combinations of these settings, one of which is invalid:
1) Both set to zero:
   With these settings, all records are loaded and displayed.
2) "Maximum records" set to some positive integer grater than zero and "Batch
   size." set to zero:
   With this combination, only the number of records in "Maximum records" will
   be intially loaded and displayed. At the top of the report page, there will
   be a button labeled "Load all records," with which you can load all records
   using Drupal's Batch API. This combination is also useful if you have a
   sizeable number of records that take a while to display, but don't timeout or
   exceed the memory limit, as it will allow a quicker initial load.
3) "Maximum records" set to zero and "Batch size" set to some positive integer
   grater than zero:
   This combination is invalid, because if "Maximum records" set to zero, it
   does not matter what "Batch size" set to, because the records will never be
   loaded via the Batch API.
4) Both set to some positive integer grater than zero:
   Sometimes, setting "Maximum records" and batch loading all the records isn't
   enough, and a report may still timeout or exhaust the available memory. If
   that is the case, entering a positive integer in this setting will limit the
   batch operation and provide a paging mechanizm for accessing the other
   records.
   To test if this will be helpful or not, set it to a lower number. If the
   report loads, then set it to a higher number to access more records per batch
   load operation. Since it is still using the Batch API, this number can be
   rather high, in an attempt to access as many records as possible.

For all options above, paging can be enabled with the "Number of items per page"
setting.

MIGRATION
---------
In typical usage, the reports can be used independently and the occasional
issues revealed dealt with one-by-one. Another application is the migration of
content and images from another web site (for example, using the node_import
module or pasting HTML content manually into a node creation form), which
typically will break any embedded image references. The file audit tools can
automate much of rectifying this situation.

A typical workflow for migrating content containing embedded images would be:

1. Copy all referenced images to the Drupal files directory (typically
   sites/default/files).
2. Go to the Not in database report, and after sanity-checking the list, execute
   the Add All Files to Database action. This will add each file you've copied
   to the file_managed table.

3. Go to the Missing References report, and after sanity-checking the list execute the
   Attach All Unique Matches action. This will rewrite the image references in your content
   to properly point to their path on the Drupal server, in every case where there is a
   single matching filename.
4. The Missing References report will now show you image references which don't match any
   files in the Drupal files directory - review these to see if you missed any. Some cases
   may be offsite references that can be safely ignored - in other cases, where you can't
   track down the missing image, you can go to the node and edit it to remove the image reference.
5. The Unreferenced report will now show you files that are not in the file_managed and file_usage
   tables. Review these carefully to make sure they aren't being used in some way not picked 
   up by the other tools (e.g., through Javascript). Suggestion: take some sample filenames
   and query the Drupal database directly: 
   SELECT * FROM node_revisions WHERE body LIKE '%file.jpg%'
   If you find any files you're sure are not being used, you may (after making sure you have
   a fresh backup of the directory tree) delete them from the Unreferenced report.

ISSUES
------
Files are associated with nodes using the file_usage table, behind the upload module's back. The
main issue here is that we may associate a single file with multiple nodes, but the upload
module assumes that each file is only associated with a single node and thus deletes the file
when the node is deleted. This suggests that our generated associations should be saved in our
own table, which would require every place that now checks the file_usage table to check two
tables.

The newer functionality is untested with private downloads.

TROUBLESHOOTING
---------------
You receive the following error messages:
* Warning: Unknown: POST Content-Length of [some number] bytes exceeds the
  limit of [some number] bytes in Unknown on line 0
* Warning: Cannot modify header information - headers already sent in Unknown on
  line 0
* (And a number of "Notice: Undefined index:..." messages.)
Set the "Maximum records" and "Batch size" settings on the Audit Files
administrative settings configuration page (admin/config/system/auditfiles), and
then use the "Load all records" button on the report that is producing the
error.

You receive the following error messages:
* Fatal error: Maximum execution time of [some number] seconds exceeded in [path
  to report file] on line [line number]
Set the "Maximum records" and "Batch size" settings on the Audit Files
administrative settings configuration page (admin/config/system/auditfiles), and
then use the "Load all records" button on the report that is producing the
error.

FAQ
---
- None, yet -

MAINTAINERS
-----------

