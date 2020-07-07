<?php

namespace Drupal\auditfiles\Batch;

use Drupal\Component\Utility\Html;
use Drupal\Core\StringTranslation\TranslatableMarkup;

/**
 * Process batch files.
 *
 * @todo Refactor to make a Factory Worker class.
 */
class AuditFilesUsedNotManagedBatchProcess {

  /**
   * The batch process for deleting the file feature 'used not managed'.
   *
   * @todo Called only from ServiceAuditFilesUsedNotManaged, refactor to make a
   * factory worker on that service.
   */
  public static function auditfilesUsedNotManagedBatchDeleteProcessBatch($file_id, array &$context) {
    \Drupal::service('auditfiles.used_not_managed')->auditfilesUsedNotManagedBatchDeleteProcessFile($file_id);
    $context['results'][] = $file_id;
    $context['message'] = new TranslatableMarkup('Processed file ID %file_id.', ['%file_id' => $file_id]);
  }

}
