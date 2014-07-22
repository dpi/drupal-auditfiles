<?php

/**
 * @file
 * Callback and functions to generate unreferenced files report
 */

/**
 * Menu callback: audit files not in the database.
 */
function auditfiles_unreferenced() {
  return drupal_get_form('auditfiles_unreferenced_form');
}

/**
 * Form definition for audit files not in database
 */
function auditfiles_unreferenced_form($form, &$form_state) {
  if (isset($form_state['storage']['confirm'])) {
    return auditfiles_unreferenced_form_confirm($form, $form_state);
  }

  // Get the list of files that aren't in the database
  $filesunreferenced = _auditfiles_filesunreferenced();

  // Output count of files not in the database
  if ($filesunreferenced) {
    $form['count'] = array(
      '#markup' => format_plural(count($filesunreferenced), '1 file found.', '@count files found.'),
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
  foreach ($filesunreferenced as $uri) {
    $files[$uri] = '';
    $form['file'][$uri] = array('#markup' => l($uri, 'admin/reports/auditfiles/unreferenced/' . $uri));
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
 * Theme auditfiles_unreferenced_form
 */
function theme_auditfiles_unreferenced_form($variables) {
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
      t('Uri'),
    );

    foreach (element_children($form['file']) as $uri) {
      $row = array();
      $row[] = drupal_render($form['files'][$uri]);
      $row[] = drupal_render_children($form['file'][$uri]);
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
function auditfiles_unreferenced_form_submit($form, &$form_state) {
//  drupal_set_message("submit form_state: ".dpr($form_state, TRUE));
  if ($form_state['clicked_button']['#id'] == 'edit-submit' &&
  $form_state['values']['operation'] <> 'donothing') {
    $form_state['storage']['confirm'] = TRUE;
    $form_state['storage']['values'] = $form_state['values'];
  }
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function auditfiles_unreferenced_form_confirm($form, &$form_state) {
  $values = $form_state['storage']['values'];
  drupal_set_message("form_confirm: values: " . dpr($values, TRUE));

  $form['changelist'] = array(
    '#prefix' => '<ul>',
    '#suffix' => '</ul>',
    '#tree' => TRUE,
  );

  // Note we iterate over fidvals, since no operation makes sense without a valid file
  $count = 0;
  foreach ($values['files'] as $key => $file) {
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
        $form['changelist'][$key] = array(
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
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function auditfiles_unreferenced_form_confirm_submit($form, &$form_state) {
//  drupal_set_message('confirm_submit, form: '.dpr($form, TRUE));
//  drupal_set_message('confirm_submit, form_state: '.dpr($form_state, TRUE));
  if ($form_state['values']['confirm']) {
    $values = $form_state['storage']['values'];
    drupal_set_message("in confirm_submit, values: " . dpr($values, TRUE));
    foreach ($values['files'] as $filename) {
      if ($filename) {
        // TODO Please review the conversion of this statement to the D7 database API syntax.
        /* db_query("DELETE FROM {files}
         WHERE filepath='%s'", $filename) */
        db_delete('files')
  ->condition('filepath', $filename)
  ->execute();
        if (file_delete(file_create_path($filename))) {
          watchdog('audit', '%file was deleted', array('%file' => $filename));
        }
        else {
          drupal_set_message(t('Failed to delete %file', array('%file' => $filename)));
        }
      }
    }
    // Clear so our return to the primary form doesn't think we're going to the confirmation form
    unset($form_state['storage']['confirm']);
  }
}

/**
 * Helper function - retrieve sorted list of files that are in {files}
 * but not in {upload}
 */
function _auditfiles_filesunreferenced() {

  $allFiles = db_query('SELECT f.uri FROM {file_managed} f ORDER BY f.uri ASC')->fetchAll(PDO::FETCH_COLUMN);

  $foundFiles = array();

  $fields = field_info_fields();
  foreach ($fields as $name => $field) {
    if ($field['type'] == 'file' || $field['type'] == 'image') {
      $table = key($field['storage']['details']['sql']['FIELD_LOAD_CURRENT']);
      $column = $field['storage']['details']['sql']['FIELD_LOAD_CURRENT'][$table]['fid'];
      // TODO Please convert this statement to the D7 database API syntax.
      $result = db_query('SELECT f.uri FROM {file_managed} f INNER JOIN ' . $table . ' u ON f.fid = u.' . $column)->fetchAll(PDO::FETCH_COLUMN);

      foreach ($result as $uri) {
        if (in_array($uri, $allFiles)) {
          $foundFiles[] = $uri;
        }
      }
    }
  }

  return array_diff($allFiles, $foundFiles);
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function auditfiles_node_references_form($form, $form_state, $basename) {
  $form['description'] = array(
    '#prefix' => '<div>',
    '#value' => t("The nodes listed below reference the filename <strong>$basename</strong>,
      but there is no record of the reference in the Drupal database. Review these nodes to
      determine how they're referencing the file.
      "),
    '#suffix' => '</div>',
  );
  // TODO Please convert this statement to the D7 database API syntax.
  $result = db_query("SELECT nr.nid,nr.title
                      FROM {node_revisions} nr
                      WHERE nr.body LIKE '%%%s%%'", $basename);
  foreach ($result as $node) {
    drupal_set_message("nid=$node->nid, title=$node->title");
    $form['node'][$file] = array('#value' => l($node->title, 'node/' . $node->nid));
  }
  return $form;
}

/**
 * Theme auditfiles_unreferenced_form
 */
function theme_auditfiles_node_references_form($variables) {
  $variables = $variables['form'];
  $form = $variables['form'];
  // Render count
  $output = drupal_render($form['description']);

  // If there are files found
  if (isset($form['node']) && is_array($form['node'])) {
    // Construct table of files
    $header = array(
      t('Matching nodes'),
    );

    foreach (element_children($form['node']) as $key) {
      $row = array();
      $row[] = drupal_render($form['node'][$key]);
      $rows[] = array('data' => $row);
    }

    // Render themed table
    $output .= theme('table', array('header' => array('header' => $header, 'rows' => $rows)));
  }

  $output .= drupal_render_children($form);

  // Return output
  return $output;
}
