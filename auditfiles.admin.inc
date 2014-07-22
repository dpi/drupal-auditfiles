<?php

/**
 * @file
 * Callback for settings page
 */
function auditfiles_admin_settings($form, &$form_state) {

  $form['exclusions'] = array(
    '#type' => 'fieldset',
    '#title' => t('Exclusions'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );
  $form['exclusions']['auditfiles_exclude_files'] = array(
    '#type' => 'textfield',
    '#title' => t('Exclude these files'),
    '#default_value' => trim(variable_get('auditfiles_exclude_files', '.htaccess')),
    '#description' => t('Enter a list of files to exclude, separated by spaces.'),
  );

  $form['exclusions']['auditfiles_exclude_extensions'] = array(
    '#type' => 'textfield',
    '#title' => t('Exclude these extensions'),
    '#default_value' => trim(variable_get('auditfiles_exclude_extensions', '')),
    '#description' => t('Enter a list of extensions to exclude, separated by spaces. Do not
      include the leading dot.'),
  );

  $form['exclusions']['auditfiles_exclude_paths'] = array(
    '#type' => 'textfield',
    '#title' => t('Exclude these paths'),
    '#default_value' => trim(variable_get('auditfiles_exclude_paths', 'color')),
    '#description' => t('Enter a list of paths to exclude, separated by spaces. Do not
      include the leading slash. Paths are relative to %directorypath',
      array('%directorypath' => file_default_scheme())),
  );

  $form['domains'] = array(
    '#type' => 'fieldset',
    '#title' => t('Domains'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );

  $form['domains']['auditfiles_include_domains'] = array(
    '#type' => 'textfield',
    '#title' => t('Include references to these domains'),
    '#default_value' => trim(variable_get('auditfiles_include_domains', '')),
    '#size' => 80,
    '#maxlength' => 1024,
    '#description' => t('Enter a list of domains (e.g., www.example.com) pointing
      to your website, separated by spaces. When scanning content for file references (such as &lt;img&gt;
      tags), any absolute references using these domains will be included and rewritten
      to use relative references. Absolute references to domains not in this list will
      be considered to be external references and will not be audited or rewritten.'),
  );

  return system_settings_form($form);
}
