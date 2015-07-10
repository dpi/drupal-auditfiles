CONTENTS
--------
 * Introduction
 * Reports
   - Not in database
   - Not on server
   - Managed not used
   - Used not managed
   - Used not referenced
   - Referenced not used
   - Duplicated files
 * Buttons on reports
 * Troubleshooting
 * Maintainers

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

From this report you can mark files for deletion. There is intentionally no
"select all" check box because you probably don't want to accidentally get rid
of everything in one hit!

Be careful with the delete feature - the deletion is permanent - be sure the
file is no longer needed before erasing it!

If you're not sure what the file is then you can click on the filename to
open the file in your browser.

You can also add one or more files to the file_managed table from this report.

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
removed, but are still being listed as used somewhere and may have content
referencing them.

You should verify the file's existence on the server and in the objects it is
listed as being used in, and either delete the reference in this report, or add
it to the file_managed table (which is a manual process, due to the fact that
the necessary data is not available to this module).

Used not referenced
-------------------
The files listed here are in the file_usage database table, but the content that
has the file field no longer contains the file reference.

Referenced not used
-------------------
Listed here are the file references in file fields attached to entities Which do
not have a corresponding listing in the file_usage table. If there is a file in
the file_managed table with a corresponding base name, it is listed in this
report.
Scenarios are:


BUTTONS ON THE REPORTS
----------------------


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
   sizeable number of records that take a while to display, but don't time out
   or exceed the memory limit, as it will allow a quicker initial load.
3) "Maximum records" set to zero and "Batch size" set to some positive integer
   grater than zero:
   This combination is invalid, because if "Maximum records" set to zero, it
   does not matter what "Batch size" set to, because the records will never be
   loaded via the Batch API.
4) Both set to some positive integer grater than zero:
   Sometimes, setting "Maximum records" and batch loading all the records isn't
   enough, and a report may still time out or exhaust the available memory. If
   that is the case, entering a positive integer in this setting will limit the
   batch operation and provide a paging mechanizm for accessing the other
   records.
   To test if this will be helpful or not, set it to a lower number. If the
   report loads, then set it to a higher number to access more records per batch
   load operation. Since it is still using the Batch API, this number can be
   rather high, in an attempt to access as many records as possible.

For all options above, paging can be enabled with the "Number of items per page"
setting.

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

MAINTAINERS
-----------
