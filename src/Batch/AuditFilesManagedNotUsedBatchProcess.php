<?php

namespace Drupal\auditfiles\Batch;

use Drupal\Component\Utility\Html;
use Drupal\Core\StringTranslation\TranslatableMarkup;
use Drupal\auditfiles\ServiceAuditFilesManagedNotUsed;

/**
 * Batch Worker to handle Deleting entity records from file_managed table.
 *
 * @todo Refactor to make a Factory Worker class.
 */
class AuditFilesManagedNotUsedBatchProcess {

  /**
   * The File entity ID to delete.
   *
   * @var int
   */
  protected $fileId;

  /**
   * ManagedNotUsed service.
   *
   * @var \Drupal\auditfiles\ServiceAuditFilesManagedNotUsed
   */
  protected $managedNotUsed;

  /**
   *  Class constructor.
   *
   * @param \Drupal\auditfiles\ServiceAuditFilesManagedNotUsed $managed_not_used
   *   Injected ServiceAuditFilesManagedNotUsed service.
   * @param int $file_id
   *   File entity ID to delete.
   */
  public function __construct(ServiceAuditFilesManagedNotUsed $managed_not_used, $file_id) {
    $this->managedNotUsed = $managed_not_used;
    $this->fileId = $file_id;
  }

  /**
   * Batch process to delete file entities from file_managed not in file_usage.
   *
   * Used by the Batch API to keep track of and pass data from one operation to
   * the next.
   *
   * @param int $file_id
   *   File entity ID to delete.
   * @param array $context
   *   Batch context.
   *
   * @todo Called only from ServiceAuditFilesManagedNotUsed, refactor to make a
   * factory worker on that service.
   */
  public static function auditfilesManagedNotUsedBatchDeleteProcessBatch($file_id, array &$context) {
    $managedNotUsed = \Drupal::service('auditfiles.managed_not_used');
    $worker = new static($managedNotUsed, $file_id);
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
    $this->managedNotUsed->auditfilesManagedNotUsedBatchDeleteProcessFile($file_id);
    $context['results'][] = Html::escape($file_id);
    $context['message'] = new TranslatableMarkup('Processed file ID %file_id.', ['%file_id' => $file_id]);
  }

}
