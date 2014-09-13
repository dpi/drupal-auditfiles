<?php
/**
 * @file
 * Generates the report showing files in the database, but not referenced by
 * content.
 */

/**
 * Menu callback for files not in the database.
 *
 * @return array
 *   The form definition.
 */
function auditfiles_unreferenced() {
  return drupal_get_form('auditfiles_unreferenced_form');
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
function auditfiles_unreferenced_form($form, &$form_state) {
  if (isset($form_state['storage']['confirm'])) {
    return auditfiles_unreferenced_form_confirm($form, $form_state);
  }

  // Get the list of files that aren't in the database
  $unreferenced_files = _auditfiles_unreferenced_files();

  // Output count of files not in the database
  if (!empty($unreferenced_files)) {
    $form['count'] = array(
      '#markup' => format_plural(count($unreferenced_files), '1 file found.', '@count files found.'),
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
    'delete' => t('Delete selected files'),
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
  foreach ($unreferenced_files as $uri) {
    $files[$uri] = '';
    $form['file'][$uri] = array(
      '#markup' => l($uri, 'admin/reports/auditfiles/unreferenced/' . $uri)
    );
  }
//dpm($form, '$form');

  // Add list of files to checkboxes
  $form['files'] = array(
    '#type' => 'checkboxes',
    '#options' => $files,
  );

  // Return form
  return $form;
}

/**
 * Submit handler for the auditfiles_unreferenced_form form.
 *
 * @param array $form
 *   The form definition.
 * @param array $form_state
 *   The current state of the form.
 */
function auditfiles_unreferenced_form_submit($form, &$form_state) {
  if ($form_state['clicked_button']['#id'] == 'edit-submit' &&
    $form_state['values']['operation'] != 'donothing') {
    $form_state['storage']['confirm'] = TRUE;
    $form_state['storage']['values'] = $form_state['values'];
  }
}

/**
 * Theme the auditfiles_unreferenced_form form.
 *
 * @param array $variables
 *   The variables currently available for use in a template file.
 *
 * @return string
 *   The formatted display output.
 */
function theme_auditfiles_unreferenced_form($variables) {
  $form = $variables['form'];
dpm($form, '$form');

  $output = drupal_render($form['count']);

  // If there are files found...
  if (isset($form['file']) && is_array($form['file'])) {
    $output .= drupal_render($form['options']);

    // Construct table of files.
    $header = array(
      t('Select'),
      t('Uri'),
    );

    $rows = array();
    foreach (element_children($form['file']) as $uri) {
      $row = array();
      $row[] = drupal_render($form['files'][$uri]);
      $row[] = drupal_render($form['file'][$uri]);
      $rows[] = array('data' => $row);
    }
dpm($rows, '$rows');

    // Render themed table
    $output .= theme('table', array('header' => $header, 'rows' => $rows));
  }

  $output .= drupal_render_children($form);

  // Return output
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
function auditfiles_unreferenced_form_confirm($form, &$form_state) {
  $form['change_list'] = array(
    '#prefix' => '<ul>',
    '#suffix' => '</ul>',
    '#tree' => TRUE,
  );

  // Note we iterate over fidvals, since no operation makes sense without a valid file
  $count = 0;
  foreach ($form_state['storage']['values']['files'] as $key => $file) {
    if ($file) {
      $count++;
      // Limit what's displayed (also helps avoid max_allowed_packet errors)
      if ($count == 50) {
        $message = '...and many more...';
      }
      elseif ($count < 50) {
        $message = "Deleting file <strong>$file</strong>";
      }
      if ($message) {
        $form['change_list'][$key] = array(
          '#type' => 'hidden',
          '#value' => $message,
          '#prefix' => '<li>',
          '#suffix' => $message . "</li>\n",
        );
        unset($message);
      }
    }
  }

  $form['operation'] = array(
    '#type' => 'hidden',
    '#value' => 'delete',
  );

  $form['#submit'][] = 'auditfiles_unreferenced_form_confirm_submit';

  return confirm_form(
    $form, t('Are you sure you want to delete these files?'), 'admin/reports/auditfiles/unreferenced', '<strong>' . t('This action cannot be undone.') . '</strong>', t('Process all'), t('Cancel')
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
function auditfiles_unreferenced_form_confirm_submit($form, &$form_state) {
  if ($form_state['values']['confirm']) {
    foreach ($form_state['storage']['values']['files'] as $filename) {
      if (!empty($filename)) {
        // Get the file object.
        $query = "SELECT fid FROM {file_managed} WHERE filename = :filename";
        $fid = db_query($query, array(':filename' => $filename))->fetchField();
        $file = file_load($fid);

        // Delete the file from the file system.
        if (file_unmanaged_delete($file->uri)) {
          watchdog('auditfiles', '%file was deleted from the file system.', array('%file' => $filename));
        }
        else {
          drupal_set_message(t('%file was not deleted. See the system logs for why the action failed.', array('%file' => $filename)));
        }

        // Remove the file listing from the {file_managed} table.
        db_delete('file_managed')
          ->condition('fid', $file->fid)
          ->execute();

        // Remove the file listing from the {file_usage} table.
        db_delete('file_usage')
          ->condition('fid', $file->fid)
          ->execute();
      }
    }

    // Clear so our return to the primary form doesn't think we're going to the confirmation form
    unset($form_state['storage']['confirm']);
  }
}

/**
 * Retrieves a list of files in {file_managed} but not in {file_usage}.
 *
 * @return array
 *   The list of files in the {file_managed} table but not in the {file_usage}
 *   table.
 */
function _auditfiles_unreferenced_files() {
  $all_files = db_query('SELECT uri FROM {file_managed} ORDER BY uri ASC')->fetchAll(PDO::FETCH_COLUMN);
  $found_files = array();
  $fields = field_info_fields();
  foreach ($fields as $field) {
    if ($field['type'] == 'file' || $field['type'] == 'image') {
      // Get the database table name for the field.
      $table = key($field['storage']['details']['sql']['FIELD_LOAD_CURRENT']);
      // Get the column name in the database table for the field.
      $column = $field['storage']['details']['sql']['FIELD_LOAD_CURRENT'][$table]['fid'];
      // The query needs to constructed without place holders, because Drupal
      // automatically adds quotes where they don't belong, otherwise.
      $result = db_query("SELECT f.uri FROM {file_managed} f INNER JOIN {$table} t ON f.fid = t.$column");
      foreach ($result as $uri) {
        if (in_array($uri, $all_files)) {
          $found_files[] = $uri;
        }
      }
    }
  }

  return array_diff($all_files, $found_files);
}


/**
 * Form definition for a list of nodes referencing a single file.
 *
 * @param array $form
 *   The form definition.
 * @param array $form_state
 *   The current state of the form.
 * @param string $basename
 *
 * @return array
 *   The form definition.
 */
function auditfiles_node_references_form($form, $form_state, $basename) {
  $form['description'] = array(
    '#prefix' => '<div>',
    '#value' => t(
      "The nodes listed below reference the filename %basename, but there is no
        record of the reference in the Drupal database. Review these nodes to
        determine how they're referencing the file.",
      array('%basename' => $basename)
    ),
    '#suffix' => '</div>',
  );

  $query = "SELECT nid, title FROM {node} WHERE body LIKE '%%:basename%%'";

  $result = db_query($query, array(':basename' => $basename));
  foreach ($result as $node) {
    $form['node'][$file] = array('#value' => l($node->title, 'node/' . $node->nid));
  }

  return $form;
}

/**
 * Theme the auditfiles_node_references_form form.
 *
 * @param array $variables
 *   The variables currently available for use in a template file.
 *
 * @return string
 *   The formatted display output.
 */
function theme_auditfiles_node_references_form($variables) {
  $form = $variables['form'];

  $output = drupal_render($form['description']);

  // If there are files found...
  if (!empty($form['node']) && is_array($form['node'])) {
    // Construct a table of files.
    $header = array(
      t('Matching nodes'),
    );

    $rows = array();
    foreach (element_children($form['node']) as $key) {
      $rows[] = array('data' => drupal_render($form['node'][$key]));
    }

    // Render themed table
    $output .= theme('table', array('header' => $header, 'rows' => $rows));
  }

  $output .= drupal_render_children($form);

  return $output;
}
