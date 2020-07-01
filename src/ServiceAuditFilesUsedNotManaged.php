<?php

namespace Drupal\auditfiles;

use Drupal\Core\Link;
use Drupal\Core\Url;
use Drupal\Core\StringTranslation\TranslationInterface;
use Drupal\Core\StringTranslation\StringTranslationTrait;
use Drupal\Core\Messenger\MessengerTrait;
use Drupal\Core\Config\ConfigFactory;
use Drupal\Core\Database\Connection;

/**
 * Form for Files used not managed functionality.
 */
class ServiceAuditFilesUsedNotManaged {

  use MessengerTrait;

  /**
   * The Translation service.
   *
   * @var \Drupal\Core\StringTranslation\TranslationInterface
   */
  protected $stringTranslation;

  /**
   * The Configuration Factory.
   *
   * @var \Drupal\Core\Config\ConfigFactory
   */
  protected $configFactory;

  /**
   * The database connection.
   *
   * @var \Drupal\Core\Database\Connection
   */
  protected $connection;

  /**
   * Define constructor for string translation.
   *
   * @param \Drupal\Core\StringTranslation\TranslationInterface $translation
   *   A Tranlation Serevice object.
   * @param \Drupal\Core\Config\ConfigFactory $config_factory
   *   A configuration factory object.
   * @param \Drupal\Core\Database\Connection $connection
   *   A database connection for queries.
   */
  public function __construct(TranslationInterface $translation, ConfigFactory $config_factory, Connection $connection) {
    $this->stringTranslation = $translation;
    $this->configFactory = $config_factory;
    $this->connection = $connection;
  }

  /**
   * Retrieves the file IDs to operate on.
   *
   * @return array
   *   The file IDs.
   */
  public function auditfilesUsedNotManagedGetFileList() {
    // Get all the file IDs in the file_usage table that are not in the
    // file_managed table.
    $connection = $this->connection;
    $config = $this->configFactory->get('auditfiles.settings');
    $query = 'SELECT DISTINCT fid FROM {file_usage} fu WHERE fid NOT IN (SELECT fid FROM {file_managed})';
    $maximum_records = $config->get('auditfiles_report_options_maximum_records') ? $config->get('auditfiles_report_options_maximum_records') : 250;

    if ($maximum_records > 0) {
      $query .= ' LIMIT ' . $maximum_records;
    }
    return $connection->query($query)->fetchCol();
  }

  /**
   * Retrieves information about an individual file from the database.
   */
  public function auditfilesUsedNotManagedGetFileData($file_id) {
    // Get the file information for the specified file ID from the database.
    $connection = $this->connection;
    $query = 'SELECT * FROM {file_usage} WHERE fid = ' . $file_id;
    $file = $connection->query($query)->fetchObject();

    $url = Url::fromUri('internal:/' . $file->type . '/' . $file->id);
    $result = Link::fromTextAndUrl($file->type . '/' . $file->id, $url)->toString();
    return [
      'fid' => $file->fid,
      'module' => $file->module . ' ' . $this->stringTranslation->translate('module'),
      'id' => $result,
      'count' => $file->count,
    ];
  }

  /**
   * Returns the header to use for the display table.
   */
  public function auditfilesUsedNotManagedGetHeader() {
    return [
      'fid' => [
        'data' => $this->stringTranslation->translate('File ID'),
      ],
      'module' => [
        'data' => $this->stringTranslation->translate('Used by'),
      ],
      'id' => [
        'data' => $this->stringTranslation->translate('Used in'),
      ],
      'count' => [
        'data' => $this->stringTranslation->translate('Count'),
      ],
    ];
  }

  /**
   * Creates the batch for deleting files from the file_usage table.
   */
  public function auditfilesUsedNotManagedBatchDeleteCreateBatch(array $fileids) {
    $batch['error_message'] = $this->stringTranslation->translate('One or more errors were encountered processing the files.');
    $batch['finished'] = '\Drupal\auditfiles\AuditFilesBatchProcess::auditfilesUsedNotManagedBatchFinishBatch';
    $batch['progress_message'] = $this->stringTranslation->translate('Completed @current of @total operations.');
    $batch['title'] = $this->stringTranslation->translate('Deleting files from the file_usage table');
    $operations = $file_ids = [];
    foreach ($fileids as $file_id) {
      if ($file_id != 0) {
        $file_ids[] = $file_id;
      }
    }
    // Fill in the $operations variable.
    foreach ($file_ids as $file_id) {
      $operations[] = [
        '\Drupal\auditfiles\AuditFilesBatchProcess::auditfilesUsedNotManagedBatchDeleteProcessBatch',
        [$file_id],
      ];
    }
    $batch['operations'] = $operations;
    return $batch;
  }

  /**
   * Deletes the specified file from the file_usage table.
   *
   * @param int $file_id
   *   The ID of the file to delete from the database.
   */
  public function auditfilesUsedNotManagedBatchDeleteProcessFile($file_id) {
    $connection = $this->connection;
    $num_rows = $connection->delete('file_usage')->condition('fid', $file_id)->execute();
    if (empty($num_rows)) {
      $this->messenger()->addWarning(
        $this->stringTranslation->translate(
          'There was a problem deleting the record with file ID %fid from the file_usage table. Check the logs for more information.',
          ['%fid' => $file_id]
        )
      );
    }
    else {
      $this->messenger()->addStatus(
        $this->stringTranslation->translate(
          'Sucessfully deleted File ID : %fid from the file_usages table.',
          ['%fid' => $file_id]
        )
      );
    }
  }

}
