<?php
/**
 * @file
 * This generates a report showing all the files on the server that are not in
 * the database.
 */

/**
 * Generates the report.
 *
 * @param array $form
 *   The form definition.
 * @param array $form_state
 *   The current state of the form.
 *
 * @return array
 *   The form definition.
 */
function auditfiles_not_in_database_form($form, &$form_state) {
  // Check to see if the confirmation form needs to be displayed instead of the
  // normal form.
  if (isset($form_state['storage']['confirm'])) {
    return _auditfiles_not_in_database_confirm_operation($form, $form_state);
  }

  // Set up the table header.
  $header = array(
    'filepathname' => array(
      'data' => t('File pathname'),
    ),
    'filemime' => array(
      'data' => t('MIME'),
    ),
    'filesize' => array(
      'data' => t('Size (in bytes)'),
    ),
    'filemodtime' => array(
      'data' => t('Last modified'),
    ),
  );

  // Get the admin specified date format for the file modification time.
  $date_format = variable_get('auditfiles_report_options_date_format', 'long');

  // Get the list of files that aren't in the database.
  $files_not_in_database = _auditfiles_get_files_not_in_database();
  asort($files_not_in_database);
  // Process the data.
  $rows = array();
  foreach ($files_not_in_database as $file) {
    $row = array(
      'filepathname' => $file,
      'filemime' => file_get_mimetype($file),
      'filesize' => number_format(filesize($file)),
      'filemodtime' => format_date(filemtime($file), $date_format),
    );
    $rows[$file] = $row;
  }

  // Define the form table.
  $form_count = format_plural(
    count($rows),
    'Found 1 file on the server that is not in the database.',
    'Found @count files on the server that are not in the database.'
  );
  $form['files'] = array(
    '#type' => 'tableselect',
    '#header' => $header,
    '#options' => $rows,
    '#empty' => t('No unmatched files found.'),
    '#prefix' => '<div><em>' . $form_count . '</em></div>',
  );

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['add'] = array(
    '#type' => 'submit',
    '#value' => t('Add selected items to the database'),
  );
  $form['actions']['markup'] = array(
    '#markup' => '&nbsp;' . t('or') . '&nbsp;',
  );
  $form['actions']['delete'] = array(
    '#type' => 'submit',
    '#value' => t('Delete selected items from the server'),
  );

//  $form['pager'] = array('#markup' => theme('pager'));

  return $form;
}

/**
 * Submit handler for the auditfiles_not_in_database_form form.
 */
function auditfiles_not_in_database_form_submit($form, &$form_state) {
  // Check to see if any files were selected.
  if (!empty($form_state['storage']['files'])) {
    foreach ($form_state['storage']['files'] as $filename => $file_id) {
      if (!empty($file_id)) {
        // The operation has been confirmed.
        if ($form_state['storage']['op'] == 'Add selected items to the database') {
          // Add the file to the database.
          _auditfiles_not_in_database_add_to_database($filename);
        }
        elseif ($form_state['storage']['op'] == 'Delete selected items from the server') {
          // Delete the file from the server.
          _auditfiles_not_in_database_delete_from_server($filename);
        }
      }
    }
  }
  elseif (!empty($form_state['values']['files'])) {
    foreach ($form_state['values']['files'] as $filename => $file_id) {
      if (!empty($file_id)) {
        // At least one file was selected. Check to see if the confirmation form
        // has been processed.
        if (!isset($form_state['values']['process'])) {
          // The operation has not been confirmed, so modify the data to display
          // the confirmation form.
          $form_state['storage']['files'] = $form_state['values']['files'];
          $form_state['storage']['op'] = $form_state['values']['op'];
          $form_state['storage']['confirm'] = TRUE;
          $form_state['rebuild'] = TRUE;
          return TRUE;
        }
      }
    }

    drupal_set_message(t('No items were selected to operate on.'));
  }
}

/**
 * Presents a confimation form to verify the user wants to complete the action.
 *
 * @param array $form
 *   The form definition.
 * @param array $form_state
 *   The current state of the form.
 *
 * @return array
 *   A form array for a confirmation form.
 */
function _auditfiles_not_in_database_confirm_operation($form, &$form_state) {
  $values = $form_state['input'];

  $form['changelist'] = array(
    '#prefix' => '<ul>',
    '#suffix' => '</ul>',
    '#tree' => TRUE,
  );

  // Prepare the list of tiems to present to the user.
  if (!empty($values['files'])) {
    foreach ($values['files'] as $filename => $file_id) {
      if (!empty($file_id)) {
        if ($values['op'] == 'Add selected items to the database') {
          $message = t('will be added to the database.');
        }
        elseif ($values['op'] == 'Delete selected items from the server') {
          $message = t('will be deleted from the server.');
        }
        $form['changelist'][$file_id] = array(
          '#type' => 'hidden',
          '#value' => $filename,
          '#prefix' => '<li><strong>' . $file->filename . '</strong> ' . $message,
          '#suffix' => "</li>\n",
        );
      }
      else {
        // Unsetting the unprocessed files prevents confirm_submit from dealing
        // with them.
        unset($form_state['input']['files'][$file_id]);
      }
    }
  }

  // Tell the submit handler to process the confirmation.
  $form['process'] = array(
    '#type' => 'hidden',
    '#value' => 'TRUE'
  );
  // Go back to the main form, when done with this one.
  $form['destination'] = array(
    '#type' => 'hidden',
    '#value' => 'admin/reports/auditfiles/notindatabase'
  );

  $form['operation'] = array(
    '#type' => 'hidden',
  );

  if ($values['op'] == 'Add selected items to the database') {
    $form['operation']['#value'] = 'add';

    $confirm_question = t('Add these files to the database?');
    $confirm_description = '';
  }
  elseif ($values['op'] == 'Delete selected items from the server') {
    $form['operation']['#value'] = 'delete';

    $confirm_question = t('Delete these files from the server?');
    $confirm_description = '<strong>' . t('This action cannot be undone.') . '</strong>';
  }

  return confirm_form(
    $form,
    $confirm_question,
    'admin/reports/auditfiles/notindatabase',
    $confirm_description,
    t('Yes'),
    t('No')
  );
}

/**
 * Adds the specified file to the database.
 *
 * @param string $filename
 *   The full pathname to the file to add to the database.
 */
function _auditfiles_not_in_database_add_to_database($filename) {
  global $user;
  $file = new stdClass();
  $file->uid = $user->uid;
  // Get the scheme's path.
  $scheme_path = variable_get('file_' . file_default_scheme() . '_path');
  // Get the path to the file relative to it's file storage location.
  $filename = preg_replace('@' . preg_quote(drupal_realpath('./')) . '.@', '', $filename);
  $filename = preg_replace('@' . preg_quote($scheme_path) . '.@', '', $filename);
  $file->filename = trim(basename($filename));
  $file->uri = file_build_uri($filename);
  $file->filemime = file_get_mimetype($file->uri);
  $file->filesize = filesize(drupal_realpath($file->uri));
  $file->status = FILE_STATUS_PERMANENT;
  $file->timestamp = REQUEST_TIME;

  $results = drupal_write_record('file_managed', $file);

  if ($results === FALSE) {
    drupal_set_message(t('Failed to add %file to the database.', array('%file' => $filename)));
  }
  else {
    drupal_set_message(t('Sucessfully added %file to the database.', array('%file' => $filename)));
  }
}

/**
 * Deletes the specified file from the server.
 *
 * @param string $filename
 *   The full pathname of the file to delete from the server.
 */
function _auditfiles_not_in_database_delete_from_server($filename) {
  // Get the actual path to the Drupal root directory.
  $root = drupal_realpath('./');
  // Make sure the Drupal root path is not already in the filename.
  $filename = preg_replace('@' . preg_quote(drupal_realpath('./')) . '.@', '', $filename);
  // Delete the file without invoking any hooks.
  if (file_unmanaged_delete($root . '/' . $filename)) {
    drupal_set_message(t('Sucessfully deleted %file from the server.', array('%file' => $filename)));
  }
  else {
    drupal_set_message(t('Failed to delete %file from the server.', array('%file' => $filename)));
  }
}

/**
 * Retrieves a list of files that are on the server but not in the database.
 *
 * @return array
 *   An array of file names not found in the database.
 */
function _auditfiles_get_files_not_in_database() {
  // Initialize variables.
  $files_in_database = array();
  $files_not_in_database = array();
  // Get the root path.
  $root = drupal_realpath('./');

  // Get all the files from the {file_managed} table and store them as qualified
  // path.
  $result = db_query('SELECT uri FROM {file_managed} ORDER BY uri ASC');
  foreach ($result as $file) {
    $files_in_database[] = drupal_realpath($file->uri);
  }

  // Get the exclusions string.
  $exclusions = _auditfiles_get_exclusions();

  // Get all the files out of the directory structure
  $files_on_server = _auditfiles_get_file_list(drupal_realpath(file_default_scheme() . '://'), TRUE);
  // Sort the rows to make it easier to compare to file listing.
  asort($files_on_server);
  // Process each result in turn
  foreach ($files_on_server as $file) {
    // Skip directory entries.
    if (!file_prepare_directory($file)) {
      // Exclude files, paths and extensions according to the admin specified
      // exclusions list.
      if (!preg_match('@' . $exclusions . '@', $file) || !$exclusions) {
        // Check to see if file is not in the database.
        if (!in_array($file, $files_in_database)) {
          // This file is not in the database.
          $file = preg_replace('@^' . preg_quote(file_default_scheme()) . '@', '', $file);
          $files_not_in_database[] = $file;
        }
      }
    }
  }

  return $files_not_in_database;
}

/**
 * Recurse directories and add files to an array.
 *
 * @see http://snippets.dzone.com/posts/show/155
 *
 * @param string $directory
 *   The directory to find the files in.
 * @param boolean $recursive
 *   Whether to recurse into subdirectories or not.
 *
 * @return array
 *   A collection of files.
 */
function _auditfiles_get_file_list($directory, $recursive) {
  $array_items = array();
  if ($handle = opendir($directory)) {
    while (($file = readdir($handle)) !== FALSE) {
      if ($file != "." && $file != "..") {
        if (is_dir($directory . "/" . $file)) {
          if ($recursive) {
            $array_items = array_merge($array_items, _auditfiles_get_file_list($directory . "/" . $file, $recursive));
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
 * Creates an exclusion string.
 *
 * This function creates a list of file and/or directory exclusions to be used
 * with a preg_* function.
 *
 * @return string
 *   The excluions.
 */
function _auditfiles_get_exclusions() {
  $exclusions_array = array();

  // Create the list of requested file exclusions.
  $files = trim(variable_get('auditfiles_exclude_files', '.htaccess'));
  if ($files) {
    $exclude_files = explode(' ', $files);
    array_walk($exclude_files, '_auditfiles_make_preg', FALSE);
    $exclusions_array = array_merge($exclusions_array, $exclude_files);
  }

  // Create the list of requested path exclusions.
  $paths = trim(variable_get('auditfiles_exclude_paths', 'color css ctools js'));
  if ($paths) {
    $exclude_paths = explode(' ', $paths);
    array_walk($exclude_paths, '_auditfiles_make_preg', TRUE);
    $exclusions_array = array_merge($exclusions_array, $exclude_paths);
  }

  // Create the list of requested extension exclusions. (This is a little more
  // complicated.)
  $extensions = trim(variable_get('auditfiles_exclude_extensions', ''));
  if ($extensions) {
    $exclude_extensions = explode(' ', $extensions);
    array_walk($exclude_extensions, '_auditfiles_make_preg', FALSE);

    $extensions = implode('|', $exclude_extensions);
    // Add grouping around string & end marker and append to exlusions_array.
    $extensions = '(' . $extensions . ')$';
    $exclusions_array[] = $extensions;
  }

  // Implode exclusions array to a string
  $exclusions = implode('|', $exclusions_array);

  // Return prepared exclusion string
  return $exclusions;
}

/**
 * Escapes any possible regular expression characters in the specified string.
 *
 * @param string $element
 *   The string to escape.
 * @param mixed $key
 *   The key or index for the array item passed into $element.
 *
 * @param boolean $makefilepath
 *   Set to TRUE to change elements to file paths at the same time.
 */
function _auditfiles_make_preg(&$element, $key, $makefilepath = FALSE) {
  if ($makefilepath) {
    $realpath = drupal_realpath(file_build_uri($element));
    if ($realpath) {
      $element = $realpath;
    }
  }
  $element = preg_quote($element);
}
