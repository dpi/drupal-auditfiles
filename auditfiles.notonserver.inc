<?php
/**
 * @file
 * This generates the report showing the files in the database, but not on the
 * server.
 */

/**
 * Displays the files found in the database, but not on the server.
 *
 * @param array $form
 *   The form definition.
 * @param array $form_state
 *   The current state of the form.
 *
 * @return array
 *   The form definition.
 */
function auditfiles_not_on_server_form($form, &$form_state) {
  // Check to see if the confirmation form needs to be displayed instead of the
  // normal form.
  if (isset($form_state['storage']['confirm'])) {
    return auditfiles_not_on_server_confirm_delete($form, $form_state);
  }

  // Set up the table header.
  $header = array(
    'fid' => array(
      'data' => t('File ID'),
      'field' => 'fm.fid',
      'sort' => 'asc',
    ),
    'uid' => array(
      'data' => t('User ID'),
      'field' => 'fm.uid',
    ),
    'filename' => array(
      'data' => t('Name'),
      'field' => 'fm.filename',
    ),
    'uri' => array(
      'data' => t('URI'),
      'field' => 'fm.uri',
    ),
    'path' => array(
      'data' => t('Path'),
      'field' => 'fm.uri',
    ),
    'filemime' => array(
      'data' => t('MIME'),
      'field' => 'fm.filemime',
    ),
    'filesize' => array(
      'data' => t('Size'),
      'field' => 'fm.filesize',
    ),
//    'status' => array(
//      'data' => t('Status'),
//      'field' => 'fm.status',
//    ),
    'timestamp' => array(
      'data' => t('Timestamp'),
      'field' => 'fm.timestamp',
    ),
    'datetime' => array(
      'data' => t('Date/time'),
      'field' => 'fm.timestamp',
    ),
  );

  // Get the data.
//  $items_per_page = variable_get('auditfiles_reports_items_per_page', 50);

  // Get all the files listed in the file_managed database table.
  $query = db_select('file_managed', 'fm');
  $query->fields('fm');
  $query->condition('status', 1);
  $query->extend('TableSort')->orderByHeader($header);
//  $query = $query->extend('PagerDefault')->limit($items_per_page);
  $results = $query->execute();

  // Get the admin specified date format for the file modification time.
  $date_format = variable_get('auditfiles_report_options_date_format', 'long');

  // Set up the table rows.
  $rows = array();
  foreach ($results as $file) {
    // Get the path for the file.
    $target = drupal_realpath($file->uri);
    if (!file_exists($target)) {
      // If the file does not exist, add it to the table.
      $rows[$file->fid] = array(
        'fid' => $file->fid,
        'uid' => $file->uid,
        'filename' => $file->filename,
        'uri' => $file->uri,
        'path' => $target,
        'filemime' => $file->filemime,
        'filesize' => $file->filesize,
//        'status' => $file->status,
        'timestamp' => $file->timestamp,
        'datetime' => format_date($file->timestamp, $date_format),
      );
    }
  }

  // Define the form table.
  $form_count = format_plural(
    count($rows),
    'Found 1 file in the database that is not on the server.',
    'Found @count files in the database that are not on the server.'
  );
  $form['files'] = array(
    '#type' => 'tableselect',
    '#header' => $header,
    '#options' => $rows,
    '#empty' => t('No unmatched files found.'),
    '#prefix' => '<div><em>' . $form_count . '</em></div>',
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Delete selected items from the database'),
  );
//  $form['pager'] = array('#markup' => theme('pager'));

  return $form;
}

/**
 * Submit handler for the auditfiles_not_on_server_form form.
 */
function auditfiles_not_on_server_form_submit($form, &$form_state) {
  // Check to see if any files were selected.
  if (!empty($form_state['storage']['files'])) {
    foreach ($form_state['storage']['files'] as $file_id) {
      if ($file_id != 0) {
        // The operation has been confirmed, so delete the database records.
        $file = file_load($file_id);
        $result = file_delete($file, TRUE);
        if ($result != TRUE) {
          drupal_set_message(
            t(
              'File %filename with ID of %fid was not deleted. Check the logs for more information.',
              array(
                '%filename' => $file->filename,
                '%fid' => $file_id,
              )
            ),
            'warning'
          );
        }
      }
    }
  }
  elseif (!empty($form_state['values']['files'])) {
    foreach ($form_state['values']['files'] as $file_id) {
      if ($file_id != 0) {
        // At least one file was selected. Check to see if the confirmation form
        // has been processed.
        if (!isset($form_state['values']['process'])) {
          // The operation has not been confirmed, so modify the data to display
          // the confirmation form.
          $form_state['storage']['files'] = $form_state['values']['files'];
          $form_state['storage']['confirm'] = TRUE;
          $form_state['rebuild'] = TRUE;
          return TRUE;
        }
      }
    }

    drupal_set_message(t('No items were selected to delete.'));
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
function auditfiles_not_on_server_confirm_delete($form, &$form_state) {
  $values = $form_state['input'];

  $form['changelist'] = array(
    '#prefix' => '<ul>',
    '#suffix' => '</ul>',
    '#tree' => TRUE,
  );

  // Prepare the list of tiems to present to the user.
  if (!empty($values['files'])) {
    foreach ($values['files'] as $file_id) {
      if (!empty($file_id)) {
        $file = file_load($file_id);
        if (!empty($file)) {
          $message = "<strong>$file->filename</strong> and all usages will be deleted from the database.";
          $form['changelist'][$file_id] = array(
            '#type' => 'hidden',
            '#value' => $file_id,
            '#prefix' => '<li>' . $message,
            '#suffix' => "</li>\n",
          );
        }
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
    '#value' => 'delete',
  );

  // Tell the submit handler to process the confirmation.
  $form['process'] = array(
    '#type' => 'hidden',
    '#value' => 'TRUE'
  );
  // Make sure the form redirects in the end
  $form['destination'] = array(
    '#type' => 'hidden',
    '#value' => 'admin/reports/auditfiles/notonserver'
  );

  return confirm_form(
    $form,
    t('Remove these items from the database?'),
    'admin/reports/auditfiles/notonserver',
    '<strong>' . t('This action cannot be undone.') . '</strong>',
    t('Yes'),
    t('No')
  );
}
