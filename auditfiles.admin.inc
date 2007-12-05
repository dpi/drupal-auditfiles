<?php
// $Id$

/**
 * @file
 * Callback for settings page
 */
function auditfiles_admin_settings() {

    $form['auditfiles_exclude_files'] = array(
      '#type' => 'textfield',
      '#title' => t('Exclude these files'),
      '#default_value' => trim(variable_get('auditfiles_exclude_files', '.htaccess')),
      '#description' => t('Enter a list of files to exclude, separated by spaces.'),
    );

    $form['auditfiles_exclude_extensions'] = array(
      '#type' => 'textfield',
      '#title' => t('Exclude these extensions'),
      '#default_value' => trim(variable_get('auditfiles_exclude_extensions', '')),
      '#description' => t('Enter a list of extensions to exclude, separated by spaces. Do not include the leading dot.'),
    );

    $form['auditfiles_exclude_paths'] = array(
      '#type' => 'textfield',
      '#title' => t('Exclude these paths'),
      '#default_value' => trim(variable_get('auditfiles_exclude_paths', 'color')),
      '#description' => t('Enter a list of paths to exclude, separated by spaces. Do not include the leading slash. Paths are relative to %directorypath', array('%directorypath' => file_directory_path())),
    );

  return system_settings_form($form);
}
