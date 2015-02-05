<?php
/**
 * @file
 * This generates the report showing files in the database, but not referenced
 * by any content.
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
function auditfiles_used_not_referenced_form($form, &$form_state) {
  // Check to see if the confirmation form needs to be displayed instead of the
  // normal form.
  if (isset($form_state['storage']['confirm'])) {
    return _auditfiles_used_not_referenced_confirm_operation($form, $form_state);
  }

  $header = array(
    'fid' => array(
      'data' => t('File ID'),
      'field' => 'fu.fid',
      'sort' => 'asc',
    ),
    'uri' => array(
      'data' => t('File URI'),
      'field' => 'fm.module',
    ),
    'usage' => array(
      'data' => t('Usages'),
    ),
  );

  $unreferenced_files = _auditfiles_used_not_referenced_get_data();

  // Process each result in turn and build check box list
  $files = array();
  foreach ($unreferenced_files as $fid) {
    $files[$fid] = '';
    // Get the file's information from the file_managed table.
    $results = db_query("SELECT * FROM {file_managed} WHERE fid = $fid");
    foreach ($results as $file_managed) {
      if (empty($file_managed)) {
        // The file is not referenced in the file_managed table. Display an
        // error message.
        $rows[$file_managed->fid] = array(
          'fid' => t(
            'This file is not listed in the file_managed table. See the ":usednotmanaged" report.',
            array(':usednotmanaged' => l('Used not managed', 'admin/reports/auditfiles/usednotmanaged'))),
          'uri' => '',
          'usage' => '',
        );
      }
      else {
        $usage = '<ul>';
        $results2 = db_query("SELECT * FROM {file_usage} WHERE fid = $fid");
        foreach ($results2 as $file_usage) {
          // Create the usages list.
          $used_by = $file_usage->module . ' ' . t('module');
          $used_in = l($file_usage->type . '/' . $file_usage->id, $file_usage->type . '/' . $file_usage->id);
          $times_used = $file_usage->count;
          $usage .= '<li>' . t('Used by: ') . $used_by . t('; ') . t('Used in: ') . $used_in . t('; ') . t('Times used: ') . $times_used . '</li>';
        }
        $usage .= '</ul>';

        // If the file does not exist, add it to the table.
        $rows[$file_managed->fid] = array(
          'fid' => $file_managed->fid,
          'uri' => $file_managed->uri,
          'usage' => $usage,
        );
      }
    }
  }

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
    $maximum_records = variable_get('auditfiles_report_options_maximum_records', 0);
    if ($maximum_records > 0 && count($rows) >= $maximum_records) {
      $file_count_message = 'Found at least @count files in the file_usage table that are not referenced in content.';
    }
    else {
      $file_count_message = 'Found @count files in the file_usage table that are not referenced in content.';
    }
    $form_count = format_plural(
      count($rows),
      'Found 1 file in the file_usage table that is not referenced in content.',
      $file_count_message
    );
  }
  else {
    $form_count = 'Found no files in the file_usage table that are not referenced in content.';
  }
  $form['files'] = array(
    '#type' => 'tableselect',
    '#header' => $header,
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
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Delete selected items from the file_usage table'),
    );

    $form['pager'] = array('#markup' => theme('pager'));
  }

  return $form;
}

/**
 * Submit handler for the auditfiles_used_not_referenced_form form.
 */
function auditfiles_used_not_referenced_form_submit($form, &$form_state) {
  // Check to see if any files were selected.
  if (!empty($form_state['storage']['files'])) {
    foreach ($form_state['storage']['files'] as $file_id) {
      if ($file_id != 0) {
        // The operation has been confirmed, so delete the records.
        _auditfiles_used_not_referenced_delete_record($file_id);
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
 * Retrieves a list of files in {file_managed} but not in {file_usage}.
 *
 * @return array
 *   The list of files in the {file_managed} table but not in the {file_usage}
 *   table.
 */
function _auditfiles_used_not_referenced_get_data() {
  // Get a list of files in the file_usage database table.
  $maximum_records = variable_get('auditfiles_report_options_maximum_records', 0);
  $query = "SELECT DISTINCT(fid) FROM {file_usage}";
  if ($maximum_records > 0) {
    $query .= " LIMIT $maximum_records";
  }
  $files_in_file_usage = db_query($query)->fetchAll(PDO::FETCH_COLUMN);

  $files_in_fields = array();
  // Get a list of all fields on the site.
  $fields = field_info_fields();
  foreach ($fields as $field) {
    /**
     * @todo
     * Add a setting to allow the administrator the ability for spcifying the
     * field types.
     */
    if (!empty($field['type']) && ($field['type'] == 'file' || $field['type'] == 'image')) {
      // Get the database table name for the field.
      $table = key($field['storage']['details']['sql']['FIELD_LOAD_CURRENT']);
      // Get the column name in the database table for the field.
      $column = $field['storage']['details']['sql']['FIELD_LOAD_CURRENT'][$table]['fid'];

      $query = "SELECT t.$column FROM {$table} t INNER JOIN {file_usage} f ON f.fid = t.$column";
      $result = db_query($query);

      foreach ($result as $fid) {
        if (in_array($fid->{$column}, $files_in_file_usage)) {
          $files_in_fields[] = $fid->{$column};
        }
      }
    }
  }

  return array_diff($files_in_file_usage, $files_in_fields);
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
function _auditfiles_used_not_referenced_confirm_operation($form, &$form_state) {
  $values = $form_state['input'];

  $form['changelist'] = array(
    '#prefix' => '<ul>',
    '#suffix' => '</ul>',
    '#tree' => TRUE,
  );

  // Prepare the list of items to present to the user.
  if (!empty($values['files'])) {
    foreach ($values['files'] as $file_id) {
      if (!empty($file_id)) {
        $file = file_load($file_id);

        if (!empty($file)) {
          $form['changelist'][$file_id] = array(
            '#type' => 'hidden',
            '#value' => $file_id,
            '#prefix' => '<li><strong>' . $file->filename . '</strong> '
              . t('will be deleted from the file_usage table.'),
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
  // Go back to the main form, when done with this one.
  $form['destination'] = array(
    '#type' => 'hidden',
    '#value' => 'admin/reports/auditfiles/usednotreferenced'
  );

  return confirm_form(
    $form,
    t('Delete these items from the file_usage table?'),
    'admin/reports/auditfiles/usednotreferenced',
    '<strong>' . t('This action cannot be undone.') . '</strong>',
    t('Yes'),
    t('No')
  );
}

/**
 * Deletes a specified file from the database.
 *
 * @param integer $file_id
 *   The ID of the file to delete from the database.
 */
function _auditfiles_used_not_referenced_delete_record($file_id) {
  $result = db_delete('file_usage')
    ->condition('fid', $file_id)
    ->execute();
  if ($result == 0) {
    drupal_set_message(
      t(
        'There was a problem deleting one or more records from the file_usage table with the file ID of %fid. Check the logs for more information.',
        array('%fid' => $file_id)
      ),
      'warning'
    );
  }
}
