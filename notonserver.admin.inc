<?php

/**
 * @file
 * Callback to generate files not on server report
 */


/**
 * Menu callback: audit files not on the server.
 */
function auditfiles_notonserver() {

  // Get all the files from the files table using defined sort order
  if (db_table_exists('upload')) {
    // Using a left join means all rows in {files} are selected even if there is no entry in {upload}
    $sql = 'SELECT u.nid, f.filepath FROM {files} AS f LEFT JOIN {upload} AS u ON f.fid = u.fid';

    // Initialise table header to allow sorting
    $header = array(
      array('data' => t('Node'), 'field' => 'u.nid', 'sort' => 'asc'),
      array('data' => t('File'), 'field' => 'f.filepath'),
      array('data' => t('Operations')),
    );
  }
  else {
    // If {upload} doesn't exist do a simpler query
    $sql = 'SELECT f.filepath FROM {files} AS f';

    // Initialise table header to allow sorting
    $header = array(
      array('data' => t('Node')),
      array('data' => t('File'), 'field' => 'f.filepath', 'sort' => 'asc'),
      array('data' => t('Operations')),
    );
  }

  $table_sort = tablesort_sql($header);
  $result = db_query($sql . $table_sort);

  // Initialise array to hold rows of table
  $rows = array();

  // Iterate through the results
  while ($file = db_fetch_object($result)) {

    // Construct a valid drupal path for the named file
    $target = file_create_path($file->filepath);

    // Check to see if the file exists
    if (!file_exists($target)) {

      // If it doesn't strip out the directory path and store the result
      $file->filepath = preg_replace('@^'.preg_quote(file_directory_path()).'/@', '', $file->filepath);

      // Construct table rows, but only make hyperlinks if $file->nid is defined
      if ($file->nid) {
        $rows[] = array(
          array('data' => l($file->nid, 'node/'.$file->nid)),
          array('data' => $file->filepath),
          array('data' => l(t('edit'), 'node/'.$file->nid.'/edit'))
        );
      }
      else {
        $rows[] = array(
          array('data' => ''),
          array('data' => $file->filepath),
          array('data' => '')
        );
      }
    }

  }

  // Create output string
  if ($rows) {
    $output .= format_plural(count($rows), '1 file found.', '@count files found.');
    $output .= theme('table', $header, $rows);
  }
  else {
    $output .= t('No files found.');
  }

  // Return the results
  return $output;
}

