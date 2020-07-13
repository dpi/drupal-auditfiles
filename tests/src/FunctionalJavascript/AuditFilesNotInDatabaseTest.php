<?php

namespace Drupal\Tests\auditfiles\FunctionalJavascript;

use Drupal\FunctionalJavascriptTests\WebDriverTestBase;
use Drupal\user\RoleInterface;
use Drupal\Core\Url;
use Drupal\Tests\TestFileCreationTrait;

/**
 * Tests that the "Not in Database" report is reachable with no errors.
 *
 * @group auditfiles
 */
class AuditFilesNotInDatabaseTest extends WebDriverTestBase {

  use TestFileCreationTrait;

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
    // Create physical files.
    // Possible values: 'binary', 'html', 'image', 'javascript', 'php', 'sql',
    // 'text'.
    $this->getTestFiles('binary');
    $this->getTestFiles('html');
    $this->getTestFiles('image');
    $this->getTestFiles('javascript');
    $this->getTestFiles('php');
    $this->getTestFiles('sql');
    $this->getTestFiles('text');
  }

  /**
   * Tests report page returns correct HTTP response code.
   */
  public function testReportPage() {
    // Form to test.
    $path = URL::fromRoute('auditfiles.notindatabase');
    // Establish session.
    $session = $this->assertSession();
    // Visit page as anonymous user, should get Access Denied message.
    $this->drupalGet($path);
    $session->pageTextContains('Access denied');
    // Log in as admin user.
    $this->drupalLogin($this->user);
    // Test that report page returns the report page.
    $this->drupalGet($path);
    $session->pageTextContains('Not in database');
  }

  /**
   * Tests that orphaned files display on the report.
   *
   * An "orphan" file is one in the file system that has no corresponding record
   * in the database.
   */
  public function testFileNotInDatabase() {
    // Form to test.
    $path = URL::fromRoute('auditfiles.notindatabase');
    // Establish session.
    $session = $this->assertSession();
    // Log in as admin user.
    $this->drupalLogin($this->user);
    // Load the report page.
    $this->drupalGet($path);
    // Check for the report title.
    $session->pageTextContains("Not in database");
    // Check that the report table is not empty.
    $session->elementNotContains('css', '#edit-files', 'No items found');
    $session->pageTextNotContains("Found no files on the server that are not in the database");
    // Check that at least 36 files were found.
    $session->elementContains('xpath', '//*[@id="notindatabase"]/div[1]/em', "Found at least 36 files on the server that are not in the database");
  }

  /**
   * Tests that an orphan file can be deleted.
   *
   * An "orphan" file is one in the file system that has no corresponding record
   * in the database.
   */
  public function testFileCanBeDeleted() {
    // Form to test.
    $path = URL::fromRoute('auditfiles.notindatabase');
    // Establish session.
    $session = $this->assertSession();
    // Log in as admin user.
    $this->drupalLogin($this->user);
    // Load the report page.
    $this->drupalGet($path);
    // Check for the report title.
    $session->pageTextContains("Not in database");
    // Check box for file to delete from server, and submit form.
    $edit = [
      'edit-files-html-2html' => TRUE,
    ];
    $this->submitForm($edit, 'Delete selected items from the server');
    // Check for correct confirmation page and submit.
    $session->pageTextContains("Delete these files from the server?");
    $edit = [];
    $this->submitForm($edit, 'Confirm');
    // Check that target file is no longer listed.
    $session->waitForElementVisible('css', '#notindatabase');
    $session->pageTextContains("Not in database");
    $session->pageTextContains("Sucessfully deleted html-2.html from the server.");
  }

  /**
   * Tests that orphan file system files can be added to the database.
   *
   * An "orphan" file is one in the file system that has no corresponding record
   * in the database.
   */
  public function testFileCanBeAdded() {
    // Form to test.
    $path = URL::fromRoute('auditfiles.notindatabase');
    // Establish session.
    $session = $this->assertSession();
    // Log in as admin user.
    $this->drupalLogin($this->user);
    // Load the report page.
    $this->drupalGet($path);
    // Check for the report title.
    $session->pageTextContains("Not in database");
    // Check box for file to add to database, and submit form.
    $edit = [
      'edit-files-image-1png' => TRUE,
    ];
    $this->submitForm($edit, 'Add selected items to the database');
    // Check for correct confirmation page and submit.
    $session->pageTextContains("Add these files to the database?");
    $edit = [];
    $this->submitForm($edit, 'Confirm');
    // Check that target file is no longer listed.
    $session->waitForElementVisible('css', '#notindatabase');
    $session->pageTextContains("Not in database");
    $session->pageTextContains("Sucessfully added image-1.png to the database.");
  }

}
