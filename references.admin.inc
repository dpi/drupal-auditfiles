<?php

/**
 * @file
 * Callback and functions to generate references report
 */


/**
 * Menu callback: audit files referenced but not attached via uploads
 */
function auditfiles_references() {
  return drupal_get_form('auditfiles_references_form');
}

/**
 * Form definition for audit files not in database
 */
function auditfiles_references_form(&$form_state) {
//  drupal_set_message("form: form_state: ".dpr($form_state,TRUE));
  if (isset($form_state['storage']['confirm'])) {
    return auditfiles_references_form_confirm($form_state);
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
  $result = db_query("SELECT n.nid,n.title,nr.body
                      FROM {node} n
                      INNER JOIN {node_revisions} nr ON n.vid=nr.vid
                      where nr.body like '%img%' or nr.body like '%window.open%'
                        or nr.body like '%href%'
                      ORDER BY n.nid");

  // Start at 1 - $files checkboxes shouldn't have a 0 key
  $matchid = 1;
  $files = array();
  $singlematch = 0;
  $multmatch = 0;
  $nomatch = 0;
  $filedirpath = file_directory_path();
  $domains = explode(' ', variable_get('auditfiles_include_domains', ''));
  $externaldomains = array();
  
  $patterns = array(
    // Get src attributes from img tags (assumes ")
    '(?:<img [^>]*src[ ]*=[ ]*"([^"]*)")',
    // Get href attributes from a tags (assumes ")
    '(?:<a [^>]*href[ ]*=[ ]*"([^"]*)")',
    // Get window.open argument (assumes ')
    "(?:window.open[ ]*\([ ]*'([^\']*)')",
  );
  $regpatt = '/'.implode('|', $patterns).'/';
  
  // Stuff not likely to represent local file references
  $ignorepatterns = array(
    'window\.open',
    '^mailto:',
    '\.html?($|\?)', 
    '^#',
    '^javascript:void\(0\)',
    '^ftp:\/\/',
  );
  // Add references to our own home page(s)
  foreach ($domains as $domain) {
    $ignorepatterns[] = "^https?:\/\/$domain\/?$";
  }
  $ignorepatt = '/'.implode('|', $ignorepatterns).'/';

  while ($node = db_fetch_object($result)) {
    $nodelink = l($node->title, 'node/'.$node->nid);

    $imgsmatched = preg_match_all($regpatt, $node->body, $imgmatchgroups);

    // Pull all matches together
    $imgmatches = array();
    for ($i = 1; $i < count($patterns)+1; $i++) {
      $imgmatches = array_merge($imgmatches, $imgmatchgroups[$i]);
    }

    for ($i=0; $i < count($imgmatches); $i++) {
      $src = trim($imgmatches[$i]);
      // Merge gives us some blanks
      if (!$src) continue;
      // Bunch of stuff that won't get us anywhere
      if (preg_match($ignorepatt, $src)) continue;
      
      // If we've got a full URL, and the domain is not in our list of "local" 
      // domains, assume it's a valid external URL and leave it be
      $domainfound = preg_match('&^https?://([^/$]+)&', $src, $matches);
      if (!$domainfound) {
        $local = TRUE;
      } else {
        $local = FALSE;
        $srcdomain = $matches[1];
        foreach ($domains as $domain) {
          if ($domain == $srcdomain) {
            $local = TRUE;
            break;
          }
        }
      }
      if (!$local) {
        $externaldomains[$srcdomain]++;
        continue;
      }

      // Fix up encoded spaces
      $decodesrc = str_replace('%20', ' ', $src);

      // If we've got an exact match on path, and have an exact match on that
      // path in the {upload} table, we don't have to go farther (note how we
      // skip the / in the src)
      $hit = db_result(db_query("SELECT u.nid
                                 FROM {files} f
                                 INNER JOIN {upload} u ON f.fid=u.fid 
                                 WHERE u.nid=%d AND CONCAT('/', f.filepath)='%s'",
                                $node->nid, $decodesrc));
                                
      if (!$hit) {
        $basename = basename($src);
        $decodebase = basename($decodesrc);
        $fileresult = db_query("SELECT f.fid,f.filepath,u.nid
                                FROM {files} f
                                LEFT JOIN {upload} u ON f.fid=u.fid AND u.nid=%d
                                WHERE f.filename='%s'",
                               $node->nid, $decodebase);
        $nummatches = 0;
        while ($file = db_fetch_object($fileresult)) {
          $nummatches++;
          // Visible fields first
          $form['titles'][$matchid] = array('#value' => $nodelink);
          $form['srcs'][$matchid] = array('#value' => $src);
          $form['fids'][$matchid] = array('#value' => $file->fid);
          // Strip the Drupal file path, and make the link
          $filepath = preg_replace('@^'.preg_quote($filedirpath).'/@', '', $file->filepath);
          $filelink = l($filepath, $GLOBALS['base_url'] .'/'. $filedirpath .'/'. 
            str_replace('\\', '/', $filepath));
          $form['paths'][$matchid] = array('#value' => $filelink);
          $files[$matchid] = '';
          // Fields passed through form submission
          $form['nidvals'][$matchid] = array('#type' => 'value', '#value' => $node->nid);
          $form['srcvals'][$matchid] = array('#type' => 'value', '#value' => $src);
          $form['fidvals'][$matchid] = array('#type' => 'value', '#value' => $file->fid);
          $form['pathvals'][$matchid] = array('#type' => 'value', '#value' => $file->filepath);
          if (!$file->nid) {
            $form['noupload'][$matchid] = array('#type' => 'value', '#value' => TRUE);
          }
          $matchid++;
        }
        if ($nummatches == 0) {
          $nomatch++;
          $form['titles'][$matchid] = array('#value' => $nodelink);
          $form['srcs'][$matchid] = array('#value' => $src);
          $form['fids'][$matchid] = array('#value' => '');
          $form['paths'][$matchid] = array('#value' => '');
          $files[$matchid] = '';
          $form['nidvals'][$matchid] = array('#type' => 'value', '#value' => $node->nid);
          $form['srcvals'][$matchid] = array('#type' => 'value', '#value' => $src);
          $form['noupload'][$matchid] = array('#type' => 'value', '#value' => TRUE);
          $matchid++;
        } elseif ($nummatches == 1) {
          $singlematch++;
          $form['unique'][$matchid-1] = array('#type' => 'value', '#value' => TRUE);
        } else {
          $multmatch++;
        }
      }
    }
  }

  if (count($externaldomains) > 0) {
    $form['externaldomains'] = array(
      '#type' => 'fieldset',
      '#title' => 'External domains referenced',
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );

    arsort($externaldomains);

    foreach ($externaldomains as $domain => $count) {
      $form['domains'][$domain] = array('#value' => $domain);
      $form['domaincounts'][$domain] = array('#value' => $count);
    }
  }
  
  // Remember, we started at 1 instead of 0...
  if ($matchid > 1) {
    $broken = $nomatch + $singlematch + $multmatch;
    $total = format_plural($broken, '1 broken reference found:', '@count broken references found:');
    $unmatched = "$nomatch with no matches";
    $uniquematch = "$singlematch with a unique match";
    $multmatches = "$multmatch with multiple matches";
    $form['count'] = array(
      '#value' => "<div>$total<br />$unmatched<br />$uniquematch<br />$multmatches</div>",
    );
  }
  else {
    $form['count'] = array(
      '#value' => t('No broken references found.'),
    );
  }
  
  // Add list of files to checkboxes
  $form['files'] = array('#type' => 'checkboxes', '#options' => $files);
  
  // Maintains hidden fields
  $form['#tree'] = TRUE;

  // Return form
  return $form;
}

/**
 * Theme auditfiles_references_form
 */
function theme_auditfiles_references_form($form) {
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
      $row[] = drupal_render($form['domaincounts'][$key]);
      $rows[] = $row;
    }
    
    // Render themed table
    $tableoutput = theme('table', $header, $rows);
    $form['externaldomains']['list'] = array('#value' => $tableoutput);
    $output .= drupal_render($form['externaldomains']);
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
      $row[] = drupal_render($form['srcs'][$key]);
      $row[] = drupal_render($form['fids'][$key]);
      $row[] = drupal_render($form['paths'][$key]);
      $rows[] = $row;
    }
    
    // Render themed table
    $output .= theme('table', $header, $rows);
  }
  $output .= drupal_render($form);
  // Return output
  return $output;
}

function auditfiles_references_form_submit($form, &$form_state) {
//  drupal_set_message("submit form_state: ".dpr($form_state, TRUE));
  if ($form_state['clicked_button']['#id'] == 'edit-options-submit' &&
      $form_state['values']['options']['operation'] <> 'donothing') {
    $form_state['storage']['confirm'] = TRUE;
    $form_state['storage']['values'] = $form_state['values'];
  }
}

function auditfiles_references_form_confirm(&$form_state) {
//  drupal_set_message("form_confirm: values: ".dpr($values, TRUE));
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
  
  $form['changelist'] = array('#prefix' => '<ul>', '#suffix' => '</ul>', '#tree' => TRUE);

  // Note we iterate over fidvals, since no operation makes sense without a valid file
  $count = 0;
  foreach ($values['fidvals'] as $id => $value) {
    if (($seltype == 'unique' && $values['unique'][$id]) ||
        ($seltype == 'selected' && $values['files'][$id] == $id)) {
      $count++;
      // Limit what's displayed (also helps avoid max_allowed_packet errors)
      if ($count == 50) {
        $message = '...and many more...';
      } elseif ($count < 50) {
        if ($optype == 'delete') {
          $message = "Deleting file <strong>".$values['pathvals'][$id].'</strong>';
        } else {
          $message = 'Replacing src <strong>'.$values['srcvals'][$id].'</strong> with <strong>'.
            $values['pathvals'][$id].'</strong> for node rev <strong>'.$values['nidvals'][$id].
            '</strong>';
        }
      }
      if ($message) {
        $form['changelist'][$id] = array('#type' => 'hidden', '#value' => $message, 
          '#prefix' => '<li>', '#suffix' => $message ."</li>\n");
        unset($message);
      }
    } else {
      // Unsetting the unprocessed fidvals prevents confirm_submit from dealing with them
      unset($form_state['storage']['values']['fidvals'][$id]);
    }
  }
  $form['operation'] = array('#type' => 'hidden', '#value' => $operation);
  $form['#submit'][] = 'auditfiles_references_form_confirm_submit';

  return confirm_form(
    $form,
    t('Are you sure you want to make these changes?'),
    'admin/reports/auditfiles/references',
    '<strong>'. t('This action cannot be undone.') .'</strong>',
    t('Process all'),
    t('Cancel')
  );
}

function auditfiles_references_form_confirm_submit($form, &$form_state) {
//  drupal_set_message('confirm_submit, form: '.dpr($form, TRUE));
//  drupal_set_message('confirm_submit, form_state: '.dpr($form_state, TRUE));
  if ($form_state['values']['confirm']) {
    $values = $form_state['storage']['values'];
    //drupal_set_message("in confirm_submit, values: ".dpr($values,TRUE));
    foreach ($values['fidvals'] as $id => $fid) {
      if ($values['options']['operation'] == 'delete') {
        db_query("DELETE FROM {files}
                  WHERE fid=%d",
                 $fid);
        if (file_delete(file_create_path($values['pathvals'][$id]))) {
          drupal_set_message(t('%file was deleted', array('%file' => $values['pathvals'][$id])));
        }
        else {
          drupal_set_message(t('Failed to delete %file', array('%file' => $values['pathvals'][$id])));
        }
      } else {
        // Load $values['nidvals'][$id], replace srcvals with pathvals, save
        // If necessary, write record to {upload} table
        $newpath = '/'.$values['pathvals'][$id];
        $oldpath = $values['srcvals'][$id];

        if ($newpath != $oldpath) {
          $row = db_fetch_object(db_query("SELECT nr.body,nr.vid
                                      FROM {node_revisions} nr
                                      INNER JOIN {node} n ON nr.vid=n.vid
                                      WHERE n.nid = %d",
                                     $values['nidvals'][$id]));
          $body = str_replace($oldpath, $newpath, $row->body);
          db_query("UPDATE {node_revisions}
                    SET body='%s'
                    WHERE vid=%d",
                   $body, $row->vid);
        }
        if ($values['noupload'][$id]) {
          db_query("INSERT INTO {upload}
                    (fid, nid, vid, description, list, weight)
                    SELECT %d, n.nid, n.vid, 'Attached by auditfiles', 0, 0
                    FROM {node} n
                    WHERE n.nid=%d",
                   $fid, $values['nidvals'][$id]);
        }
        drupal_set_message("Updated path $oldpath to $newpath for rev ".$values['nidvals'][$id]);
      }
    }
    // Clear so our return to the primary form doesn't think we're going to the confirmation form
    unset($form_state['storage']['confirm']);
  }
}
