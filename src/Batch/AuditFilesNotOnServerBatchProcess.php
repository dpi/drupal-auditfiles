<?php

namespace Drupal\auditfiles\Batch;

use Drupal\Component\Utility\Html;
use Drupal\Core\StringTranslation\TranslatableMarkup;

/**
 * Process batch files.
 *
 * @todo Refactor to make a Factory Worker class.
 */
class AuditFilesNotOnServerBatchProcess {

  /**
   * The batch process for deleting the file.
   *
   * Used by the Batch API to keep track of and pass data from one operation to
   * the next.
   *
   * @todo Called only from ServiceAuditFilesNotOnServer, refactor to make a
   * factory worker on that service.
   */
  public static function auditfilesNotOnServerBatchDeleteProcessBatch($file_id, array &$context) {
    \Drupal::service('auditfiles.not_on_server')->auditfilesNotOnServerBatchDeleteProcessFile($file_id);
    $context['results'][] = Html::escape($file_id);
    $context['message'] = new TranslatableMarkup('Processed file ID %file_id.', ['%file_id' => $file_id]);
  }

}
