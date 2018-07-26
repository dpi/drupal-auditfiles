<?php

namespace Drupal\auditfiles\Form;

use Drupal\Core\Form\ConfirmFormInterface;
use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\StringTranslation\StringTranslationTrait;
use Drupal\Core\StringTranslation\TranslationInterface;
use Drupal\Core\Form\ConfirmFormHelper;
use Drupal\Core\Url;
use Drupal\Core\Link;

class AuditFilesNotInDatabase extends FormBase implements ConfirmFormInterface {
  
  /**
   * Widget Id.
   */
  public function getFormId() {
    return 'notindatabase';
  }

  /**
   * {@inheritdoc}
   */
  public function getDescription() {
    return $this->t('This action cannot be undone.');
  }

  /**
   * {@inheritdoc}
   */
  public function getConfirmText() {
    return $this->t('Confirm');
  }

  /**
   * {@inheritdoc}
   */
  public function getCancelText() {
    return $this->t('Cancel');
  }

  /**
   * {@inheritdoc}
   */
  public function getFormName() {
    return 'notInDatabase';
  }
  /**
   * {@inheritdoc}
   */
  public function getCancelUrl() {
    return new Url('auditfiles.notindatabase');
  }

  /**
   * {@inheritdoc}
   */
  public function getQuestion() {
    return $this->t("Do you wan't to delete following record");
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state) {
    $storage = &$form_state->getStorage();
    if(isset($storage['confirm'])) {
      $values = $form_state->getValue('files');
      $form['changelist'] = [
        '#prefix' => '<ul>',
        '#suffix' => '</ul>',
        '#tree' => TRUE,
      ];
      // Prepare the list of items to present to the user.
      if (!empty($values)) {
        foreach ($values as $filename) {
          if (!empty($filename)) {
            if ($storage['op'] == 'add') {
              $message = $this->t('will be added to the database.');
            }
            elseif ($storage['op'] == 'delete') {
              $message = $this->t('will be deleted from the server.');
            }
            $form['changelist'][$filename] = [
              '#type' => 'hidden',
              '#value' => $filename,
              '#prefix' => '<li><strong>' . $filename . '</strong> ' . $message,
              '#suffix' => "</li>\n",
            ];
          }
          else {
            unset($form_state->getValue('files')[$filename]);
          }
        }
      }
      if ($storage['op'] == 'add') {
        $form['#title'] = $this->t('Add these files to the database?');
      }
      elseif ($storage['op'] == 'delete') {
        $form['#title'] = $this->t('Delete these files from the server?');
      }
      $form['#attributes']['class'][] = 'confirmation';
      $form['actions'] = [
        '#type' => 'actions',
      ];
      $form['actions']['submit'] = [
        '#type' => 'submit',
        '#value' => $this->getConfirmText(),
        '#button_type' => 'primary',
        '#submit' => ['::confirmSubmissionHandler']
      ];
      $form['actions']['cancel'] = ConfirmFormHelper::buildCancelLink($this, $this->getRequest());
      // By default, render the form using theme_confirm_form().
      if (!isset($form['#theme'])) {
        $form['#theme'] = 'confirm_form';
      }
    return $form;
    }
    $config = \Drupal::config('auditfiles_config.settings');
    //Get the records to display.
    //Check to see if there is saved data, and if so, use that.
    $rows = \Drupal::service('auditfiles.not_in_database')->_auditfiles_not_in_database_get_reports_files();
    if (!empty($rows)) {
      // Set up the pager.
      $items_per_page = $config->get('auditfiles_report_options_items_per_page') ? $config->get('auditfiles_report_options_items_per_page') : 50;
      if (!empty($items_per_page)) {
        $current_page = pager_default_initialize(count($rows), $items_per_page);
        // Break the total data set into page sized chunks.
        $pages = array_chunk($rows, $items_per_page, TRUE);
      }
    }
    // Define the form.
    // Setup the record count and related messages.
    $maximum_records =  $config->get('auditfiles_report_options_maximum_records') ? $config->get('auditfiles_report_options_maximum_records') : 250;
   // $form_count = '';
    if (!empty($rows)) {
      if ($maximum_records > 0) {
        $file_count_message = $this->t('Found at least @count files on the server that are not in the database.');
      }
      else {
        $file_count_message = $this->t('Found @count files on the server that are not in the database.');
      }
      $form_count = $this->formatPlural(count($rows), 'Found 1 file on the server that is not in the database.', $file_count_message);
    }
    else {
      $form_count = $this->t('Found no files on the server that are not in the database.');
    }
    // Create the form table.
    $form['files'] = [
      '#type' => 'tableselect',
      '#header' => \Drupal::service('auditfiles.not_in_database')->_auditfiles_not_in_database_get_header(),
      '#empty' => $this->t('No items found.'),
      '#prefix' => '<div><em>' . $form_count . '</em></div>',
    ];

    // Add the data.
    if (!empty($rows) && !empty($pages)) {
      $form['files']['#options'] = $pages[$current_page];
    }
    elseif (!empty($rows)) {
      $form['files']['#options'] = $rows;
    }
    else {
      $form['files']['#options'] = [];
    }

    if (!empty($rows)) {
      $form['actions'] = ['#type' => 'actions'];
      $form['actions']['add'] = [
        '#type' => 'submit',
        '#value' => $this->t('Add selected items to the database'),
        '#submit' => ['::submissionHandlerAddRecord'],
      ];
      $form['actions']['markup'] = [
        '#markup' => '&nbsp;' . $this->t('or') . '&nbsp;',
      ];
      $form['actions']['delete'] = [
        '#type' => 'submit',
        '#value' => $this->t('Delete selected items from the server'),
        '#submit' => ['::submissionHandlerDeleteRecord'],
      ];
      $form['pager'] = ['#type' => 'pager'];
    }
  return $form;
  }

  /**
   * Submit form.
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {

  }

  /**
   * Add record to database.
   */
  public function submissionHandlerAddRecord(array &$form, FormStateInterface $form_state) {
    if (!empty($form_state->getValue('files'))) {
      foreach ($form_state->getValue('files') as $file_id) {
        if (!empty($file_id)) {
          $storage = [
            'files' => $form_state->getValue('files'),
            'op' => 'add',
            'confirm' => TRUE
          ];
          $form_state->setStorage($storage);      
          $form_state->setRebuild();
        }
      } 
      if(!isset($storage)) {
        drupal_set_message($this->t('No items were selected to operate Add.'),'error');  
      }
    }
  }

  /**
   * Delete record from files.
   */
  public function submissionHandlerDeleteRecord(array &$form, FormStateInterface $form_state) {
    if (!empty($form_state->getValue('files'))) {
      foreach ($form_state->getValue('files') as $file_id) {
        if (!empty($file_id)) {
          $storage = [
            'files' => $form_state->getValue('files'),
            'op' => 'delete',
            'confirm' => TRUE
          ];
          $form_state->setStorage($storage);      
          $form_state->setRebuild();
        }
      }
      if (!isset($storage)) {
        drupal_set_message($this->t('No items were selected to operate Delete.'), 'error');  
      }
    }
  }

  /**
   * Delete record from files.
   */
  public function confirmSubmissionHandler(array &$form, FormStateInterface $form_state) {
    $storage = &$form_state->getStorage();
    if ($storage['op'] == 'add') {   
      batch_set(\Drupal::service('auditfiles.not_in_database')->_auditfiles_not_in_database_batch_add_create_batch($form_state->getValue('changelist')));
    }
    else {
      batch_set(\Drupal::service('auditfiles.not_in_database')->_auditfiles_not_in_database_batch_delete_create_batch($form_state->getValue('changelist')));
    }
  }

}