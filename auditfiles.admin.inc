<?php
/**
 * @file
 * This is where the administrative settings configuration is.
 */

/**
 * @file
 * Callback for settings page
 */
function auditfiles_admin_settings($form, &$form_state) {
  $form['auditfiles_exclusions'] = array(
    '#type' => 'fieldset',
    '#title' => t('Exclusions'),
    '#collapsible' => TRUE,
  );
  $form['auditfiles_exclusions']['auditfiles_exclude_files'] = array(
    '#type' => 'textfield',
    '#title' => t('Exclude these files'),
    '#default_value' => trim(variable_get('auditfiles_exclude_files', '.htaccess')),
    '#description' => t('Enter a list of files to exclude, separated by spaces.'),
  );
  $form['auditfiles_exclusions']['auditfiles_exclude_extensions'] = array(
    '#type' => 'textfield',
    '#title' => t('Exclude these extensions'),
    '#default_value' => trim(variable_get('auditfiles_exclude_extensions', '')),
    '#description' => t('Enter a list of extensions to exclude, separated by spaces. Do not
      include the leading dot.'),
  );
  $form['auditfiles_exclusions']['auditfiles_exclude_paths'] = array(
    '#type' => 'textfield',
    '#title' => t('Exclude these paths'),
    '#default_value' => trim(variable_get('auditfiles_exclude_paths', 'color css ctools js')),
    '#description' => t('Enter a list of paths to exclude, separated by spaces. Do not
      include the leading slash. Paths are relative to %directorypath',
      array('%directorypath' => file_default_scheme())),
  );

  $form['auditfiles_domains'] = array(
    '#type' => 'fieldset',
    '#title' => t('Domains'),
    '#collapsible' => TRUE,
  );
  $form['auditfiles_domains']['auditfiles_include_domains'] = array(
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

  $form['auditfiles_report_options'] = array(
    '#type' => 'fieldset',
    '#title' => t('Report options'),
    '#collapsible' => TRUE,
  );
  $form['auditfiles_report_options']['auditfiles_report_options_items_per_page'] = array(
    '#type' => 'textfield',
    '#title' => t('Number of items per page'),
    '#default_value' => variable_get('auditfiles_report_options_items_per_page', 50),
    '#size' => 10,
    '#description' => t('Enter an integer representing the number of items to display on each page of a report. If there are more than this number on a page, then a pager will be used to display the additional items. Enter a 0 to show all items on a single page.'),
  );
  // Show date format select list.
  $date_types = system_get_date_types();
  $options = array();
  foreach ($date_types as $date_type) {
    $options[$date_type['type']] = $date_type['title'];
  }
  $form['auditfiles_report_options']['auditfiles_report_options_date_format'] = array(
    '#type' => 'select',
    '#title' => 'Date format',
    '#default_value' => variable_get('auditfiles_report_options_date_format', 'long'),
    '#options' => $options,
    '#description' => t('Select the date format to use when displaying file dates in the reports. Date types and formats can be modified in the system !datetime settings pages.', array('!datetime' => l('Date and time', 'admin/config/regional/date-time'))),
  );

  return system_settings_form($form);
}
