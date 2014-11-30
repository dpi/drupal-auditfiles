<?php
/**
 * @file
 * This generates the report showing the files referenced by content, but not in
 * the database.
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

  // Define the Actions form elements.
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

  // Start at 1; $files checkboxes shouldn't have a 0 key
  $match_id = 1;
  $files = array();
  $single_match = 0;
  $multi_match = 0;
  $no_match = 0;
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

  // Stuff not likely to represent local file references.
  $ignore_patterns = array(
    'window\.open',
    '^mailto:',
    '\.html?($|\?)',
    '^#',
    '^javascript:void\(0\)',
    '^ftp:\/\/',
  );
  // Add references to our own home page(s)
  $domains = variable_get('auditfiles_include_domains', '');
  if (!empty($domains)) {
    $domains = explode(' ', $domains);
    foreach ($domains as $domain) {
      $ignore_patterns[] = "^https?:\/\/$domain\/?$";
    }
  }
  $ignore_pattern = '/' . implode('|', $ignore_patterns) . '/';

  // Get a list of nodes that might have a file reference in them.
  // @TODO Please convert this statement to the D7 database API syntax.
  $result = db_query(
    "SELECT n.nid, n.title, b.body_value
    FROM {node} n
    INNER JOIN {node_revision} nr ON n.vid = nr.vid
    INNER JOIN {field_data_body} b ON b.revision_id = nr.vid
    WHERE b.body_value LIKE '%img%'
       OR b.body_value LIKE '%window.open%'
       OR b.body_value LIKE '%href%'
    ORDER BY n.nid DESC
    LIMIT 100"
  );
  // Process each node found with a potential file reference.
  foreach ($result as $node) {
dpm($node, '$node');
    $node_link = l($node->title, 'node/' . $node->nid);

    // Pull all the matches together.
    preg_match_all($reg_pattern, $node->body_value, $file_match_groups);
    $file_matches = array();
    for ($i = 1; $i < count($patterns) + 1; $i++) {
      $file_matches = array_merge($file_matches, $file_match_groups[$i]);
    }

    // Process each potential file reference.
    for ($i = 0; $i < count($file_matches); $i++) {
      $source = trim($file_matches[$i]);
      // Don't process any empty strings created as a result of the previous
      // merging.
      if (!$source) {
        continue;
      }
      // Don't process any strings that are likely external to this system.
      if (preg_match($ignore_pattern, $source)) {
        continue;
      }

      // If we've got a full URL, and the domain is not in our list of "local"
      // domains, assume it's a valid external URL and leave it be.
      // Search the source for a URI indicator.
      $domain_found = preg_match('@^https?://([^/$]+)@', $source, $matches);
      $local = FALSE;
      if (!$domain_found) {
        // The file reference is local to this system.
        $local = TRUE;
      }
      else {
        // If local domains have been specified, search them to see if the
        // current string is one of them.
        if (!empty($domains)) {
          $source_domain = $matches[1];
          foreach ($domains as $domain) {
            if ($domain == $source_domain) {
              $local = TRUE;
              break;
            }
          }
        }
      }
      // If the domain is not local, add it to a list of external domains and
      // stop processing the file reference.
      if (!$local) {
        $external_domains[$matches[0]]++;
        continue;
      }

      // Fix up encoded spaces.
      $decode_source = str_replace('%20', ' ', $source);
dpm($decode_source, '$decode_source');

      $file_dir_path = drupal_realpath('public://');

      // If there is an exact match on path and an exact match on that path in
      // the {file_usage} table, we don't have to go farther.

      // Search the file tables for the file reference to see if it is there.
      // We need to compare {file_managed}.uri with $decode_source, which are in
      // different formats. {file_managed}.uri is like "scheme://path/to/file.nam" and $decode_source may be like "http://domainname.com/
      $query = "SELECT fu.id
        FROM {file_usage} fu
        INNER JOIN {file_managed} fm ON fm.fid = fu.fid
        WHERE fu.id = :fu_nid
        AND fm.uri = :fm_uri";
      $hit = db_query(
        $query,
        array(
          ':fu_nid' => $node->nid,
          ':fm_uri' => $decode_source,
        )
      )->fetchField();

      // The file reference is not exactly matched in the file tables, so
      // include it in the list for the user to see.
      if (!$hit) {
        $num_matches = 0;
        $decode_base = basename($decode_source);
        $query = "SELECT fm.fid, fm.filename, fu.id
          FROM {file_managed} fm
          LEFT JOIN {file_usage} fu ON fm.fid = fu.fid
          WHERE fu.id = :fu_id
          AND fm.filename = :fm_filename";
        $file_result = db_query(
          $query,
          array(
            ':fu_id' => $node->nid,
            ':fm_filename' => $decode_base,
          )
        );

        while ($file = $file_result->fetch()) {
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
dpm($num_matches, '$num_matches');

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
dpm($single_match, '$single_match');
dpm($multi_match, '$multi_match');
dpm($no_match, '$no_match');

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
        /* db_query("DELETE FROM {file_managed}
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
        // If necessary, write record to {file_usage} table
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
          /* db_query("INSERT INTO {file_usage}
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
