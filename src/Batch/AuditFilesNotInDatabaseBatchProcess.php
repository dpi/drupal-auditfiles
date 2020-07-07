<?php

namespace Drupal\auditfiles\Batch;

use Drupal\Component\Utility\Html;
use Drupal\Core\StringTranslation\TranslatableMarkup;

/**
 * Process batch files.
 *
 * @todo Refactor to make a Factory Worker class.
 */
class AuditFilesNotInDatabaseBatchProcess {

  /**
   * The batch process for adding the file.
   *
   * @param string $filename
   *   File name that to be process.
   * @param array $context
   *   Used by the Batch API to keep track of data and pass it from one
   *   operation to the next.
   *
   * @todo Called only from ServiceAuditFilesNotInDatabase, refactor to make a
   * factory worker on that service.
   */
  public static function auditfilesNotInDatabaseBatchAddProcessBatch($filename, array &$context) {
    \Drupal::service('auditfiles.not_in_database')->auditfilesNotInDatabaseBatchAddProcessFile($filename);
    $context['results'][] = Html::escape($filename);
    $context['message'] = new TranslatableMarkup('Processed %filename.', ['%filename' => $filename]);
  }

  /**
   * The batch process for deleting the file.
   *
   * @param string $filename
   *   File name that to be process.
   * @param array $context
   *   Used by the Batch API to keep track of data and pass it from one
   *   operation to the next.
   *
   * @todo Called only from ServiceAuditFilesNotInDatabase, refactor to make a
   * factory worker on that service.
   */
  public static function auditfilesNotInDatabaseBatchDeleteProcessBatch($filename, array &$context) {
    \Drupal::service('auditfiles.not_in_database')->auditfilesNotInDatabaseBatchDeleteProcessFile($filename);
    $context['results'][] = Html::escape($filename);
    $context['message'] = new TranslatableMarkup('Processed %filename.', ['%filename' => $filename]);
  }

}
