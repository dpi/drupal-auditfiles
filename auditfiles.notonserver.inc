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
  // Using a left join means all rows in {files} are selected even if there is no entry in {upload}
  $sql = 'SELECT u.id, f.uri FROM {file_managed} AS f LEFT JOIN {file_usage} AS u ON f.fid = u.fid';

  // Initialise table header to allow sorting
  $header = array(
    array(
      'data' => t('Node'),
      'field' => 'u.id',
      'sort' => 'asc',
    ),
    array(
      'data' => t('File'),
      'field' => 'f.uri',
    ),
    array('data' => t('Operations')),
  );

  $result = db_query($sql);

  // Initialise array to hold rows of table
  $rows = array();

  // Iterate through the results
  foreach ($result as $file) {

    // Construct a valid drupal path for the named file
    $target = drupal_realpath($file->uri);

    // Check to see if the file exists
    if (!file_exists($target)) {

      // If it doesn't strip out the directory path and store the result
      $file->uri = preg_replace('@^' . preg_quote(file_default_scheme()) . '://@', '', $file->uri);

      // Construct table rows, but only make hyperlinks if $file->nid is defined
      if ($file->id) {
        $rows[] = array(
          array('data' => l($file->id, 'node/' . $file->id)),
          array('data' => $file->uri),
          array('data' => l(t('edit'), 'node/' . $file->id . '/edit')),
        );
      }
      else {
        $rows[] = array(
          array('data' => ''),
          array('data' => $file->uri),
          array('data' => ''),
        );
      }
    }
  }

  // Create output string
  if ($rows) {
    $output = format_plural(count($rows), '1 file found.', '@count files found.');
    $output .= theme('table', array('header' => array('header' => $header, 'rows' => $rows)));
  }
  else {
    $output = t('No files found.');
  }

  // Return the results
  return $output;
}
