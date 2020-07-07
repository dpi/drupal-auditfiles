<?php

namespace Drupal\auditfiles\Batch;

use Drupal\Component\Utility\Html;
use Drupal\Core\StringTranslation\TranslatableMarkup;
use Drupal\auditfiles\ServiceAuditFilesMergeFileReferences;

/**
 * Process batch files.
 *
 * @todo Refactor to make a Factory Worker class.
 */
class AuditFilesMergeFileReferencesBatchProcess {

  /**
   * The File entity ID to delete.
   *
   * @var int
   */
  protected $fileId;

  /**
   * The File entity IDs to merge.
   *
   * @var array
   */
  protected $fileIds;

  /**
   * ManagedNotUsed service.
   *
   * @var \Drupal\auditfiles\ServiceAuditFilesMergeFileReferences
   */
  protected $mergeFileReferences;

  /**
   *  Class constructor.
   *
   * @param \Drupal\auditfiles\ServiceAuditFilesMergeFileReferences $merge_file_references
   *   Injected ServiceAuditFilesManagedNotUsed service.
   * @param int $file_id
   *   File entity ID to delete.
   * @param array $files_being_merged
   *   File IDs to merge.
   */
  public function __construct(ServiceAuditFilesMergeFileReferences $merge_file_references, $file_being_kept, array $files_being_merged) {
    $this->mergeFileReferences = $merge_file_references;
    $this->fileId = $file_being_kept;
    $this->fileIds = $files_being_merged;
  }

  /**
   * The batch process for deleting the file.
   *
   * @param int $file_being_kept
   *   The file ID of the file to merge the other into.
   * @param int $file_being_merged
   *   The file ID of the file to merge.
   * @param array $context
   *   Used by the Batch API to keep track of and pass data from one operation
   *   to the next.
   *
   * @todo Called only from ServiceAuditMergeFileReferences, refactor to make
   * a factory worker on that service.
   */
  public static function auditfilesMergeFileReferencesBatchMergeProcessBatch($file_being_kept, $file_being_merged, array &$context) {
    $mergeFileReferences = \Drupal::service('auditfiles.merge_file_references');
    $worker = new static($mergeFileReferences, $file_being_kept, $file_being_merged);
    $worker->dispatch($context);
  }

  /**
   * Processes the file IDs to delete and merge.
   *
   * @param \Drupal\auditfiles\ServiceAuditFilesMergeFileReferences $merge_file_references
   *   Injected ServiceAuditFilesManagedNotUsed service.
   * @param int $file_being_kept
   *   File entity ID to delete.
   * @param array $files_being_merged
   *   File IDs to merge.
   */
  protected function dispatch(&$context) {
    $this->mergeFileReferences->auditfilesMergeFileReferencesBatchMergeProcessFile($file_being_kept, $file_being_merged);
    $context['results'][] = Html::escape($file_being_merged);
    $context['message'] = new TranslatableMarkup(
      'Merged file ID %file_being_merged into file ID %file_being_kept.',
      [
        '%file_being_kept' => $file_being_kept,
        '%file_being_merged' => $file_being_merged,
      ]
    );
  }

}
