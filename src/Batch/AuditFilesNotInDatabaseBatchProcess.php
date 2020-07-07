<?php

namespace Drupal\auditfiles\Batch;

use Drupal\Component\Utility\Html;
use Drupal\Core\StringTranslation\TranslatableMarkup;
use Drupal\auditfiles\ServiceAuditFilesNotInDatabase;

/**
 * Process batch files.
 */
class AuditFilesNotInDatabaseBatchProcess {

  /**
   * The file name to process.
   *
   * @var string
   */
  protected $fileName;

  /**
   * ReferencedNotUsed service.
   *
   * @var \Drupal\auditfiles\ServiceAuditFilesNotInDatabase
   */
  protected $notInDatabase;

  /**
   *  Class constructor.
   *
   * @param \Drupal\auditfiles\ServiceAuditFilesNotInDatabase $not_in_database
   *   Injected ServiceAuditFilesUsedNotManaged service.
   * @param string $file_name
   *   File name to process.
   */
  public function __construct(ServiceAuditFilesNotInDatabase $not_in_database, $file_name) {
    $this->notInDatabase = $not_in_database;
    $this->fileName = $file_name;
  }

  /**
   * The batch process for adding the file.
   *
   * @param string $filename
   *   File name that to be process.
   * @param array $context
   *   Batch context.
   */
  public static function auditfilesNotInDatabaseBatchAddProcessBatch($filename, array &$context) {
    $notInDatabase = \Drupal::service('auditfiles.not_in_database');
    $worker = new static($notInDatabase, $filename);
    $worker->addDispatch($context);
  }

  /**
   * Adds filenames referenced in content in file_managed but not in file_usage.
   *
   * @param string $filename
   *   File entity ID to delete.
   * @param array $context
   *   Batch context.
   */
  protected function addDispatch(&$context) {
    $this->notInDatabase->auditfilesNotInDatabaseBatchAddProcessFile($filename);
    $context['results'][] = Html::escape($filename);
    $context['message'] = new TranslatableMarkup('Processed %filename.', ['%filename' => $filename]);
  }

  /**
   * The batch process for deleting the file.
   *
   * @param string $filename
   *   File name that to be process.
   * @param array $context
   *   Batch context.
   */
  public static function auditfilesNotInDatabaseBatchDeleteProcessBatch($filename, array &$context) {
    $notInDatabase = \Drupal::service('auditfiles.not_in_database');
    $worker = new static($notInDatabase, $filename);
    $worker->deleteDispatch($context);
  }

  /**
   * Deletes filenames referenced in content frm file_managed not in file_usage.
   *
   * @param int $reference_id
   *   File entity ID to delete.
   * @param array $context
   *   Batch context.
   */
  protected function deleteDispatch(&$context) {
    $this->notInDatabase->auditfilesNotInDatabaseBatchDeleteProcessFile($filename);
    $context['results'][] = Html::escape($filename);
    $context['message'] = new TranslatableMarkup('Processed %filename.', ['%filename' => $filename]);
  }

}
