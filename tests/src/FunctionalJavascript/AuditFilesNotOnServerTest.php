<?php

namespace Drupal\Tests\auditfiles\FunctionalJavascript;

use Drupal\FunctionalJavascriptTests\WebDriverTestBase;
use Drupal\user\RoleInterface;
use Drupal\Core\Url;
use Drupal\file\Entity\File;

/**
 * Tests that the "Not on server" report is reachable with no errors.
 *
 * @group auditfiles
 */
class AuditFilesNotOnServerTest extends WebDriverTestBase {

  /**
   * {@inheritdoc}
   */
  protected static $modules = ['node', 'field', 'file', 'user', 'auditfiles'];

  /**
   * User with admin privileges.
   *
   * @var \Drupal\user\UserInterface
   */
  protected $user;

  /**
   * User's role ID.
   *
   * @var string
   */
  protected $rid;

  /**
   * {@inheritdoc}
   */
  protected $defaultTheme = 'stark';

  /**
   * {@inheritdoc}
   */
  protected function setUp() {
    parent::setUp();
    // Create user with permissions to manage site configuration and access
    // audit files reports.
    $this->user = $this->drupalCreateUser(['access audit files reports']);
    $all_rids = $this->user->getRoles();
    unset($all_rids[array_search(RoleInterface::AUTHENTICATED_ID, $all_rids)]);
    // Save role IDs.
    $this->rid = reset($all_rids);
    // Create File Entities.
    for ($i = 0; $i < 3; $i++) {
      $path = "public://example_$i.png";
      $image = File::create([
        'uri' => $path,
        'status' => TRUE,
      ]);
      $image->save();
    }
  }

  /**
   * Tests report page returns correct HTTP response code.
   */
  public function testReportPage() {
    // Form to test.
    $path = URL::fromRoute('auditfiles.audit_files_notonserver');
    // Establish session.
    $session = $this->assertSession();
    // Visit page as anonymous user, should get Access Denied message.
    $this->drupalGet($path);
    $session->pageTextContains('Access denied');
    // Log in as admin user.
    $this->drupalLogin($this->user);
    // Test that report page returns the report page.
    $this->drupalGet($path);
    $session->pageTextContains('Not on server');
  }

  /**
   * Tests that an orphan file can be deleted.
   *
   * An "orphan" file is one in the file system that has no corresponding record
   * in the database.
   */
  public function testFileEntityCanBeDeleted() {
    // Form to test.
    $path = URL::fromRoute('auditfiles.audit_files_notonserver');
    // Establish session.
    $session = $this->assertSession();
    // Log in as admin user.
    $this->drupalLogin($this->user);
    // Load the report page.
    $this->drupalGet($path);
    // Check for the report title.
    $session->pageTextContains("Not on server");
    // Check box for file ID to delete from database, and delete.
    $edit = [
      'edit-files-1' => TRUE,
    ];
    $this->submitForm($edit, 'Delete selected items from the database');
    // Check for correct confirmation page and submit.
    $session->pageTextContains("Delete these items from the database?");
    $edit = [];
    $this->submitForm($edit, 'Confirm');
    // Check that target file is no longer listed.
    $session->waitForElementVisible('css', '#audit-files-not-on-server');
    $session->pageTextContains("Not on server");
    $session->pageTextContains("Sucessfully deleted File ID : 1 from the file_managed table.");
  }

}
