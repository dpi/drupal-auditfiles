<?php
/**
 * @file
 * This generates a report showing all the files on the server that are not in
 * the database.
 */

// Functions for displaying the list of files on the page.

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

  $record_limiters = _auditfile_not_in_database_get_record_limiters();
  $record_selection = $record_limiters['record_selection'];
  $maximum_records = $record_limiters['maximum_records'];

  // Get the records to display.
  // Check to see if the data has been stored.
  if (!empty($form_state['storage']['saved_rows'])) {
    // The data is currently saved, so use that.
    $rows = unserialize($form_state['storage']['saved_rows']);
  }
  else {
    // Check to see if the batch operation was just run, and if so, use that data.
    $rows = variable_get('auditfiles_not_in_database_files_to_display', array());
    if (empty($rows)) {
      // The data is not saved and the batch operation has not been run, so get
      // the data using the default options.
      $file_ids = _auditfiles_not_in_database_get_file_list(drupal_realpath(file_default_scheme() . '://'), TRUE);
      if (!empty($file_ids)) {
        foreach ($file_ids as $file_ids) {
          $rows[] = _auditfiles_not_in_database_get_file_data($file_ids);
        }
      }
    }
  }
  // Save the data for later retrieval.
  $form['saved_rows'] = array(
    '#type' => 'hidden',
    '#value' => serialize($rows),
  );

  // Set up the pager.
  if (!empty($rows)) {
    $items_per_page = variable_get('auditfiles_report_options_items_per_page', 50);
    if (!empty($items_per_page)) {
      $current_page = pager_default_initialize(count($rows), $items_per_page);
      // Break the total data set into page sized chunks.
      $pages = array_chunk($rows, $items_per_page, TRUE);
    }
  }

  // Define the form.
  // Setup the record count and related messages.
  if (!empty($rows)) {
    if ($maximum_records > 0 && count($rows) == $maximum_records) {
      $file_count_message = 'Found at least @count files on the server that are not in the database.';
    }
    else {
      $file_count_message = 'Found @count files on the server that are not in the database.';
    }
    $form_count = format_plural(
      count($rows),
      'Found 1 file on the server that is not in the database.',
      $file_count_message
    );
  }
  else {
    $form_count = 'Found no files on the server that are not in the database.';
  }

  // Add the button to batch process the list of results.
  if (!empty($rows)) {
    if ($record_selection == 'limited') {
      $form['batch_process'] = array(
        '#type' => 'submit',
        '#value' => t('Load all records'),
      );
    }
    elseif ($record_selection == 'batch_sets') {
      // Define the previous batch set button.
      $batch_offset = variable_get('auditfiles_not_in_database_batch_offset', 0);
      if ($batch_offset > 0) {
        $form['batch_process_prev'] = array(
          '#type' => 'submit',
          '#value' => t('Load previous batch set'),
        );
      }

      // Define the next batch set button.
      $batch_size = variable_get('auditfiles_report_options_batch_size', 0);
      if (count($rows) == $batch_size) {
        $form['batch_process_next'] = array(
          '#type' => 'submit',
          '#value' => t('Load next batch set'),
        );
      }
    }
  }
  // Add the button to reset the record selection.
  if ($record_selection != 'normal') {
    $form['reset_records'] = array(
      '#type' => 'submit',
      '#value' => t('Reset record selection'),
    );
  }

  $form['files'] = array(
    '#type' => 'tableselect',
    '#header' => _auditfile_not_in_database_get_header(),
    '#empty' => t('No items found.'),
    '#prefix' => '<div><em>' . $form_count . '</em></div>',
  );

  if (!empty($rows) && !empty($pages)) {
    $form['files']['#options'] = $pages[$current_page];
  }
  elseif (!empty($rows)) {
    $form['files']['#options'] = $rows;
  }
  else {
    $form['files']['#options'] = array();
  }

  if (!empty($rows)) {
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

    $form['pager'] = array('#markup' => theme('pager'));
  }

  return $form;
}

/**
 * Submit handler for the auditfiles_not_in_database_form form.
 */
function auditfiles_not_in_database_form_submit($form, &$form_state) {
  // Check if an operation was performed.
  if (!empty($form_state['values']['op'])) {
    // Clear the variable, so subsequent pages will load the correct data.
    variable_del('auditfiles_not_in_database_files_to_display');
    // Check which operation was performed and start the batch process.
    if ($form_state['values']['op'] == 'Load all records') {
      $batch_size = variable_get('auditfiles_report_options_batch_size', 0);
      if ($batch_size > 0) {
        // Set appropriate variables for this operation.
        variable_set('auditfiles_not_in_database_record_selection', 'batch_sets');
      }
      else {
        // Set appropriate variables for this operation.
        variable_set('auditfiles_not_in_database_record_selection', 'batched');
      }
      // Prepare and set the batch.
      batch_set(_auditfiles_not_in_database_batch_display_create_batch());
    }
    elseif ($form_state['values']['op'] == 'Load previous batch set') {
      // Set appropriate variables for this operation.
      $batch_size = variable_get('auditfiles_report_options_batch_size', 0);
      $batch_offset = variable_get('auditfiles_not_in_database_batch_offset', 0);
      variable_set('auditfiles_not_in_database_batch_offset', $batch_offset - $batch_size);
      variable_set('auditfiles_not_in_database_record_selection', 'batch_sets');
      // Prepare and set the batch.
      batch_set(_auditfiles_not_in_database_batch_display_create_batch());
    }
    elseif ($form_state['values']['op'] == 'Load next batch set') {
      // Set appropriate variables for this operation.
      $batch_size = variable_get('auditfiles_report_options_batch_size', 0);
      $batch_offset = variable_get('auditfiles_not_in_database_batch_offset', 0);
      variable_set('auditfiles_not_in_database_batch_offset', $batch_offset + $batch_size);
      variable_set('auditfiles_not_in_database_record_selection', 'batch_sets');
      // Prepare and set the batch.
      batch_set(_auditfiles_not_in_database_batch_display_create_batch());
    }
    elseif ($form_state['values']['op'] == 'Reset record selection') {
      // Set appropriate variables for this operation.
      unset($form_state['values']['saved_rows']);
      variable_set('auditfiles_not_in_database_record_selection', 'normal');
      variable_del('auditfiles_not_in_database_batch_offset');
      variable_del('auditfiles_not_in_database_files_to_display');
    }
    elseif ($form_state['values']['op'] == 'Add selected items to the database' && !empty($form_state['values']['files'])) {
      // Set appropriate variables for this operation.
      variable_set('auditfiles_not_in_database_record_selection', 'normal');
      // Prepare and set the batch.
      batch_set(_auditfiles_not_in_database_batch_add_create_batch($form_state['values']['files']));
    }
    elseif ($form_state['values']['op'] == 'Delete selected items from the server' && !empty($form_state['values']['files'])) {
      // Set appropriate variables for this operation.
      variable_set('auditfiles_not_in_database_record_selection', 'normal');
      // Prepare and set the batch.
      batch_set(_auditfiles_not_in_database_batch_delete_create_batch($form_state['values']['files']));
    }
  }
  // Check to see if any files were selected for operating on.
  elseif (!empty($form_state['values']['files']) && !isset($form_state['values']['process'])) {
    foreach ($form_state['values']['files'] as $file_id) {
      if (!empty($file_id)) {
        // At least one file was selected, and the operation has not been
        // confirmed, so modify the data to display the confirmation form.
        $form_state['storage']['files'] = $form_state['values']['files'];
        $form_state['storage']['op'] = $form_state['values']['op'];
        $form_state['storage']['confirm'] = TRUE;
        $form_state['rebuild'] = TRUE;
        return TRUE;
      }
    }

    drupal_set_message(t('No items were selected to operate on.'));
  }

  if (!empty($form_state['values']['saved_rows'])) {
    $form_state['storage']['saved_rows'] = $form_state['values']['saved_rows'];
  }
}

// Functions that are common for all batches in this file.

/**
 * Adds vaules to a batch definition that are common to all batches in this file.
 *
 * @return array
 *   The beginning of the batch definition.
 */
function _auditfiles_not_in_database_batch_set_common_values() {
  return array(
    'error_message' => t('One or more errors were encountered processing the files.'),
    'file' => drupal_get_path('module', 'auditfiles') . '/auditfiles.notindatabase.inc',
    'finished' => '_auditfiles_not_in_database_batch_finish_batch',
    'progress_message' => t('Completed @current of @total operations.'),
  );
}

/**
 * The function that is called when the batch is completed.
 */
function _auditfiles_not_in_database_batch_finish_batch($success, $results, $operations) {
  if ($success) {
    if (!empty($results['files_to_display'])) {
      // Save the gathered data for display.
      variable_set('auditfiles_not_in_database_files_to_display', $results['files_to_display']);
    }
  }
  else {
    // An error occurred.
    // $operations contains the operations that remained unprocessed.
    $error_operation = reset($operations);
    drupal_set_message(
      t('An error occurred while processing @operation with arguments : @args',
        array(
          '@operation' => $error_operation[0],
          '@args' => print_r($error_operation[0], TRUE),
        )
      )
    );
  }
}

// Functions for preparing the batch for displaying the list of files.

/**
 * Prepares the definition for the page display batch.
 *
 * @return array
 *   The batch definition.
 */
function _auditfiles_not_in_database_batch_display_create_batch() {
  $batch = _auditfiles_not_in_database_batch_set_common_values();
  $batch['title'] = t('Loading file audit data');
  $batch['operations'] = _auditfiles_not_in_database_batch_display_get_operations();
  return $batch;
}

/**
 * Configures the operations for the batch process.
 *
 * @return array
 *   The operations to execute.
 */
function _auditfiles_not_in_database_batch_display_get_operations() {
  $operations = array();
  $file_ids = _auditfiles_not_in_database_get_file_list(drupal_realpath(file_default_scheme() . '://'), TRUE);
  foreach ($file_ids as $file_id) {
    $operations[] = array(
      '_auditfiles_not_in_database_batch_display_process_batch',
      array($file_id, count($file_ids)),
    );
  }
  return $operations;
}

/**
 * The batch process for displaying the files.
 *
 * @param string $filename
 *   The full pathname to the file to add to the database.
 */
function _auditfiles_not_in_database_batch_display_process_batch($file_id, $count, &$context) {
  // Store information needed to track progress between successive calls.
  if (empty($context['sandbox'])) {
    $context['sandbox'] = array();
    $context['sandbox']['progress'] = 0;
    $context['sandbox']['current_file'] = '';
    $context['sandbox']['max'] = $count;
  }

  // Process the current file.
  $file = _auditfiles_not_in_database_get_file_data($file_id);

  // The contents of 'results' are available as $results in the 'finished'
  // function.
  $context['results'][] = $file;
  // Update our progress information.
  $context['sandbox']['progress']++;
  $context['sandbox']['current_node'] = $file_id;
  $context['message'] = t('Processed ') . check_plain($file['filename']);

  // Inform the batch engine that we are not finished and provide an estimation
  // of the completion level we reached.
  if ($context['sandbox']['progress'] != $context['sandbox']['max']) {
    $context['finished'] = ($context['sandbox']['progress'] >= $context['sandbox']['max']);
  }
}

// Functions for the add and delete operations.

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

  // Prepare the list of items to present to the user.
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
          '#prefix' => '<li><strong>' . $filename . '</strong> ' . $message,
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
 * Creates the batch for adding files to the database.
 *
 * @param array $files
 *   The list of files returned from the form submit.
 *
 * @return array
 *   The definition of the batch.
 */
function _auditfiles_not_in_database_batch_add_create_batch($files) {
  $batch = _auditfiles_not_in_database_batch_set_common_values();
  $batch['title'] = t('Adding files to Drupal file management');
  $operations = array();
  foreach ($files as $file) {
    $operations[] = array(
      '_auditfiles_not_in_database_batch_add_process_batch',
      array($file),
      count($files),
    );
  }
  $batch['operations'] = $operations;
  return $batch;
}

/**
 * The batch process for adding the file.
 *
 * @param string $filename
 *   The full pathname to the file to add to the database.
 */
function _auditfiles_not_in_database_batch_add_process_batch($file, $count, &$context) {
  // Store information needed to track progress between successive calls.
  if (empty($context['sandbox'])) {
    $context['sandbox'] = array();
    $context['sandbox']['progress'] = 0;
    $context['sandbox']['current_file'] = '';
    $context['sandbox']['max'] = $count;
  }

  // Process the current file.
//  _auditfiles_not_in_database_batch_add_process_file($file);

  // The contents of 'results' are available as $results in the 'finished'
  // function.
  $context['results'][] = check_plain($file);
  // Update our progress information.
  $context['sandbox']['progress']++;
  $context['sandbox']['current_node'] = $file;
  $context['message'] = t('Processed ') . check_plain($file);

  // Inform the batch engine that we are not finished and provide an estimation
  // of the completion level we reached.
  if ($context['sandbox']['progress'] != $context['sandbox']['max']) {
    $context['finished'] = ($context['sandbox']['progress'] >= $context['sandbox']['max']);
  }
}

/**
 * Adds the specified file to the database.
 *
 * @param string $filename
 *   The full pathname to the file to add to the database.
 */
function _auditfiles_not_in_database_batch_add_process_file($filename) {
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
 * Creates the batch for deleting files from the server.
 *
 * @param array $files
 *   The list of files returned from the form submit.
 *
 * @return array
 *   The definition of the batch.
 */
function _auditfiles_not_in_database_batch_delete_create_batch($files) {
  $batch = _auditfiles_not_in_database_batch_set_common_values();
  $batch['title'] = t('Deleting files from the server');
  $operations = array();
  foreach ($files as $file) {
    $operations[] = array(
      '_auditfiles_not_in_database_batch_delete_process_batch',
      array($file),
      count($files),
    );
  }
  $batch['operations'] = $operations;
  return $batch;
}

/**
 * The batch process for deleting the file.
 *
 * @param string $filename
 *   The full pathname to the file to add to the database.
 */
function _auditfiles_not_in_database_batch_delete_process_batch($file, $count, &$context) {
  // Store information needed to track progress between successive calls.
  if (empty($context['sandbox'])) {
    $context['sandbox'] = array();
    $context['sandbox']['progress'] = 0;
    $context['sandbox']['current_file'] = '';
    $context['sandbox']['max'] = $count;
  }

  // Process the current file.
//  _auditfiles_not_in_database_batch_delete_process_file($file);

  // The contents of 'results' are available as $results in the 'finished'
  // function.
  $context['results'][] = check_plain($file);
  // Update our progress information.
  $context['sandbox']['progress']++;
  $context['sandbox']['current_node'] = $file;
  $context['message'] = t('Processed ') . check_plain($file);

  // Inform the batch engine that we are not finished and provide an estimation
  // of the completion level we reached.
  if ($context['sandbox']['progress'] != $context['sandbox']['max']) {
    $context['finished'] = ($context['sandbox']['progress'] >= $context['sandbox']['max']);
  }
}

/**
 * Deletes the specified file from the server.
 *
 * @param string $filename
 *   The full pathname of the file to delete from the server.
 */
function _auditfiles_not_in_database_batch_delete_process_file($filename) {
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

// Functions for retrieving and processing the files and file data.

/**
 * Recurse directories and add files to an array.
 *
 * @param string $directory
 *   The directory in which to look for files.
 * @param boolean $recursive
 *   Whether or not to recurse into subdirectories.
 *
 * @return array
 *   The list of file names to display.
 */
function _auditfiles_not_in_database_get_file_list($directory, $recursive) {
  $array_items = array();
  $exclusions = _auditfiles_get_exclusions();
  // Set up for any record limition.
  $record_limiters = _auditfile_not_in_database_get_record_limiters();
  $maximum_records = $record_limiters['maximum_records'];
  $batch_offset = 0;
  if ($maximum_records > 0) {
    $batch_offset = variable_get('auditfiles_not_in_database_batch_offset', 0);
  }
  static $file_count = 0;

  if ($handle = opendir($directory)) {
    while (($file = readdir($handle)) !== FALSE) {
      // Exclude files, paths and extensions according to the admin specified
      // exclusions list.
      if (!$exclusions || !preg_match('@' . $exclusions . '@', $file)) {
        if ($file != "." && $file != "..") {
          if (is_dir($directory . "/" . $file)) {
            if ($recursive) {
              $array_items = array_merge($array_items, _auditfiles_not_in_database_get_file_list($directory . "/" . $file, $recursive));
            }
          }
          else {
            // Exclude files, paths and extensions according to the admin specified
            // exclusions list.
            if (!preg_match('@' . $exclusions . '@', $file) || !$exclusions) {
              // Convert the path name to a Drupal URI scheme for comparison in
              // the database.
              $files_path = '';
              // Get the path of the currently used file scheme.
              $default_file_scheme = file_default_scheme();
              if ($default_file_scheme == 'public') {
                $files_path = variable_get('file_public_path', '');
              }
              elseif ($default_file_scheme == 'private') {
                $files_path = variable_get('file_private_path', '');
              }
              else {
                /**
                 * @todo
                 *   Unknown file scheme.
                 */
              }

              // Remove the path to the files directory.
              if ($files_path != '') {
                $matches = array();
                preg_match_all('~(.*)(' . $files_path . ')(.*)~', $file, $matches);
                $match = end($matches);
                $match = reset($match);
              }

              // See if the file is in the database.
              $stored_file = db_query("SELECT fid FROM {file_managed} WHERE uri = ':uri'", array(':uri' => file_build_uri($match)))->fetchField();
              if (empty($stored_file)) {
                // This file is not in the database.
                // Check to see if it is within the limitations specified.
                if ($batch_offset > 0) {
                  if ($file_count >= $batch_offset && $file_count < ($batch_offset + $maximum_records)) {
                    // It is within the specified limitations. Add the file to
                    // the array.
                    $file = $directory . "/" . $file;
                    $array_items[] = preg_replace("/\/\//si", "/", $file);
                  }
                }
                elseif ($maximum_records > 0) {
                  if ($file_count < $maximum_records) {
                    // It is within the specified limitations. Add the file to
                    // the array.
                    $file = $directory . "/" . $file;
                    $array_items[] = preg_replace("/\/\//si", "/", $file);
                  }
                }
                else {
                    // There are no specified limitations. Add the file to the
                    // array.
                  $file = $directory . "/" . $file;
                  $array_items[] = preg_replace("/\/\//si", "/", $file);
                }

                $file_count++;
              }
            }
          }
        }
      }
    }

    closedir($handle);
  }

  return $array_items;
}

/**
 * Retrieves file data from the database and checks if it is on the server.
 *
 * @param integer $file
 *   The ID of the file to check and prepare for display.
 * @param array $context
 *   Information about the batch, its operations and its status.
 */
function _auditfiles_not_in_database_get_file_data($file) {
  $exclusions = _auditfiles_get_exclusions();
  static $date_format = '';
  $date_format = variable_get('auditfiles_report_options_date_format', 'long');
  // Exclude files, paths and extensions according to the admin specified
  // exclusions list.
  if (!preg_match('@' . $exclusions . '@', $file) || !$exclusions) {
    // Get the filename from the filepathname.
    $filename = substr($file, strrpos($file, '/') + 1);
    // Return the file
    return array(
      'filepathname' => $file,
      'filemime' => file_get_mimetype($file),
      'filesize' => number_format(filesize($file)),
      'filemodtime' => format_date(filemtime($file), $date_format),
      'filename' => $filename,
    );
  }
}

// Helper functions.

/**
 * Returns report limiting settings.
 *
 * Returns the information that is needed to modify the report and display based
 * on any report limiting options that may have been set on the administrative
 * configuration settings page.
 *
 * @return array
 *   An associative array with these two values:
 *   - record_selection: A string representing record selection type, on which
 *     maximum_records is based.
 *   - maximum_records: An integer representing the total number of records to
 *     display on a report. (A value of 0 means there is no limit.)
 */
function _auditfile_not_in_database_get_record_limiters() {
  $record_selection = variable_get('auditfiles_not_in_database_record_selection', 'normal');
  $maximum_records = 0;
  if ($record_selection == 'batch_sets') {
    $maximum_records = variable_get('auditfiles_report_options_batch_size', 0);
  }
  elseif ($record_selection != 'batched') {
    $maximum_records = variable_get('auditfiles_report_options_maximum_records', 0);
    if ($maximum_records > 0) {
      $record_selection = 'limited';
      variable_set('auditfiles_not_in_database_record_selection', 'limited');
    }
  }
  return array(
    'record_selection' => $record_selection,
    'maximum_records' => $maximum_records,
  );
}

/**
 * Returns the header to use for the display table.
 *
 * @return array
 *   The header to use.
 */
function _auditfile_not_in_database_get_header() {
  return array(
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
    $exclude_files = explode(';', $files);
    array_walk($exclude_files, '_auditfiles_make_preg', FALSE);
    $exclusions_array = array_merge($exclusions_array, $exclude_files);
  }

  // Create the list of requested path exclusions.
  $paths = trim(variable_get('auditfiles_exclude_paths', 'color css ctools js'));
  if ($paths) {
    $exclude_paths = explode(';', $paths);
    array_walk($exclude_paths, '_auditfiles_make_preg', TRUE);
    $exclusions_array = array_merge($exclusions_array, $exclude_paths);
  }

  // Create the list of requested extension exclusions. (This is a little more
  // complicated.)
  $extensions = trim(variable_get('auditfiles_exclude_extensions', ''));
  if ($extensions) {
    $exclude_extensions = explode(';', $extensions);
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
