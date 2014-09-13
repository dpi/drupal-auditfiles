<?php
/**
 * @file
 * Generates the report showing files referenced by content, but not in the
 * database.
 */

/**
 * Menu callback for files referenced but not attached via uploads.
 *
 * @return array
 *   The form definition.
 */
function auditfiles_references() {
  return drupal_get_form('auditfiles_references_form');
}

/**
 * Form definition for audit files not in database
 */
function auditfiles_references_form($form, &$form_state) {
  if (isset($form_state['storage']['confirm'])) {
    return auditfiles_references_form_confirm($form, $form_state);
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
    'attach' => t('Attach selected files'),
    'attachunique' => t('Attach all unique matches'),
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
  // Note we cut down the overhead by preselecting nodes with potential
  // to match the $patterns below
  // @TODO Please convert this statement to the D7 database API syntax.
  $result = db_query(
    "SELECT n.nid, n.title, b.body_value
    FROM {node} n
    INNER JOIN {node_revision} nr ON n.vid = nr.vid
    INNER JOIN {field_data_body} b ON b.revision_id = nr.vid
    WHERE b.body_value LIKE '%img%'
       OR b.body_value LIKE '%window.open%'
       OR b.body_value LIKE '%href%'
    ORDER BY n.nid"
  );

  // Start at 1 - $files checkboxes shouldn't have a 0 key
  $match_id = 1;
  $files = array();
  $single_match = 0;
  $multi_match = 0;
  $no_match = 0;
  $file_dir_path = drupal_realpath('public://');
  $domains = explode(' ', variable_get('auditfiles_include_domains', ''));
  $external_domains = array();

  $patterns = array(
    // Get src attributes from img tags (assumes ")
    '(?:<img [^>]*src[ ]*=[ ]*"([^"]*)")',
    // Get href attributes from a tags (assumes ")
    '(?:<a [^>]*href[ ]*=[ ]*"([^"]*)")',
    // Get window.open argument (assumes ')
    "(?:window.open[ ]*\([ ]*'([^\']*)')",
  );
  $reg_pattern = '/' . implode('|', $patterns) . '/';

  // Stuff not likely to represent local file references
  $ignore_patterns = array(
    'window\.open',
    '^mailto:',
    '\.html?($|\?)',
    '^#',
    '^javascript:void\(0\)',
    '^ftp:\/\/',
  );
  // Add references to our own home page(s)
  foreach ($domains as $domain) {
    $ignore_patterns[] = "^https?:\/\/$domain\/?$";
  }
  $ignore_pattern = '/' . implode('|', $ignore_patterns) . '/';

  foreach ($result as $node) {
    $node_link = l($node->title, 'node/' . $node->nid);

    // This is not initialized or used.
//    $images_matched = preg_match_all($reg_pattern, $node->body_value, $image_match_groups);

    // Pull all matches together
    $image_matches = array();
    for ($i = 1; $i < count($patterns) + 1; $i++) {
      $image_matches = array_merge($image_matches, $image_match_groups[$i]);
    }

    for ($i = 0; $i < count($image_matches); $i++) {
      $source = trim($image_matches[$i]);
      // Merge gives us some blanks
      if (!$source) {
        continue;
      }
      // Bunch of stuff that won't get us anywhere
      if (preg_match($ignore_pattern, $source)) {
        continue;
      }

      // If we've got a full URL, and the domain is not in our list of "local"
      // domains, assume it's a valid external URL and leave it be
      $domain_found = preg_match('@^https?://([^/$]+)@', $source, $matches);
      if (!$domain_found) {
        $local = TRUE;
      }
      else {
        $local = FALSE;
        $source_domain = $matches[1];
        foreach ($domains as $domain) {
          if ($domain == $source_domain) {
            $local = TRUE;
            break;
          }
        }
      }
      if (!$local) {
        $external_domains[$source_domain]++;
        continue;
      }

      // Fix up encoded spaces
      $decode_source = str_replace('%20', ' ', $source);

      // If we've got an exact match on path, and have an exact match on that
      // path in the {upload} table, we don't have to go farther (note how we
      // skip the / in the src)
      $hit = db_query(
        "SELECT u.nid
        FROM {files} f
        INNER JOIN {upload} u ON f.fid=u.fid
        WHERE u.nid = :u.nid
          AND CONCAT('/', f.filepath) = :f.filepath)",
        array(
          ':u.nid' => $node->nid,
          ':f.filepath)' => $decode_source))
        ->fetchField();

      if (!$hit) {
        $num_matches = 0;
        $basename = basename($source);
        $decode_base = basename($decode_source);
        $file_result = db_query(
          "SELECT f.fid, f.filepath, u.nid
          FROM {files} f
          LEFT JOIN {upload} u ON f.fid = u.fid AND u.nid = %d
          WHERE f.filename = :f.filename",
          array(
            ':f.filename' => $node->nid,
            '' => $decode_base,
          )
        );
        while ($file = db_fetch_object($file_result)) {
          $num_matches++;
          // Visible fields first
          $form['titles'][$match_id] = array('#value' => $node_link);
          $form['sources'][$match_id] = array('#value' => $source);
          $form['fids'][$match_id] = array('#value' => $file->fid);

          // Strip the Drupal file path, and make the link
          $filepath = preg_replace('@^' . preg_quote($file_dir_path) . '/@', '', $file->filepath);
          $filelink = l($filepath, $GLOBALS['base_url'] . '/' . $file_dir_path . '/' . str_replace('\\', '/', $filepath));
          $form['paths'][$match_id] = array('#value' => $filelink);
          $files[$match_id] = '';

          // Fields passed through form submission
          $form['nid_values'][$match_id] = array(
            '#type' => 'value',
            '#value' => $node->nid,
          );
          $form['source_values'][$match_id] = array(
            '#type' => 'value',
            '#value' => $source,
          );
          $form['fid_values'][$match_id] = array(
            '#type' => 'value',
            '#value' => $file->fid,
          );
          $form['path_values'][$match_id] = array(
            '#type' => 'value',
            '#value' => $file->filepath,
          );

          if (!$file->nid) {
            $form['no_upload'][$match_id] = array(
              '#type' => 'value',
              '#value' => TRUE,
            );
          }

          $match_id++;
        }

        if ($num_matches == 0) {
          $no_match++;

          $form['titles'][$match_id] = array('#value' => $node_link);
          $form['sources'][$match_id] = array('#value' => $source);
          $form['fids'][$match_id] = array('#value' => '');
          $form['paths'][$match_id] = array('#value' => '');
          
          $files[$match_id] = '';

          $form['nid_values'][$match_id] = array(
            '#type' => 'value',
            '#value' => $node->nid,
          );
          $form['source_values'][$match_id] = array(
            '#type' => 'value',
            '#value' => $source,
          );
          $form['no_upload'][$match_id] = array(
            '#type' => 'value',
            '#value' => TRUE,
          );

          $match_id++;
        }
        elseif ($num_matches == 1) {
          $single_match++;

          $form['unique'][$match_id - 1] = array(
            '#type' => 'value',
            '#value' => TRUE,
          );
        }
        else {
          $multi_match++;
        }
      }
    }
  }

  if (count($external_domains) > 0) {
    $form['external_domains'] = array(
      '#type' => 'fieldset',
      '#title' => 'External domains referenced',
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );

    arsort($external_domains);

    foreach ($external_domains as $domain => $count) {
      $form['domains'][$domain] = array('#value' => $domain);
      $form['domain_counts'][$domain] = array('#value' => $count);
    }
  }

  // Remember, we started at 1 instead of 0...
  if ($match_id > 1) {
    $broken = $no_match + $single_match + $multi_match;
    $total = format_plural($broken, '1 broken reference found:', '@count broken references found:');
    $unmatched = "$no_match with no matches";
    $unique_match = "$single_match with a unique match";
    $multi_matches = "$multi_match with multiple matches";
    $form['count'] = array(
      '#value' => "<div>$total<br />$unmatched<br />$unique_match<br />$multi_matches</div>",
    );
  }
  else {
    $form['count'] = array(
      '#value' => t('No broken references found.'),
    );
  }

  // Add list of files to checkboxes
  $form['files'] = array(
    '#type' => 'checkboxes',
    '#options' => $files,
  );

  // Maintains hidden fields
  $form['#tree'] = TRUE;

  // Return form
  return $form;
}

/**
 * Submit handler for the auditfiles_references_form form.
 *
 * @param array $form
 *   The form definition.
 * @param array $form_state
 *   The current state of the form.
 */
function auditfiles_references_form_submit($form, &$form_state) {
  if ($form_state['clicked_button']['#id'] == 'edit-options-submit' &&
    $form_state['values']['options']['operation'] <> 'donothing') {
    $form_state['storage']['confirm'] = TRUE;
    $form_state['storage']['values'] = $form_state['values'];
  }
}

/**
 * Theme the auditfiles_references_form form.
 *
 * @param array $variables
 *   The variables currently available for use in a template file.
 *
 * @return string
 *   The formatted display output.
 */
function theme_auditfiles_references_form($variables) {
  $form = $variables['form'];
  // Render count
  $output = drupal_render($form['count']);

  // List any external domain references found
  if (isset($form['domains']) && is_array($form['domains'])) {
    $header = array(
      array('data' => t('External Domain')),
      array('data' => t('# References')),
    );
    foreach (element_children($form['domains']) as $key) {
      $row = array();
      $row[] = drupal_render($form['domains'][$key]);
      $row[] = drupal_render($form['domain_counts'][$key]);
      $rows[] = array('data' => $row);
    }

    // Render themed table
    $tableoutput = theme('table', array('header' => array('header' => $header, 'rows' => $rows)));
    $form['external_domains']['list'] = array('#markup' => $tableoutput);
    $output .= drupal_render($form['external_domains']);
  }

  // If there are files found
  if (isset($form['titles']) && is_array($form['titles'])) {
    // Render actions
    $output .= drupal_render($form['options']);

    // Construct table of files
    $header = array(
      t('Select'),
      t('Node'),
      t('Src'),
      t('Fid'),
      t('Path'),
    );

    $rows = array();
    foreach (element_children($form['titles']) as $key) {
      $row = array();
      $row[] = drupal_render($form['files'][$key]);
      $row[] = drupal_render($form['titles'][$key]);
      $row[] = drupal_render($form['sources'][$key]);
      $row[] = drupal_render($form['fids'][$key]);
      $row[] = drupal_render($form['paths'][$key]);
      $rows[] = array('data' => $row);
    }

    // Render themed table
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
function auditfiles_references_form_confirm($form, &$form_state) {
  $values = $form_state['storage']['values'];
  $operation = $values['options']['operation'];
  switch ($operation) {
    case 'delete':
      $optype = 'delete';
      $seltype = 'selected';
      break;
    case 'attach':
      $optype = 'attach';
      $seltype = 'selected';
      break;
    case 'attachunique':
      $optype = 'attach';
      $seltype = 'unique';
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

  // Note we iterate over fid_values, since no operation makes sense without a valid file
  $count = 0;
  foreach ($values['fid_values'] as $id => $value) {
    if (($seltype == 'unique' && $values['unique'][$id]) ||
      ($seltype == 'selected' && $values['files'][$id] == $id)) {
      $count++;
      // Limit what's displayed (also helps avoid max_allowed_packet errors)
      if ($count == 50) {
        $message = '...and many more...';
      }
      elseif ($count < 50) {
        if ($optype == 'delete') {
          $message = "Deleting file <strong>" . $values['path_values'][$id] . '</strong>';
        }
        else {
          $message = 'Replacing src <strong>' . $values['source_values'][$id] . '</strong> with <strong>' .
              $values['path_values'][$id] . '</strong> for node rev <strong>' . $values['nid_values'][$id] .
              '</strong>';
        }
      }
      if ($message) {
        $form['changelist'][$id] = array(
          '#type' => 'hidden',
          '#value' => $message,
          '#prefix' => '<li>',
          '#suffix' => $message . "</li>\n",
        );
        unset($message);
      }
    }
    else {
      // Unsetting the unprocessed fid_values prevents confirm_submit from dealing with them
      unset($form_state['storage']['values']['fid_values'][$id]);
    }
  }
  $form['operation'] = array(
    '#type' => 'hidden',
    '#value' => $operation,
  );
  $form['#submit'][] = 'auditfiles_references_form_confirm_submit';

  return confirm_form(
    $form, t('Are you sure you want to make these changes?'), 'admin/reports/auditfiles/references', '<strong>' . t('This action cannot be undone.') . '</strong>', t('Process all'), t('Cancel')
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
function auditfiles_references_form_confirm_submit($form, &$form_state) {
  if ($form_state['values']['confirm']) {
    $values = $form_state['storage']['values'];
    foreach ($values['fid_values'] as $id => $fid) {
      if ($values['options']['operation'] == 'delete') {
        // @TODO Please review the conversion of this statement to the D7 database API syntax.
        /* db_query("DELETE FROM {files}
         WHERE fid=%d", $fid) */
        db_delete('files')
          ->condition('fid', $fid)
          ->execute();
        if (file_delete(file_create_path($values['path_values'][$id]))) {
          drupal_set_message(t('%file was deleted', array('%file' => $values['path_values'][$id])));
        }
        else {
          drupal_set_message(t('Failed to delete %file', array('%file' => $values['path_values'][$id])));
        }
      }
      else {
        // Load $values['nid_values'][$id], replace source_values with path_values, save
        // If necessary, write record to {upload} table
        $newpath = '/' . $values['path_values'][$id];
        $oldpath = $values['source_values'][$id];

        if ($newpath != $oldpath) {
          $row = db_fetch_object(db_query("SELECT nr.body,nr.vid
                                          FROM {node_revisions} nr
                                          INNER JOIN {node} n ON nr.vid=n.vid
                                          WHERE n.nid = :n.nid", array(':n.nid' => $values['nid_values'][$id])));
          $body = str_replace($oldpath, $newpath, $row->body);
          // @TODO Please review the conversion of this statement to the D7 database API syntax.
          /* db_query("UPDATE {node_revisions}
           SET body='%s'
           WHERE vid=%d", $body, $row->vid) */
          db_update('node_revisions')
            ->fields(array('body' => $body))
            ->condition('vid', $row->vid)
            ->execute();
        }
        if ($values['no_upload'][$id]) {
          // @TODO Please convert this statement to the D7 database API syntax.
          /* db_query("INSERT INTO {upload}
           (fid, nid, vid, description, list, weight)
           SELECT %d, n.nid, n.vid, 'Attached by auditfiles', 0, 0
           FROM {node} n
           WHERE n.nid=%d", $fid, $values['nid_values'][$id]) */
          NULL;
        }
        drupal_set_message(t(
          'Updated path %oldpath to %newpath for revision %revision.',
          array(
            '%oldpath' => $oldpath,
            '%newpath' => $newpath,
            '%revision' => $values['nid_values'][$id],
          )
        ));
      }
    }
    // Clear so our return to the primary form doesn't think we're going to the confirmation form
    unset($form_state['storage']['confirm']);
  }
}
