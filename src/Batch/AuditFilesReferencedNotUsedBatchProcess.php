<?php

namespace Drupal\auditfiles\Batch;

use Drupal\Component\Utility\Html;
use Drupal\Core\StringTranslation\TranslatableMarkup;

/**
 * Process batch files.
 *
 * @todo Refactor to make a Factory Worker class.
 */
class AuditFilesReferencedNotUsedBatchProcess {

  /**
   * Used by the Batch API to keep track of and pass data from one operation.
   *
   * @todo Called only from ServiceAuditFilesReferencedNotUsed, refactor to make
   * a factory worker on that service.
   */
  public static function auditfilesReferencedNotUsedBatchAddProcessBatch($reference_id, array &$context) {
    \Drupal::service('auditfiles.referenced_not_used')->auditfilesReferencedNotUsedBatchAddProcessFile($reference_id);
    $context['results'][] = $reference_id;
    $context['message'] = new TranslatableMarkup('Processed reference ID %file_id.', ['%file_id' => $reference_id]);
  }

  /**
   * Used by the Batch API to keep track of and pass data from one operation.
   *
   * @todo Called only from ServiceAuditFilesReferencedNotUsed, refactor to make
   * a factory worker on that service.
   */
  public static function auditfilesReferencedNotUsedBatchDeleteProcessBatch($reference_id, array &$context) {
    \Drupal::service('auditfiles.referenced_not_used')->auditfilesReferencedNotUsedBatchDeleteProcessFile($reference_id);
    $context['results'][] = $reference_id;
    $context['message'] = new TranslatableMarkup('Processed reference ID %file_id.', ['%file_id' => $reference_id]);
  }

}
