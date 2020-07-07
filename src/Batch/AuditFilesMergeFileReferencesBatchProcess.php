<?php

namespace Drupal\auditfiles\Batch;

use Drupal\Component\Utility\Html;
use Drupal\Core\StringTranslation\TranslatableMarkup;

/**
 * Process batch files.
 *
 * @todo Refactor to make a Factory Worker class.
 */
class AuditFilesMergeFileReferencesBatchProcess {

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
    \Drupal::service('auditfiles.merge_file_references')->auditfilesMergeFileReferencesBatchMergeProcessFile($file_being_kept, $file_being_merged);
    $context['results'][] = $file_being_merged;
    $context['message'] = new TranslatableMarkup(
      'Merged file ID %file_being_merged into file ID %file_being_kept.',
      [
        '%file_being_kept' => $file_being_kept,
        '%file_being_merged' => $file_being_merged,
      ]
    );
  }

}
