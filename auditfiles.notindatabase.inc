<?php
/**
 * @file
 * Generates the report showing files on the server, but not in the database.
 */

/**
 * Menu callback for files not in the database.
 *
 * @return array
 *   The form definition.
 */
function auditfiles_not_in_database() {
  return drupal_get_form('auditfiles_not_in_database_form');
}

/**
 * Form definition for files not in database.
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
//  if (isset($form_state['storage']['confirm'])) {
  if (!empty($form_state['input']) && $form_state['input']['operation'] != 'donothing') {
    return drupal_build_form('auditfiles_not_in_database_form_confirm', $form_state);
  }

  // Get the list of files that aren't in the database.
  $files_not_in_database = _auditfiles_get_files_not_in_database();

  // Output count of files not in the database.
  if ($files_not_in_database) {
    $form['count'] = array(
      '#markup' => format_plural(
        count($files_not_in_database),
        '1 file found on the server that is not in the database.',
        '@count files found on the server that are not in the database.'
      ),
    );
  }
  else {
    $form['count'] = array(
      '#markup' => t('No files found.'),
    );
  }

  // Define the Actions form elements.
  $form['actions'] = array(
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
  $form['actions']['operation'] = array(
    '#type' => 'select',
    '#options' => $options,
    '#default_value' => 'donothing',
  );
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Update'),
  );

  $root = drupal_realpath('./');
  // Process each result in turn and build check box list.
  $files = array();
  foreach ($files_not_in_database as $file) {
    // Strip out the real path to leave just a drupal path
    $drupal_path = preg_replace('@' . preg_quote($root) . '.@', '', $file);
    // Get the scheme's path.
    $scheme_path = variable_get('file_' . file_default_scheme() . '_path');
    // Get the path to the file relative to it's file storage location.
    $files_path = preg_replace('@' . preg_quote($scheme_path) . '.@', '', $drupal_path);
    // If the site uses private transfers, file_create_url cannot be used, as
    // the links fail. Force a public URL instead.
    $files[$drupal_path] = l($files_path, $GLOBALS['base_url'] . '/' . str_replace('\\', '/', $drupal_path));
  }

  // Add checkboxes to the list of files.
  $form['files'] = array(
    '#type' => 'checkboxes',
    '#options' => $files,
  );

  return $form;
}

/**
 * Submit handler for the auditfiles_not_in_database_form form.
 *
 * @param array $form
 *   The form definition.
 * @param array $form_state
 *   The current state of the form.
 */
/**
 * @todo
 * Figure out why Drupal isn't hitting this function.
 */
//function auditfiles_not_in_database_form_submit($form, &$form_state) {
//  if ($form_state['clicked_button']['#id'] == 'edit-submit' && $form_state['values']['operation'] != 'donothing') {
//    $form_state['storage']['confirm'] = TRUE;
//    $form_state['storage']['values'] = $form_state['values'];
//    $form_state['rebuild'] = TRUE;
//  }
//}

/**
 * Theme the auditfiles_not_in_database_form form.
 *
 * @param array $variables
 *   The variables currently available for use in a template file.
 *
 * @return string
 *   The formatted display output.
 */
function theme_auditfiles_not_in_database_form($variables) {
  $form = $variables['form'];
  // Render the file count.
  $output = drupal_render($form['count']);

  // If any files are found, render them.
  if (isset($form['files']) && is_array($form['files'])) {
    // Render the actions.
    $output .= drupal_render($form['actions']);

    // Construct a table of files, starting with the header.
    $header = array(
      t('Select'),
      t('File'),
    );

    // Construct the rows of the table.
    foreach (element_children($form['files']) as $key) {
      $row = array();
      // Get the check box's title for display in the second column.
      $title = $form['files'][$key]['#title'];
      // Remove it so it isn't also displayed in the first column.
      unset($form['files'][$key]['#title']);
      $row[] = drupal_render($form['files'][$key]);
      $row[] = $title;
      $rows[] = array('data' => $row);
    }

    // Render the table.
    $output .= theme(
      'table',
      array(
        'header' => $header,
        'rows' => $rows,
      )
    );
  }

  return $output;
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
function auditfiles_not_in_database_form_confirm($form, &$form_state) {
  $values = $form_state['input'];
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

  if (!empty($values['files'])) {
    foreach ($values['files'] as $file => $selected) {
      if ($seltype == 'all' || $selected) {
        if ($optype == 'delete') {
          $message = "<strong>$file</strong> will be deleted from the server.";
        }
        else {
          $message = "<strong>$file</strong> will be added to the database.";
        }
        $form['changelist'][$file] = array(
          '#type' => 'hidden',
          '#value' => $file,
          '#prefix' => '<li>',
          '#suffix' => $message . "</li>\n",
        );
      }
      else {
        // Unsetting the unprocessed files prevents confirm_submit from dealing
        // with them.
        unset($form_state['input']['files'][$file]);
      }
    }
  }

  $form['operation'] = array(
    '#type' => 'hidden',
    '#value' => $operation,
  );
  $form['#submit'][] = 'auditfiles_not_in_database_form_confirm_submit';

  if (empty($form_state['input']['files'])) {
    return confirm_form(
      $form,
      t('No files have been selected to perform the action on.'),
      'admin/reports/auditfiles/notindatabase',
      t('No files have been selected to perform the action on.'),
      '',
      t('Cancel')
    );
  }
  else {
    return confirm_form(
      $form,
      t('Are you sure you want to make these changes?'),
      'admin/reports/auditfiles/notindatabase',
      '<strong>' . t('This action cannot be undone.') . '</strong>',
      t('Process all'),
      t('Cancel')
    );
  }
}

/**
 * Submit handler for the confirmation form.
 *
 * @param array $form
 *   The form definition.
 * @param array $form_state
 *   The current state of the form.
 */
function auditfiles_not_in_database_form_confirm_submit($form, &$form_state) {
  if ($form_state['input']['confirm']) {
    $values = $form_state['input'];
    $operation = $values['operation'];
    if ($operation != 'delete') {
      global $user;
      if (function_exists('finfo_file')) {
        $finfo = @finfo_open(FILEINFO_MIME);
      }
    }
    foreach ($values['changelist'] as $filename => $selected) {
      if ($operation == 'delete') {
        $root = drupal_realpath('./');
        if (file_unmanaged_delete($root . '/' . $filename)) {
          drupal_set_message(t('Sucessfully deleted %file from the server.', array('%file' => $filename)));
        }
        else {
          drupal_set_message(t('Failed to delete %file from the server.', array('%file' => $filename)));
        }
      }
      else {
        $file = new stdClass();
        $file->uid = $user->uid;
        // Get the scheme's path.
        $scheme_path = variable_get('file_' . file_default_scheme() . '_path');
        // Get the path to the file relative to it's file storage location.
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
    }
    if ($operation == 'delete') {
      drupal_set_message(t('The action of deleting files from the server is complete.'));
    }
    else {
      drupal_set_message(t('The action of adding files to the database is complete.'));
    }
    // Clear so our return to the primary form doesn't think we're going to the
    // confirmation form.
    unset($form_state['input']['confirm']);
  }
}

/**
 * Retrieve sorted list of files on server but not in database.
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
  // Sort the rows to make it easier to compare to file listing in FTP
  asort($files_on_server);
  // Process each result in turn
  foreach ($files_on_server as $file) {
    // Check it isn't a directory - not interested
    if (!file_prepare_directory($file)) {
      // Exclude files, paths and extensions according to the retrieved
      // exclusions string.
      if (!preg_match('@' . $exclusions . '@', $file) || !$exclusions) {
        // Check to see if file is NOT in the database
        if (!in_array($file, $files_in_database)) {
          // If we get here, we have a file that isn't in the database.
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
    while (false !== ($file = readdir($handle))) {
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
