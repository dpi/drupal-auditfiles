<?php

namespace Drupal\auditfiles\Batch;

use Drupal\Component\Utility\Html;
use Drupal\Core\StringTranslation\TranslatableMarkup;
use Drupal\auditfiles\ServiceAuditFilesUsedNotManaged;

/**
 * Batch Worker to remove files from file_usage not in file_managed table.
 *
 * @todo Refactor to make a Factory Worker class.
 */
class AuditFilesUsedNotManagedBatchProcess {

  /**
   * The File entity ID to delete.
   *
   * @var int
   */
  protected $fileId;

  /**
   * ManagedNotUsed service.
   *
   * @var \Drupal\auditfiles\ServiceAuditFilesusedNotManaged
   */
  protected $usedNotManaged;

  /**
   *  Class constructor.
   *
   * @param \Drupal\auditfiles\ServiceAuditFilesUsedNotManaged $used_not_managed
   *   Injected ServiceAuditFilesUsedNotManaged service.
   * @param int $file_id
   *   File entity ID to delete.
   */
  public function __construct(ServiceAuditFilesUsedNotManaged $used_not_managed, $file_id) {
    $this->usedNotManaged = $used_not_managed;
    $this->fileId = $file_id;
  }

  /**
   * The batch process for deleting the file feature 'used not managed'.
   *
   * @param int $file_id
   *   File entity ID to delete.
   * @param array $context
   *   Batch context.
   */
  public static function auditfilesUsedNotManagedBatchDeleteProcessBatch($file_id, array &$context) {
    $usedNotManaged = \Drupal::service('auditfiles.used_not_managed');
    $worker = new static($usedNotManaged, $file_id);
    $worker->dispatch($context);
  }

  /**
   * Processes the removal of files from file_managed that are not in file_usage.
   *
   * @param int $file_id
   *   File entity ID to delete.
   * @param array $context
   *   Batch context.
   */
  protected function dispatch(&$context) {
    $this->usedNotManaged->auditfilesUsedNotManagedBatchDeleteProcessFile($file_id);
    $context['results'][] = Html::escape($file_id);
    $context['message'] = new TranslatableMarkup('Processed file ID %file_id.', ['%file_id' => $file_id]);
  }

}
