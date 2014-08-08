<?php
/**
 * @file
 * Callback to generate files not on server report
 */

/**
 * Menu callback: audit files not on the server.
 */
function auditfiles_not_on_server() {
  // Initialize the table header.
  $header = array(
    array(
      'data' => t('File URI'),
      'field' => 'fm.uri',
      'sort' => 'desc',
    ),
    array(
      'data' => t('Referencing entity'),
      'field' => 'fu.id',
    ),
    array(
      'data' => t('Operations'),
    ),
  );
  // Initialize the table data variable.
  $rows = array();

  // Get all the files from the files table using defined sort order.
  // Using a left join means all rows in {file_managed} are selected even if
  // there is no entry in {file_usage}.
  $query = db_select('file_managed', 'fm');
  $query->fields('fm', array('uri'));
  $join_part = $query->leftJoin('file_usage', 'fu', 'fm.fid = fu.fid');
  $query->addField($join_part, 'id');
  $query->addField($join_part, 'type');
  $query->extend('PagerDefault')->limit(25);
  $query->extend('TableSort')->orderByHeader($header);
  $result = $query->execute();

  // Iterate through the results
  foreach ($result as $file) {
    // Construct a valid drupal path for the named file
    $target = drupal_realpath($file->uri);

    // Check to see if the file exists.
    if (!file_exists($target)) {
      // @todo: get the number of times the file has been used, and itemize
      // everywhere it is used.

      // If the file does not exist, strip out the directory path and store the
      // result.
      $file->uri = preg_replace('@^' . preg_quote(file_default_scheme()) . '://@', '', $file->uri);

      // Get information about the entity the file is connected to.
      if (!empty($file->type) && !empty($file->id)) {
        $entity = entity_load($file->type, array($file->id));
        $entity = reset($entity);
        // Set the entity title and link parts.
        if (!empty($entity->title)) {
          if (!empty($file->type) && $file->type == 'node') {
            $file->link = l($entity->title, 'node/' . $file->id);
          }
          else {
            $file->link = $entity->title . ', which is a(n) ' . $file->type . ' entity with the ID of ' . $file->id;
          }
        }
        else {
          $file->link = ' a(n) ' . $file->type . ' entity with the ID of ' . $file->id;
        }
      }

      // Construct table rows, but only make hyperlinks if $file->nid is defined
      if ($file->id) {
        $rows[] = array(
          array('data' => $file->uri),
          array('data' => 'Referenced in ' . $file->link),
          array('data' => l(t('edit'), 'node/' . $file->id . '/edit')),
        );
      }
      else {
        $rows[] = array(
          array('data' => $file->uri),
          array('data' => 'No reference found'),
          array('data' => ''),
        );
      }
    }
  }

  // Create output string
  if ($rows) {
    $output = format_plural(count($rows), '1 file found.', '@count files found.');
    $output .= theme('table', array('header' => $header, 'rows' => $rows, 'attributes' => array('id' => 'sort-table')));
//    $output = theme('table', array('header' => $header, 'rows' => $rows, 'attributes' => array('id' => 'sort-table')));
  }
  else {
    $output = t('No files found.');
  }

  // Add the pager.
  $output .= theme('pager');

  // Return the results
  return $output;
}
