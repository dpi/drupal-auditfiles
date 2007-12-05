<?php
// $Id$

/**
 * @file
 * Callback and functions to generate files not in database report
 */


/**
 * Menu callback: audit files not in the database.
 */
function auditfiles_notindb($form_state) {

  // If delete action was selected, and files were checked, then return confirmation form
  if ($form_state['values']['operation'] == 'delete' && array_filter($form_state['values']['files'])) {
    return auditfiles_multiple_delete_confirm($form_state, array_filter($form_state['values']['files']));
  }

  // Otherwise retreive the form
  $form['auditfiles_notindb'] = auditfiles_notindb_form();

  // Return the form
  return $form;
}


/**
 * Form definition for audit files not in database
 */
function auditfiles_notindb_form() {

  // Get the list of files that aren't in the database
  $filesnotindb = _auditfiles_filesnotindb();

  // Output count of files not in the database
  if ($filesnotindb) {
    $form['count'] = array(
      '#value' => format_plural(count($filesnotindb), '1 file found.', '@count files found.'),
    );
  }
  else {
    $form['count'] = array(
      '#value' => t('No files found.'),
    );
  }

  // Action button
  $form['options'] = array(
    '#type' => 'fieldset',
    '#title' => t('Action'),
    '#prefix' => '<div class="container-inline">',
    '#suffix' => '</div>',
  );

  $options = array(
    'donothing' => t('Do nothing'),
    'delete' => t('Delete checked files'),
  );

  $form['options']['operation'] = array(
                                    '#type' => 'select',
                                    '#options' => $options,
                                    '#default_value' => 'donothing',
                                  );

  $form['options']['submit'] = array(
                                 '#type' => 'submit',
                                 '#value' => t('Update'),
                               );


  // Process each result in turn and build check box list
  $files_dir = file_directory_path();
  $files=array();
  foreach ($filesnotindb as $file) {
    $files[$file] = '';

    // Can't use file_create_url as the links fail if the site uses private transfers
    // Force a public url instead
    $form['file'][$file] = array('#value' => l($file, $GLOBALS['base_url'] .'/'. file_directory_path() .'/'. str_replace('\\', '/', $file)));
  }

  // Add list of files to checkboxes
  $form['files'] = array('#type' => 'checkboxes', '#options' => $files);

  // Specify theme for form
  $form['#theme'] = 'auditfiles_notindb_form';

  // Return form
  return $form;
}


/**
 * Theme auditfiles_notindb_form
 */
function theme_auditfiles_notindb_form($form) {

  // Render count
  $output .= drupal_render($form['count']);

  // If there are files found
  if (isset($form['file']) && is_array($form['file'])) {

    // Render actions
    $output .= drupal_render($form['options']);

    // Construct table of files
    $header = array(
      t('Delete?'),
      t('File')
    );

    foreach (element_children($form['file']) as $key) {
      $row = array();
      $row[] = drupal_render($form['files'][$key]);
      $row[] = drupal_render($form['file'][$key]);
      $rows[] = $row;
    }

    // Render themed table
    $output .= theme('table', $header, $rows);

  }

  // Return output
  return $output;
}


/**
 * Form action: Confirm deletion of selected items
 */
function auditfiles_multiple_delete_confirm(&$form_state, $files = array()) {

  $form['files'] = array('#prefix' => '<ul>', '#suffix' => '</ul>', '#tree' => TRUE);

  // array_filter in the call returned only elements with TRUE values
  foreach ($files as $file) {
    $form['files'][$file] = array('#type' => 'hidden', '#value' => $file, '#prefix' => '<li>', '#suffix' => check_plain($file) ."</li>\n");
  }
  $form['operation'] = array('#type' => 'hidden', '#value' => 'delete');
  $form['#submit'][] = 'auditfiles_multiple_delete_confirm_submit';

  return confirm_form(
    $form,
    t('Are you sure you want to delete these items?'),
    'admin/reports/auditfiles/notindb',
    '<strong>'. t('This action cannot be undone.') .'</strong>',
    t('Delete all'),
    t('Cancel')
  );
}


/**
 * Form action: Delete selected files
 */
function auditfiles_multiple_delete_confirm_submit($form, &$form_state) {
  if ($form_state['values']['confirm']) {
    foreach ($form_state['values']['files'] as $file) {
      if (file_delete(file_create_path($file))) {
        watchdog('audit', '%file was deleted', array('%file' => $file));
      }
      else {
        drupal_set_message(t('Failed to delete %file', array('%file' => $file )));
      }
    }
    drupal_set_message(t('The items have been deleted.'));
  }
  $form_state['redirect'] = 'admin/reports/auditfiles/notindb';
  return;
}


/**
 * Form action: Submit the deletion form
 */
function auditfiles_notindb_submit($form, &$form_state) {
  // Force rebuild to populate form data
  $form_state['rebuild'] = TRUE;

}


/**
 * Helper function - retrieve sorted list of files that are on the server
 * but not in the database
 */
function _auditfiles_filesnotindb() {

  // Prepare array to hold results
  $filesnotindb = array();

  // Get all the files out the {files} table and store as qualified path
  $result = db_query('SELECT filepath FROM {files} ORDER BY filepath ASC');
  $filesindb = array();
  while ($file = db_fetch_object($result)) {
    $filesindb[] = file_create_path($file->filepath);
  }

  // Get all the files out of the directory structure
  $filesonserver = _auditfiles_directorytoarray(realpath(file_create_path()), TRUE);

  // Sort the rows to make it easier to compare to file listing in FTP
  asort($filesonserver);

  // Get the root path - will need this later
  $root .= realpath('./');

  // Get the exclusions string
  $exclusions = _auditfiles_get_exclusions();

  // Process each result in turn
  foreach ($filesonserver as $file) {

    // Strip out the real path to leave just a drupal path
    $file = preg_replace('@'.preg_quote($root).'.@', '', $file);

    // Check it isn't a directory - not interested
    if (!file_check_directory($file)) {

      // Exclude files, paths and extensions according to the retrieved exclusions string
      if (!preg_match('@'.$exclusions.'@', $file) || !$exclusions) {

      // Check to see if file is NOT in the database
        if (!in_array($file, $filesindb) ) {

        // If we get here we have a file that isn't in the database
        $file = preg_replace('@^'.preg_quote(file_directory_path()).'/@', '', $file);
        $filesnotindb[] = $file;
        }
      }
    }
  }

  return $filesnotindb;
}


/**
 * Helper function - recurse directories and files in to an array
 * http://snippets.dzone.com/posts/show/155
 */
function _auditfiles_directorytoarray($directory, $recursive) {
  $array_items = array();
  if ($handle = opendir($directory)) {
  while (false !== ($file = readdir($handle))) {
    if ($file != "." && $file != "..") {
    if (is_dir($directory. "/" . $file)) {
      if($recursive) {
      $array_items = array_merge($array_items, _auditfiles_directorytoarray($directory. "/" . $file, $recursive));
      }
      $file = $directory . "/" . $file;
      $array_items[] = preg_replace("/\/\//si", "/", $file);
      }
      else {
        $file = $directory . "/" . $file;
      $array_items[] = preg_replace("/\/\//si", "/", $file);
      }
    }
    }
    closedir($handle);
  }
  return $array_items;
}


/**
 * Helper function: create an exclusion string for the preg
 */
 function _auditfiles_get_exclusions() {

  // Get exclusion lists from the variables table
  $files = trim(variable_get('auditfiles_exclude_files', '.htaccess'));
  $extensions = trim(variable_get('auditfiles_exclude_extensions', ''));
  $paths = trim(variable_get('auditfiles_exclude_paths', 'color'));

  // Prepare an empty array
  $exclusions_array = array();

  // Create file exclusions as required
  if ($files) {
    $exclude_files = explode(' ', $files);
    array_walk($exclude_files, '_auditfiles_make_preg', FALSE);
    $exclusions_array = array_merge($exclusions_array, $exclude_files);
  }

  // Create path exclusions as required
  if ($paths) {
    $exclude_paths = explode(' ', $paths);
    array_walk($exclude_paths, '_auditfiles_make_preg', TRUE);
    $exclusions_array = array_merge($exclusions_array, $exclude_paths);
  }

  // Create extension exclusions as required (this is a little more complicated)
  if ($extensions) {
    // Prepare initial string as for files and paths
    $exclude_extensions = explode(' ', $extensions);
    array_walk($exclude_extensions, '_auditfiles_make_preg', FALSE);
    $extensions = implode('|', $exclude_extensions);

    // Add grouping around string, add end marker, and append to exlusions_array
    $extensions = '(' . $extensions . ')$';
    $exclusions_array[] = $extensions;

  }

  // Implode exclusions array to a string
  $exclusions = implode('|', $exclusions_array);

  // Return prepared exclusion string
  return $exclusions;

}


/**
 * Helper function: walk an array and preg_quote each entry
 * Pass $makefilepath = TRUE to change elements to file paths at the same time
 */
function _auditfiles_make_preg(&$element, $key, $makefilepath = FALSE) {
  if ($makefilepath) {
    $element = file_create_path($element);
  }
  $element = preg_quote($element);
}
