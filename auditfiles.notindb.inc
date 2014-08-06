<?php

/**
 * @file
 * Callback and functions to generate files not in database report
 */

/**
 * Menu callback: audit files not in the database.
 */
function auditfiles_notindb() {
  return drupal_get_form('auditfiles_notindb_form');
}

/**
 * Form definition for audit files not in database
 */
function auditfiles_notindb_form($form, &$form_state) {
  if (isset($form_state['storage']['confirm'])) {
    return drupal_build_form('auditfiles_notindb_form_confirm', $form_state);
  }
  // Get the list of files that aren't in the database
  $filesnotindb = _auditfiles_filesnotindb();

  // Output count of files not in the database
  if ($filesnotindb) {
    $form['count'] = array(
      '#markup' => format_plural(count($filesnotindb), '1 file found.', '@count files found.'),
    );
  }
  else {
    $form['count'] = array(
      '#markup' => t('No files found.'),
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
    'add' => t('Add checked files to database'),
    'addall' => t('Add all files to database'),
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
  $files = array();
  foreach ($filesnotindb as $file) {
    // Can't use file_create_url as the links fail if the site uses private transfers
    // Force a public url instead
    $files[$file] = l($file, $GLOBALS['base_url'] . '/' . str_replace('\\', '/', $file));
  }

  // Add list of files to checkboxes
  $form['files'] = array(
    '#type' => 'checkboxes',
    '#options' => $files,
  );

  // Return form
  return $form;
}

/**
 * Theme auditfiles_notindb_form
 */
function theme_auditfiles_notindb_form($variables) {
  $variables = $variables['form'];
  $form = $variables['form'];
  // Render count
  $output = drupal_render($form['count']);

  // If there are files found
  if (isset($form['file']) && is_array($form['file'])) {

    // Render actions
    $output .= drupal_render($form['options']);

    // Construct table of files
    $header = array(
      t('Select'),
      t('File'),
    );

    foreach (element_children($form['file']) as $key) {
      $row = array();
      $row[] = drupal_render($form['files'][$key]);
      $row[] = drupal_render_children($form['file'][$key]);
      $rows[] = array('data' => $row);
    }

    // Render themed table
    $output .= theme('table', array('header' => array('header' => $header, 'rows' => $rows)));
  }

  $output .= drupal_render_children($form);

  // Return output
  return $output;
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function auditfiles_notindb_form_submit($form, &$form_state) {
  if ($form_state['clicked_button']['#id'] == 'edit-submit' &&
    $form_state['values']['operation'] <> 'donothing') {
    $form_state['storage']['confirm'] = TRUE;
    $form_state['storage']['values'] = $form_state['values'];
    $form_state['rebuild'] = TRUE;
  }
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function auditfiles_notindb_form_confirm($form, &$form_state) {
  $values = $form_state['storage']['values'];
  $operation = $values['operation'];
  switch ($operation) {
    case 'delete':
      $optype = 'delete';
      $seltype = 'selected';
      break;
    case 'add':
      $optype = 'add';
      $seltype = 'selected';
      break;
    case 'addall':
      $optype = 'add';
      $seltype = 'all';
      break;
    case 'donothing':
    default:
      return;
  }

  $form['changelist'] = array(
    '#prefix' => '<ul>',
    '#suffix' => '</ul>',
    '#tree' => TRUE,
  );

  foreach ($values['files'] as $file => $selected) {
    if ($seltype == 'all' || $selected) {
      if ($optype == 'delete') {
        $message = "<strong>$file</strong> will be deleted";
      }
      else {
        $message = "<strong>$file</strong> will be added to the database";
      }
      $form['changelist'][$file] = array(
        '#type' => 'hidden',
        '#value' => $file,
        '#prefix' => '<li>',
        '#suffix' => $message . "</li>\n",
      );
    }
    else {
      // Unsetting the unprocessed files prevents confirm_submit from dealing with them
      unset($form_state['storage']['values']['files'][$file]);
    }
  }

  $form['operation'] = array(
    '#type' => 'hidden',
    '#value' => $operation,
  );
  $form['#submit'][] = 'auditfiles_notindb_form_confirm_submit';

  return confirm_form(
      $form, t('Are you sure you want to make these changes?'), 'admin/reports/auditfiles/notindb', '<strong>' . t('This action cannot be undone.') . '</strong>', t('Process all'), t('Cancel')
  );
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function auditfiles_notindb_form_confirm_submit($form, &$form_state) {
  if ($form_state['values']['confirm']) {
    $values = $form_state['storage']['values'];
    $operation = $values['operation'];
    if ($operation != 'delete') {
      global $user;
      if (function_exists('finfo_file')) {
        $finfo = @finfo_open(FILEINFO_MIME);
      }
    }
    foreach ($values['files'] as $filename => $selected) {
      if ($operation == 'delete') {
        $root = drupal_realpath("./");
        if (file_unmanaged_delete($root . '/' . $filename)) {
          watchdog('audit', '%file was deleted', array('%file' => $filename));
        }
        else {
          drupal_set_message(t('Failed to delete %file', array('%file' => $filename)));
        }
      }
      else {
        $fullpath = file_prepare_directory($filename);
        $file = new stdClass();
        $file->filename = trim(basename($filename));
        $file->filepath = trim($fullpath);
        // Adapted from imce
        if ($finfo) {
          $type = finfo_file($finfo, $fullpath);
        }
        elseif ($info = @getimagesize($fullpath)) {
          $type = $info['mime'];
        }
        elseif (function_exists('mime_content_type')) {
          $type = mime_content_type($fullpath);
        }
        else {
          $type = 'application/x-download';
        }
        $file->filemime = $type;
        $file->filesize = filesize($fullpath);
        $file->uid = $user->uid;
        $file->status = FILE_STATUS_PERMANENT;
        $file->timestamp = REQUEST_TIME;
        drupal_write_record('files', $file);
      }
    }
    if ($operation == 'delete') {
      drupal_set_message(t('The files have been deleted.'));
    }
    else {
      drupal_set_message(t('The files have been added to the database.'));
    }
    // Clear so our return to the primary form doesn't think we're going to the confirmation form
    unset($form_state['storage']['confirm']);
  }
}

/**
 * Helper function - retrieve sorted list of files that are on the server
 * but not in the database
 */
function _auditfiles_filesnotindb() {

  // Get the root path - will need this later
  $root = drupal_realpath('./');

  // Prepare array to hold results
  $filesnotindb = array();

  // Get all the files out the {file_managed} table and store as qualified path
  $result = db_query('SELECT uri FROM {file_managed} ORDER BY uri ASC');
  $filesindb = array();
  foreach ($result as $file) {
    $path = drupal_realpath($file->uri);
    // Strip out the real path to leave just a drupal path
    $filesindb[] = preg_replace('@' . preg_quote($root) . '.@', '', $path);
  }

  // Get all the files out of the directory structure
  $filesonserver = _auditfiles_directorytoarray(drupal_realpath(file_default_scheme() . '://'), TRUE);

  // Sort the rows to make it easier to compare to file listing in FTP
  asort($filesonserver);

  // Get the exclusions string
  $exclusions = _auditfiles_get_exclusions();

  // Process each result in turn
  foreach ($filesonserver as $file) {

    // Strip out the real path to leave just a drupal path
    $file = preg_replace('@' . preg_quote($root) . '.@', '', $file);

    // Check it isn't a directory - not interested
    if (!file_prepare_directory($file)) {

      // Exclude files, paths and extensions according to the retrieved exclusions string
      if (!preg_match('/' . $exclusions . '/', $file) || !$exclusions) {

        // Check to see if file is NOT in the database
        if (!in_array($file, $filesindb)) {

          // If we get here we have a file that isn't in the database
          $file = preg_replace('/^' . preg_quote(file_default_scheme()) . '/', '', $file);
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
        if (is_dir($directory . "/" . $file)) {
          if ($recursive) {
            $array_items = array_merge($array_items, _auditfiles_directorytoarray($directory . "/" . $file, $recursive));
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
    $realpath = drupal_realpath($element);
    if ($realpath) {
      $element = $realpath;
    }
  }
  $element = preg_quote($element);
}
