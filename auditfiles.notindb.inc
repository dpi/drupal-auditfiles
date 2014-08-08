<?php
/**
 * @file
 * Callback and functions to generate files not in database report
 */

/**
 * Menu callback for files not in the database.
 *
 * @return array
 *   The form definition.
 */
function auditfiles_not_in_db() {
  return drupal_get_form('auditfiles_not_in_db_form');
}

/**
 * Form definition for files not in database
 *
 * @param array $form
 *   The form definition.
 * @param array $form_state
 *   The current state of the form.
 *
 * @return array
 *   The form definition.
 */
function auditfiles_not_in_db_form($form, &$form_state) {
  if (isset($form_state['storage']['confirm'])) {
    return drupal_build_form('auditfiles_not_in_db_form_confirm', $form_state);
  }
  // Get the list of files that aren't in the database
  $files_not_in_db = _auditfiles_get_files_not_in_db();

  // Output count of files not in the database
  if ($files_not_in_db) {
    $form['count'] = array(
      '#markup' => format_plural(count($files_not_in_db), '1 file found.', '@count files found.'),
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
  foreach ($files_not_in_db as $file) {
    // If the site uses private transfers, file_create_url cannot be used, as
    // the links fail. Force a public URL instead.
    $files[$file] = l($file, $GLOBALS['base_url'] . '/' . str_replace('\\', '/', $file));
  }

  // Add checkboxes to the list of files.
  $form['files'] = array(
    '#type' => 'checkboxes',
    '#options' => $files,
  );

  return $form;
}

/**
 * Submit handler for the auditfiles_not_in_db_form form.
 *
 * @param array $form
 *   The form definition.
 * @param array $form_state
 *   The current state of the form.
 */
function auditfiles_not_in_db_form_submit($form, &$form_state) {
  if ($form_state['clicked_button']['#id'] == 'edit-submit' &&
    $form_state['values']['operation'] != 'donothing') {
    $form_state['storage']['confirm'] = TRUE;
    $form_state['storage']['values'] = $form_state['values'];
    $form_state['rebuild'] = TRUE;
  }
}

/**
 * Theme auditfiles_not_in_db_form
 *
 * @param array $variables
 *   The variables currently available for use in a template file.
 *
 * @return string
 *   The formatted display output.
 */
function theme_auditfiles_not_in_db_form($variables) {
  $form = $variables['form'];
  // Render the file count.
  $output = drupal_render($form['count']);

  // If there are files found...
  if (isset($form['file']) && is_array($form['file'])) {
    // Render the actions.
    $output .= drupal_render($form['options']);

    // Construct a table of files.
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

    // Render the table.
    $output .= theme(
      'table',
      array(
        'header' => $header,
        'rows' => $rows,
      )
    );
  }

  $output .= drupal_render_children($form);

  return $output;
}

/**
 *
 *
 * @param array $form
 *   The form definition.
 * @param array $form_state
 *   The current state of the form.
 *
 * @return array
 *   A form array for a confirmation form.
 */
function auditfiles_not_in_db_form_confirm($form, &$form_state) {
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
      // Unsetting the unprocessed files prevents confirm_submit from dealing
      // with them.
      unset($form_state['storage']['values']['files'][$file]);
    }
  }

  $form['operation'] = array(
    '#type' => 'hidden',
    '#value' => $operation,
  );
  $form['#submit'][] = 'auditfiles_not_in_db_form_confirm_submit';

  return confirm_form(
      $form, t('Are you sure you want to make these changes?'), 'admin/reports/auditfiles/notindb', '<strong>' . t('This action cannot be undone.') . '</strong>', t('Process all'), t('Cancel')
  );
}

/**
 * Submit handler for the confirmation form.
 *
 * @param array $form
 *   The form definition.
 * @param array $form_state
 *   The current state of the form.
 */
function auditfiles_not_in_db_form_confirm_submit($form, &$form_state) {
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
    // Clear so our return to the primary form doesn't think we're going to the
    // confirmation form.
    unset($form_state['storage']['confirm']);
  }
}

/**
 * Retrieve sorted list of files on server but not in database.
 *
 * @return array
 *   An array of file names not found in the database.
 */
function _auditfiles_get_files_not_in_db() {
  // Initialize variables.
  $files_in_db = array();
  $files_not_in_db = array();
  // Get the root path.
  $root = drupal_realpath('./');

  // Get all the files from the {file_managed} table and store them as qualified
  // path.
  $result = db_query('SELECT uri FROM {file_managed} ORDER BY uri ASC');
  foreach ($result as $file) {
    $path = drupal_realpath($file->uri);
    // Strip out the real path to leave just a drupal path
    $files_in_db[] = preg_replace('@' . preg_quote($root) . '.@', '', $path);
  }

  // Get the exclusions string.
  $exclusions = _auditfiles_get_exclusions();

  // Get all the files out of the directory structure
  $files_on_server = _auditfiles_get_file_list(drupal_realpath(file_default_scheme() . '://'), TRUE);
  // Sort the rows to make it easier to compare to file listing in FTP
  asort($files_on_server);
  // Process each result in turn
  foreach ($files_on_server as $file) {
    // Strip out the real path to leave just a drupal path
    $file = preg_replace('@' . preg_quote($root) . '.@', '', $file);

    // Check it isn't a directory - not interested
    if (!file_prepare_directory($file)) {
      // Exclude files, paths and extensions according to the retrieved
      // exclusions string.
      if (!preg_match('/' . $exclusions . '/', $file) || !$exclusions) {
        // Check to see if file is NOT in the database
        if (!in_array($file, $files_in_db)) {
          // If we get here, we have a file that isn't in the database.
          $file = preg_replace('/^' . preg_quote(file_default_scheme()) . '/', '', $file);
          $files_not_in_db[] = $file;
        }
      }
    }
  }

  return $files_not_in_db;
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
    $realpath = drupal_realpath($element);
    if ($realpath) {
      $element = $realpath;
    }
  }
  $element = preg_quote($element);
}
