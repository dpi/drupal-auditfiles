<?php
// $Id$

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
function auditfiles_unreferenced_form(&$form_state) {
  if (isset($form_state['storage']['confirm'])) {
    return auditfiles_unreferenced_form_confirm($form_state);
  }
  
  // Get the list of files that aren't in the database
  $filesunreferenced = _auditfiles_filesunreferenced();

  // Output count of files not in the database
  if ($filesunreferenced) {
    $form['count'] = array(
      '#value' => format_plural(count($filesunreferenced), '1 file found.', '@count files found.'),
    );
  }
  else {
    $form['count'] = array(
      '#value' => t('No files found.'),
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
  $files_dir = file_directory_path();
  $pathlen = strlen($files_dir)+1;
  $files=array();
  foreach ($filesunreferenced as $file) {
    $files[$file] = '';
    $filepath = substr($file, $pathlen);
    // Can't use file_create_url as the links fail if the site uses private transfers
    // Force a public url instead
    $basename = basename($file);
    $form['basename'][$file] = array('#value' => l($basename, 'admin/reports/auditfiles/unreferenced/'.$basename));
    $form['file'][$file] = array('#value' => l($filepath, $GLOBALS['base_url'] .'/'. str_replace('\\', '/', $file)));
  }

  // Add list of files to checkboxes
  $form['files'] = array('#type' => 'checkboxes', '#options' => $files);

  // Return form
  return $form;
}


/**
 * Theme auditfiles_unreferenced_form
 */
function theme_auditfiles_unreferenced_form($form) {
  // Render count
  $output = drupal_render($form['count']);

  // If there are files found
  if (isset($form['file']) && is_array($form['file'])) {

    // Render actions
    $output .= drupal_render($form['options']);

    // Construct table of files
    $header = array(
      t('Select'),
      t('Basename'),
      t('Path'),
    );

    foreach (element_children($form['file']) as $key) {
      $row = array();
      $row[] = drupal_render($form['files'][$key]);
      $row[] = drupal_render($form['basename'][$key]);
      $row[] = drupal_render($form['file'][$key]);
      $rows[] = $row;
    }

    // Render themed table
    $output .= theme('table', $header, $rows);

  }

  $output .= drupal_render($form);
  
  // Return output
  return $output;
}

function auditfiles_unreferenced_form_submit($form, &$form_state) {
//  drupal_set_message("submit form_state: ".dpr($form_state, TRUE));
  if ($form_state['clicked_button']['#id'] == 'edit-submit' &&
      $form_state['values']['options']['operation'] <> 'donothing') {
    $form_state['storage']['confirm'] = TRUE;
    $form_state['storage']['values'] = $form_state['values'];
  }
}

function auditfiles_unreferenced_form_confirm(&$form_state) {
  $values = $form_state['storage']['values'];
  drupal_set_message("form_confirm: values: ".dpr($values, TRUE));
  
  $form['changelist'] = array('#prefix' => '<ul>', '#suffix' => '</ul>', '#tree' => TRUE);

  // Note we iterate over fidvals, since no operation makes sense without a valid file
  $count = 0;
  foreach ($values['files'] as $key => $file) {
    if ($file) {
      $count++;
      // Limit what's displayed (also helps avoid max_allowed_packet errors)
      if ($count == 50) {
        $message = '...and many more...';
      } elseif ($count < 50) {
        $message = "Deleting file <strong>$file</strong>";
      }
      if ($message) {
        $form['changelist'][$key] = array('#type' => 'hidden', '#value' => $message, 
          '#prefix' => '<li>', '#suffix' => $message ."</li>\n");
        unset($message);
      }
    }
  }
  $form['operation'] = array('#type' => 'hidden', '#value' => 'delete');
  $form['#submit'][] = 'auditfiles_unreferenced_form_confirm_submit';

  return confirm_form(
    $form,
    t('Are you sure you want to delete these files?'),
    'admin/reports/auditfiles/unreferenced',
    '<strong>'. t('This action cannot be undone.') .'</strong>',
    t('Process all'),
    t('Cancel')
  );
}


function auditfiles_unreferenced_form_confirm_submit($form, &$form_state) {
//  drupal_set_message('confirm_submit, form: '.dpr($form, TRUE));
//  drupal_set_message('confirm_submit, form_state: '.dpr($form_state, TRUE));
  if ($form_state['values']['confirm']) {
    $values = $form_state['storage']['values'];
    drupal_set_message("in confirm_submit, values: ".dpr($values,TRUE));
    foreach ($values['files'] as $filename) {
      if ($filename) {
        db_query("DELETE FROM {files}
                  WHERE filepath='%s'",
                 $filename);
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

  // Prepare array to hold results
  $filesunreferenced = array();

  // Get all the files out the {files} table and store as qualified path
  $result = db_query('SELECT f.filepath 
                      FROM {files} f
                      LEFT JOIN {upload} u ON f.fid = u.fid
                      WHERE u.fid IS NULL
                      ORDER BY filepath ASC');
  while ($file = db_fetch_object($result)) {
    $filesunreferenced[] = file_create_path($file->filepath);
  }

  return $filesunreferenced;
}

function auditfiles_node_references_form($form_state, $basename) {
  $form['description'] = array(
    '#prefix' => '<div>',
    '#value' => t("The nodes listed below reference the filename <strong>$basename</strong>,
      but there is no record of the reference in the Drupal database. Review these nodes to
      determine how they're referencing the file.
      "),
    '#suffix' => '</div>',
  );
  $result = db_query("SELECT nr.nid,nr.title
                      FROM {node_revisions} nr
                      WHERE nr.body LIKE '%%%s%%'",
                     $basename);
  while ($node = db_fetch_object($result)) {
    drupal_set_message("nid=$node->nid, title=$node->title");
    $form['node'][$file] = array('#value' => l($node->title, 'node/'.$node->nid));
  }
  return $form;
}

/**
 * Theme auditfiles_unreferenced_form
 */
function theme_auditfiles_node_references_form($form) {
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
      $rows[] = $row;
    }

    // Render themed table
    $output .= theme('table', $header, $rows);

  }

  $output .= drupal_render($form);
  
  // Return output
  return $output;
}

