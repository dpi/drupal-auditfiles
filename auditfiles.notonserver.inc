<?php
/**
 * @file
 * Generates the report showing files in the database, but not on the server.
 */

/**
 * Menu callback for files not in the database.
 *
 * @return array
 *   The form definition.
 */
function auditfiles_not_on_server() {
  return drupal_get_form('auditfiles_not_on_server_form');
}

/**
 * Menu callback: audit files not on the server.
 */
function auditfiles_not_on_server_form($form, &$form_state) {
//  if (isset($form_state['storage']['confirm'])) {
  if (!empty($form_state['input']) && $form_state['input']['operation'] != 'donothing') {
    return drupal_build_form('auditfiles_not_on_server_form_confirm', $form_state);
  }

  // Initialize the table header.
  $header = array(
    array(
      'data' => t('File URI'),
      'field' => 'fm.uri',
      'sort' => 'desc',
    ),
  );

  // Initialize the table data variable.
  $files = array();

  // Get all the files from the files table using defined sort order.
  // Using a left join means all rows in {file_managed} are selected even if
  // there is no entry in {file_usage}.
  $query = db_select('file_managed', 'fm');
  $query->fields('fm', array('uri'));
  $query->extend('PagerDefault')->limit(25);
  $query->extend('TableSort')->orderByHeader($header);
  $result = $query->execute();

  // Iterate through the results
  foreach ($result as $file) {
    // Construct a valid drupal path for the named file
    $target = drupal_realpath($file->uri);

    // Check to see if the file exists.
    if (!file_exists($target)) {
      // If the file does not exist, strip out the directory path and store the
      // result.
      $files[$file->uri] = $file->uri;
    }
  }

  // Output count of files not on the server.
  if ($files) {
    $form['count'] = array(
      '#markup' => format_plural(
        count($files),
        '1 file found in the database that is not on the server.',
        '@count files found in the database that are not on the server.'
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
    'delete' => t('Delete checked files from database'),
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

  // Add checkboxes to the list of files.
  $form['files'] = array(
    '#type' => 'checkboxes',
    '#options' => $files,
  );

  return $form;
}

/**
 * Submit handler for the auditfiles_not_on_server_form form.
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
//function auditfiles_not_on_server_form_submit($form, &$form_state) {
//  if ($form_state['clicked_button']['#id'] == 'edit-submit' && $form_state['values']['operation'] != 'donothing') {
//    $form_state['storage']['confirm'] = TRUE;
//    $form_state['storage']['values'] = $form_state['values'];
//    $form_state['rebuild'] = TRUE;
//  }
//}

/**
 * Theme the auditfiles_not_on_server_form form.
 *
 * @param array $variables
 *   The variables currently available for use in a template file.
 *
 * @return string
 *   The formatted display output.
 */
function theme_auditfiles_not_on_server_form($variables) {
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
      array(
        'data' => t('File URI'),
        'field' => 'fm.uri',
        'sort' => 'desc',
      ),
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
function auditfiles_not_on_server_form_confirm($form, &$form_state) {
  $values = $form_state['input'];
  $operation = $values['operation'];
  switch ($operation) {
    case 'delete':
      $optype = 'delete';
      $seltype = 'selected';
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
      if ($selected) {
        if ($optype == 'delete') {
          $message = "<strong>$file</strong> will be deleted from the database.";
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
  $form['#submit'][] = 'auditfiles_not_on_server_form_confirm_submit';

  if (empty($form_state['input']['files'])) {
    return confirm_form(
      $form,
      t('No files have been selected to perform the action on.'),
      'admin/reports/auditfiles/notonserver',
      t('No files have been selected to perform the action on.'),
      '',
      t('Cancel')
    );
  }
  else {
    return confirm_form(
      $form,
      t('Are you sure you want to make these changes?'),
      'admin/reports/auditfiles/notonserver',
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
function auditfiles_not_on_server_form_confirm_submit($form, &$form_state) {
  if ($form_state['input']['confirm']) {
    $values = $form_state['input'];
    $operation = $values['operation'];

    foreach ($values['changelist'] as $filename => $selected) {
      if ($operation == 'delete') {
        $results = db_delete('file_managed')
          ->condition('uri', $filename)
          ->execute();
        if (empty($results)) {
          drupal_set_message(t('Failed to delete %file from the database.', array('%file' => $filename)));
        }
        else {
          drupal_set_message(t('Sucessfully deleted %file from the database.', array('%file' => $filename)));
        }
      }
    }
    if ($operation == 'delete') {
      drupal_set_message(t('The action of deleting files from the database is complete.'));
    }
    // Clear so our return to the primary form doesn't think we're going to the
    // confirmation form.
    unset($form_state['input']['confirm']);
  }
}
