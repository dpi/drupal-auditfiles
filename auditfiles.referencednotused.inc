<?php
/**
 * @file
 * This generates the report showing the files referenced by content, but not in
 * the database.
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
function auditfiles_referenced_not_used_form($form, &$form_state) {
  // Check to see if the confirmation form needs to be displayed instead of the
  // normal form.
  if (isset($form_state['storage']['confirm'])) {
    return _auditfiles_referenced_not_used_confirm_operation($form, $form_state);
  }

  $header = array(
    'fid' => array(
      'data' => t('File ID'),
    ),
    'entity_type' => array(
      'data' => t('Referencing entity type'),
    ),
    'entity_id_display' => array(
      'data' => t('Referencing entity ID'),
    ),
    'field' => array(
      'data' => t('Field referenced in'),
    ),
    'uri' => array(
      'data' => t('URI'),
    ),
    'filemime' => array(
      'data' => t('MIME'),
    ),
    'filesize' => array(
      'data' => t('Size (in bytes)'),
    ),
  );

  $rows = _auditfiles_referenced_not_used_get_data();

  // Set up the pager.
  if (!empty($rows)) {
    $items_per_page = variable_get('auditfiles_report_options_items_per_page', 50);
    $current_page = pager_default_initialize(count($rows), $items_per_page);
    // Break the total data set into page sized chunks.
    $pages = array_chunk($rows, $items_per_page, TRUE);
  }

  // Define the form.
  // Setup the record count and related messages.
  if (!empty($rows)) {
    $maximum_records = variable_get('auditfiles_report_options_maximum_records', 0);
    if ($maximum_records > 0 && count($rows) >= $maximum_records) {
      $file_count_message = 'Found at least @count files referenced in content that are not in the file_usage table.';
    }
    else {
      $file_count_message = 'Found @count files referenced in content that are not in the file_usage table.';
    }
    $form_count = format_plural(
      count($rows),
      'Found 1 file referenced in content that is not in the file_usage table.',
      $file_count_message
    );
  }
  else {
    $form_count = 'Found no files referenced in content that are not in the file_usage table.';
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
  else {
    $form['files']['#options'] = array();
  }

  if (!empty($rows)) {
    $form['description'] = array(
      '#markup' => t("If the file object does not exist for the reference, it cannot be added to Drupal's file management with this method."),
    );

    $form['actions'] = array('#type' => 'actions');
    $form['actions']['add'] = array(
      '#type' => 'submit',
      '#value' => t('Add selected items to Drupal file management'),
    );
    $form['actions']['markup'] = array(
      '#markup' => '&nbsp;' . t('or') . '&nbsp;',
    );
    $form['actions']['delete'] = array(
      '#type' => 'submit',
      '#value' => t('Delete selected references'),
    );

    $form['pager'] = array('#markup' => theme('pager'));
  }

  return $form;
}

/**
 * Submit handler for the auditfiles_referenced_not_used_form form.
 */
function auditfiles_referenced_not_used_form_submit($form, &$form_state) {
  // Check to see if any files were selected.
  if (!empty($form_state['storage']['files'])) {
    foreach ($form_state['storage']['files'] as $file_id) {
      if ($file_id != 0) {
        // The operation has been confirmed, so delete the records.
        // The operation has been confirmed.
        if ($form_state['storage']['op'] == 'Add selected items to Drupal file management') {
          // Add the file to the database.
          _auditfiles_referenced_not_used_add_to_database($file_id);
        }
        elseif ($form_state['storage']['op'] == 'Delete selected references') {
          // Delete the file from the server.
          _auditfiles_referenced_not_used_delete_record($file_id);
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
          foreach ($form_state['values']['files'] as $file_id) {
            if (!empty($file_id)) {
              $form_state['storage']['files'][$file_id] = $form_state['complete form']['files']['#options'][$file_id];
            }
          }
          $form_state['storage']['op'] = $form_state['values']['op'];
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
function _auditfiles_referenced_not_used_get_data() {
  $file_references = array();
  $files_referenced = array();
  $rows = array();
  $maximum_records = variable_get('auditfiles_report_options_maximum_records', 0);

  // Get a unique list of records from the file_usage table.
  $files_in_file_usage = db_query("SELECT DISTINCT(fid) FROM {file_usage}")->fetchAll(PDO::FETCH_COLUMN);

  // Get a list of all files that are referenced in content.
  $fields = field_info_fields();
  foreach ($fields as $field) {
    /**
     * @todo
     * Add an setting to allow the administrator the ability for spcifying the
     * field types.
     */
    if (!empty($field['type']) && ($field['type'] == 'file' || $field['type'] == 'image')) {
      // Get the database table name for the field.
      $table = key($field['storage']['details']['sql']['FIELD_LOAD_CURRENT']);
      // Get the column name in the database table for the field.
      $column = $field['storage']['details']['sql']['FIELD_LOAD_CURRENT'][$table]['fid'];
      // Get the records from the specified field table.
      $query = "SELECT * FROM {$table}";
      if ($maximum_records > 0) {
        $query .= " LIMIT $maximum_records";
      }
      $file_references = db_query($query);
      // Construct an array of records to use as the ultimate data store.
      foreach ($file_references as $file_reference) {
        $files_referenced[$file_reference->entity_id] = $file_reference;
      }

      // Remove any references that are in the file_usage table.
      foreach ($files_referenced as $file_referenced) {
        if (!empty($file_referenced->{$column}) && in_array($file_referenced->{$column}, $files_in_file_usage)) {
          unset($files_referenced[$file_referenced->entity_id]);
        }
      }

      // Assemble the rows for the table to display.
      foreach ($files_referenced as $file_referenced) {
        if (!empty($file_referenced->{$column})) {
          // If the referencing entity is a node, construct a link to it.
          if ($file_referenced->entity_type == 'node') {
            $entity_id_display = l('node/' . $file_referenced->entity_id, 'node/' . $file_referenced->entity_id);
          }
          else {
            $entity_id_display = $file_referenced->entity_id;
          }

          // Add the initial data to the row.
          $rows[$file_referenced->{$column}] = array(
            'fid' => $file_referenced->{$column},
            'entity_type' => $file_referenced->entity_type,
            'bundle' => array('data' => $file_referenced->bundle, 'hidden' => TRUE),
            'entity_id' => array('data' => $file_referenced->entity_id, 'hidden' => TRUE),
            'entity_id_display' => $entity_id_display,
            'field' => $column,
            'table' => array('data' => $table, 'hidden' => TRUE),
            'uri' => 'No file object exists for this reference.',
            'filename' => array('data' => '', 'hidden' => TRUE),
            'filemime' => '--',
            'filesize' => '--',
          );

          // If there is a file in the file_managed table, add some of that
          // information to the row, too.
          $file_managed = file_load($file_referenced->{$column});
          if (!empty($file_managed)) {
            $rows[$file_referenced->{$column}]['uri'] = $file_managed->uri;
            $rows[$file_referenced->{$column}]['filename'] = array('data' => $file_managed->filename, 'hidden' => TRUE);
            $rows[$file_referenced->{$column}]['filemime'] = $file_managed->filemime;
            $rows[$file_referenced->{$column}]['filesize'] = $file_managed->filesize;
          }

          if ($maximum_records > 0 && count($files_referenced) >= $maximum_records) {
            break;
          }
        }
      }
    }
  }

  return $rows;

  /****************************************************************************
   *
   * The following code is from the old method of looking for files in content.
   * It includes searching body fields for files referenced in HTML.
   *
   ****************************************************************************/

  // Start at 1; $files checkboxes shouldn't have a 0 key
//  $match_id = 1;
//  $files = array();
//  $single_match = 0;
//  $multi_match = 0;
//  $no_match = 0;
//  $external_domains = array();
//
//  $patterns = array(
//    // Get src attributes from img tags (assumes ")
//    '(?:<img [^>]*src[ ]*=[ ]*"([^"]*)")',
//    // Get href attributes from a tags (assumes ")
//    '(?:<a [^>]*href[ ]*=[ ]*"([^"]*)")',
//    // Get window.open argument (assumes ')
//    "(?:window.open[ ]*\([ ]*'([^\']*)')",
//  );
//  $reg_pattern = '/' . implode('|', $patterns) . '/';
//
//  // Stuff not likely to represent local file references.
//  $ignore_patterns = array(
//    'window\.open',
//    '^mailto:',
//    '\.html?($|\?)',
//    '^#',
//    '^javascript:void\(0\)',
//    '^ftp:\/\/',
//  );
//  // Add references to our own home page(s)
//  $domains = variable_get('auditfiles_include_domains', '');
//  if (!empty($domains)) {
//    $domains = explode(' ', $domains);
//    foreach ($domains as $domain) {
//      $ignore_patterns[] = "^https?:\/\/$domain\/?$";
//    }
//  }
//  $ignore_pattern = '/' . implode('|', $ignore_patterns) . '/';
//
//  // Get a list of nodes that might have a file reference in them.
//  // @TODO Please convert this statement to the D7 database API syntax.
//  $result = db_query(
//    "SELECT n.nid, n.title, b.body_value
//    FROM {node} n
//    INNER JOIN {node_revision} nr ON n.vid = nr.vid
//    INNER JOIN {field_data_body} b ON b.revision_id = nr.vid
//    WHERE b.body_value LIKE '%img%'
//       OR b.body_value LIKE '%window.open%'
//       OR b.body_value LIKE '%href%'
//    ORDER BY n.nid DESC
//    LIMIT 100"
//  );
//  // Process each node found with a potential file reference.
//  foreach ($result as $node) {
//dpm($node, '$node');
//    $node_link = l($node->title, 'node/' . $node->nid);
//
//    // Pull all the matches together.
//    preg_match_all($reg_pattern, $node->body_value, $file_match_groups);
//    $file_matches = array();
//    for ($i = 1; $i < count($patterns) + 1; $i++) {
//      $file_matches = array_merge($file_matches, $file_match_groups[$i]);
//    }
//
//    // Process each potential file reference.
//    for ($i = 0; $i < count($file_matches); $i++) {
//      $source = trim($file_matches[$i]);
//      // Don't process any empty strings created as a result of the previous
//      // merging.
//      if (!$source) {
//        continue;
//      }
//      // Don't process any strings that are likely external to this system.
//      if (preg_match($ignore_pattern, $source)) {
//        continue;
//      }
//
//      // If we've got a full URL, and the domain is not in our list of "local"
//      // domains, assume it's a valid external URL and leave it be.
//      // Search the source for a URI indicator.
//      $domain_found = preg_match('@^https?://([^/$]+)@', $source, $matches);
//      $local = FALSE;
//      if (!$domain_found) {
//        // The file reference is local to this system.
//        $local = TRUE;
//      }
//      else {
//        // If local domains have been specified, search them to see if the
//        // current string is one of them.
//        if (!empty($domains)) {
//          $source_domain = $matches[1];
//          foreach ($domains as $domain) {
//            if ($domain == $source_domain) {
//              $local = TRUE;
//              break;
//            }
//          }
//        }
//      }
//      // If the domain is not local, add it to a list of external domains and
//      // stop processing the file reference.
//      if (!$local) {
//        $external_domains[$matches[0]]++;
//        continue;
//      }
//
//      // Fix up encoded spaces.
//      $decode_source = str_replace('%20', ' ', $source);
//dpm($decode_source, '$decode_source');
//
//      $file_dir_path = drupal_realpath('public://');
//
//      // If there is an exact match on path and an exact match on that path in
//      // the {file_usage} table, we don't have to go farther.
//
//      // Search the file tables for the file reference to see if it is there.
//      // We need to compare {file_managed}.uri with $decode_source, which are in
//      // different formats. {file_managed}.uri is like "scheme://path/to/file.nam" and $decode_source may be like "http://domainname.com/
//      $query = "SELECT fu.id
//        FROM {file_usage} fu
//        INNER JOIN {file_managed} fm ON fm.fid = fu.fid
//        WHERE fu.id = :fu_nid
//        AND fm.uri = :fm_uri";
//      $hit = db_query(
//        $query,
//        array(
//          ':fu_nid' => $node->nid,
//          ':fm_uri' => $decode_source,
//        )
//      )->fetchField();
//
//      // The file reference is not exactly matched in the file tables, so
//      // include it in the list for the user to see.
//      if (!$hit) {
//        $num_matches = 0;
//        $decode_base = basename($decode_source);
//        $query = "SELECT fm.fid, fm.filename, fu.id
//          FROM {file_managed} fm
//          LEFT JOIN {file_usage} fu ON fm.fid = fu.fid
//          WHERE fu.id = :fu_id
//          AND fm.filename = :fm_filename";
//        $file_result = db_query(
//          $query,
//          array(
//            ':fu_id' => $node->nid,
//            ':fm_filename' => $decode_base,
//          )
//        );
//
//        while ($file = $file_result->fetch()) {
//          $num_matches++;
//          // Visible fields first
//          $form['titles'][$match_id] = array('#value' => $node_link);
//          $form['sources'][$match_id] = array('#value' => $source);
//          $form['fids'][$match_id] = array('#value' => $file->fid);
//
//          // Strip the Drupal file path, and make the link
//          $filepath = preg_replace('@^' . preg_quote($file_dir_path) . '/@', '', $file->filepath);
//          $filelink = l($filepath, $GLOBALS['base_url'] . '/' . $file_dir_path . '/' . str_replace('\\', '/', $filepath));
//          $form['paths'][$match_id] = array('#value' => $filelink);
//          $files[$match_id] = '';
//
//          // Fields passed through form submission
//          $form['nid_values'][$match_id] = array(
//            '#type' => 'value',
//            '#value' => $node->nid,
//          );
//          $form['source_values'][$match_id] = array(
//            '#type' => 'value',
//            '#value' => $source,
//          );
//          $form['fid_values'][$match_id] = array(
//            '#type' => 'value',
//            '#value' => $file->fid,
//          );
//          $form['path_values'][$match_id] = array(
//            '#type' => 'value',
//            '#value' => $file->filepath,
//          );
//
//          if (!$file->nid) {
//            $form['no_upload'][$match_id] = array(
//              '#type' => 'value',
//              '#value' => TRUE,
//            );
//          }
//
//          $match_id++;
//        }
//dpm($num_matches, '$num_matches');
//
//        if ($num_matches == 0) {
//          $no_match++;
//
//          $form['titles'][$match_id] = array('#value' => $node_link);
//          $form['sources'][$match_id] = array('#value' => $source);
//          $form['fids'][$match_id] = array('#value' => '');
//          $form['paths'][$match_id] = array('#value' => '');
//
//          $files[$match_id] = '';
//
//          $form['nid_values'][$match_id] = array(
//            '#type' => 'value',
//            '#value' => $node->nid,
//          );
//          $form['source_values'][$match_id] = array(
//            '#type' => 'value',
//            '#value' => $source,
//          );
//          $form['no_upload'][$match_id] = array(
//            '#type' => 'value',
//            '#value' => TRUE,
//          );
//
//          $match_id++;
//        }
//        elseif ($num_matches == 1) {
//          $single_match++;
//
//          $form['unique'][$match_id - 1] = array(
//            '#type' => 'value',
//            '#value' => TRUE,
//          );
//        }
//        else {
//          $multi_match++;
//        }
//      }
//    }
//  }
//dpm($single_match, '$single_match');
//dpm($multi_match, '$multi_match');
//dpm($no_match, '$no_match');
//
//  if (count($external_domains) > 0) {
//    $form['external_domains'] = array(
//      '#type' => 'fieldset',
//      '#title' => 'External domains referenced',
//      '#collapsible' => TRUE,
//      '#collapsed' => TRUE,
//    );
//
//    arsort($external_domains);
//
//    foreach ($external_domains as $domain => $count) {
//      $form['domains'][$domain] = array('#value' => $domain);
//      $form['domain_counts'][$domain] = array('#value' => $count);
//    }
//  }
//
//  // Remember, we started at 1 instead of 0...
//  if ($match_id > 1) {
//    $broken = $no_match + $single_match + $multi_match;
//    $total = format_plural($broken, '1 broken reference found:', '@count broken references found:');
//    $unmatched = "$no_match with no matches";
//    $unique_match = "$single_match with a unique match";
//    $multi_matches = "$multi_match with multiple matches";
//    $form['count'] = array(
//      '#value' => "<div>$total<br />$unmatched<br />$unique_match<br />$multi_matches</div>",
//    );
//  }
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
function _auditfiles_referenced_not_used_confirm_operation($form, &$form_state) {
  $values = $form_state['input'];
  $files = $form_state['storage']['files'];

  $form['changelist'] = array(
    '#prefix' => '<ul>',
    '#suffix' => '</ul>',
    '#tree' => TRUE,
  );

  // Prepare the list of items to present to the user.
  if (!empty($values['files'])) {
dpm($values['files'], '$values[files]');
    foreach ($values['files'] as $file_id) {
      if (!empty($file_id)) {
        $file_info = $files[$file_id];
        $file_object = file_load($file_id);

        $form['changelist'][$file_id] = array(
          '#type' => 'hidden',
          '#value' => $file_id,
          '#suffix' => "</li>\n",
        );

        if (empty($file_object)) {
          if ($values['op'] == 'Add selected items to Drupal file management') {
            $form['changelist'][$file_id]['#prefix'] = '<li>' . t(
              'Field "%field" in ID "%id" of bundle "%bundle" of entity "%entity" references file ID "%file_id" that is for a non-existant file object, and therefore, it cannot be added using this method.',
              array(
                '%field' => $file_info['field'],
                '%id' => $file_info['entity_id']['data'],
                '%bundle' => $file_info['bundle']['data'],
                '%entity' => $file_info['entity_type'],
                '%file_id' => $file_id,
              )
            );
            unset($form_state['input']['files'][$file_id]);
            unset($form_state['storage']['files'][$file_id]);
            unset($form_state['values']['files'][$file_id]);
        }
          else {
            $form['changelist'][$file_id]['#prefix'] = '<li>' . t(
              'The reference in field "%field" to the non-existant file object with ID "%file_id" will be deleted.',
              array(
                '%field' => $file_info['field'],
                '%file_id' => $file_id,
              )
            );
          }
        }
        else {
          if ($values['op'] == 'Add selected items to Drupal file management') {
            $form['changelist'][$file_id]['#prefix'] = '<li>' . t(
              'A file_usage record for the content referencing "%filename" will be added to the database.',
              array('%filename' => $file_info['filename']['data'])
            );
          }
          elseif ($values['op'] == 'Delete selected references') {
            $form['changelist'][$file_id]['#prefix'] = '<li>' . t(
              'The reference to "%filename" will be deleted from the database.',
              array('%filename' => $file_info['filename']['data'])
            );
          }
        }
      }
      else {
        // Unsetting the unprocessed files prevents confirm_submit from dealing
        // with them.
        unset($form_state['input']['files'][$file_id]);
      }
    }
  }

  // Tell the submit handler to process the confirmation.
  $form['process'] = array(
    '#type' => 'hidden',
    '#value' => 'TRUE'
  );
  // Go back to the main form, when done with this one.
  $form['destination'] = array(
    '#type' => 'hidden',
    '#value' => 'admin/reports/auditfiles/referencednotused'
  );

  $form['operation'] = array(
    '#type' => 'hidden',
  );

  if ($values['op'] == 'Add selected items to Drupal file management') {
    $form['operation']['#value'] = 'add';
    $confirm_question = t('Add these files to the database?');
    $confirm_description = '';
  }
  elseif ($values['op'] == 'Delete selected references') {
    $form['operation']['#value'] = 'delete';
    $confirm_question = t('Delete these files from the server?');
    $confirm_description = '<strong>' . t('This action cannot be undone.') . '</strong>';
  }

dpm($form_state, '$form_state');
  return confirm_form(
    $form,
    $confirm_question,
    'admin/reports/auditfiles/referencednotused',
    $confirm_description,
    t('Yes'),
    t('No')
  );
}

/**
 * Adds the specified file to the database.
 *
 * @param array $file_info
 *   Information about the file to be added.
 */
function _auditfiles_referenced_not_used_add_to_database($file_info) {
  $data = array(
    'fid' => $file_info['fid'],
    /**
     * @todo
     * This is hard coded for now, but need to determine how to figure out which
     * module needs to be here.
     */
    'module' => 'file',
    'type' => $file_info['entity_type'],
    'id' => $file_info['entity_id']['data'],
    'count' => 1,
  );
  $results = db_insert('file_usage')
    ->fields($data)
    ->execute();
}

/**
 * Deletes a specified file from the database.
 *
 * @param integer $file_id
 *   The ID of the file to delete from the database.
 */
function _auditfiles_referenced_not_used_delete_record($file_info) {
  $result = db_delete($file_info['table']['data'])
    ->condition($file_info['field'], $file_info['fid'])
    ->execute();
  if ($result == 0) {
    drupal_set_message(
      t(
        'There was a problem deleting the reference to file ID %fid in field %field. Check the logs for more information.',
        array(
          '%fid' => $file_info['fid'],
          '%field' => $file_info['field'],
        )
      ),
      'warning'
    );
  }
}
