<?php

namespace Drupal\auditfiles\Batch;

use Drupal\Component\Utility\Html;
use Drupal\Core\StringTranslation\TranslatableMarkup;
use Drupal\auditfiles\ServiceAuditFilesReferencedNotUsed;

/**
 * Process batch files.
 */
class AuditFilesReferencedNotUsedBatchProcess {

  /**
   * The entity reference ID to delete.
   *
   * @var int
   */
  protected $referenceId;

  /**
   * ReferencedNotUsed service.
   *
   * @var \Drupal\auditfiles\ServiceAuditFilesReferencedNotUsed
   */
  protected $referencedNotUsed;

  /**
   *  Class constructor.
   *
   * @param \Drupal\auditfiles\ServiceAuditFilesReferencedNotUsed $referenced_not_used
   *   Injected ServiceAuditFilesUsedNotManaged service.
   * @param int $file_id
   *   File entity ID to delete.
   */
  public function __construct(ServiceAuditFilesReferencedNotUsed $referenced_not_used, $reference_id) {
    $this->referencedNotUsed = $referenced_not_used;
    $this->referenceId = $reference_id;
  }

  /**
   * Batch Process for Adding a file reference.
   *
   * @param int $reference_id
   *   File entity reference ID to add.
   * @param array $context
   *   Batch context.
   */
  public static function auditfilesReferencedNotUsedBatchAddProcessBatch($reference_id, array &$context) {
    $referencedNotUsed = \Drupal::service('auditfiles.referenced_not_used');
    $worker = new static($referencedNotUsed, $reference_id);
    $worker->addDispatch($context);
  }

  /**
   * Processes entity reference additions from content entities to file_managed.
   *
   * @param int $reference_id
   *   File entity ID to add.
   * @param array $context
   *   Batch context.
   */
  protected function addDispatch(&$context) {
    $this->referencedNotUsed->auditfilesReferencedNotUsedBatchAddProcessFile($reference_id);
    $context['results'][] = Html::escape($reference_id);
    $context['message'] = new TranslatableMarkup('Processed file ID %file_id.', ['%file_id' => $reference_id]);
  }

  /**
   * Batch Process for Deleting a file reference.
   *
   * @param int $reference_id
   *   File entity reference ID to delete.
   * @param array $context
   *   Batch context.
   */
  public static function auditfilesReferencedNotUsedBatchDeleteProcessBatch($reference_id, array &$context) {
    $referencedNotUsed = \Drupal::service('auditfiles.referenced_not_used');
    $worker = new static($referencedNotUsed, $reference_id);
    $worker->deleteDispatch($context);
  }

  /**
   * Processes entity reference deletions from content entities to file_managed.
   *
   * @param int $reference_id
   *   File entity ID to delete.
   * @param array $context
   *   Batch context.
   */
  protected function deleteDispatch(&$context) {
    $this->referencedNotUsed->auditfilesReferencedNotUsedBatchDeleteProcessFile($reference_id);
    $context['results'][] = Html::escape($reference_id);
    $context['message'] = new TranslatableMarkup('Processed file ID %file_id.', ['%file_id' => $reference_id]);
  }

}