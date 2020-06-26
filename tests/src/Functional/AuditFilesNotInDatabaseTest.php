<?php

namespace Drupal\Tests\auditfiles\Functional;

use Drupal\Tests\BrowserTestBase;
use Drupal\user\RoleInterface;
use Drupal\Core\Url;
use Drupal\Core\StringTranslation\StringTranslationTrait;
use Drupal\Tests\TestFileCreationTrait;
use Drupal\Tests\UiHelperTrait;

/**
 * Tests that the "Not in Database" report is reachable with no errors.
 *
 * @group auditfiles
 */
class AuditFilesNotInDatabaseTest extends BrowserTestBase {

  use TestFileCreationTrait;
  use UiHelperTrait;
  use StringTranslationTrait;

  /**
   * {@inheritdoc}
   */
  protected static $modules = ['node', 'file', 'user', 'auditfiles'];

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
  protected function setUp(): void {
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
   *
   * 403 for anonymous users (also for users without permission).
   * 200 for authenticated user with 'administer site configuration' perm.
   */
  public function testReportPage() {
    // Form to test.
    $path = URL::fromRoute('auditfiles.notindatabase');
    // Establish session.
    $session = $this->assertSession();
    // Visit page as anonymous user, should receive a 403.
    $this->drupalGet($path);
    $session->statusCodeEquals(403);
    // Log in as admin user.
    $this->drupalLogin($this->user);
    // Test that report page returns a 200 response code.
    $this->drupalGet($path);
    $session->statusCodeEquals(200);
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
    $session->pageTextContains($this->t("Not in database"));
    // Check that the report table is not empty.
    $session->elementNotContains('css', '#edit-files', $this->t('No items found'));
    $session->pageTextNotContains($this->t("Found no files on the server that are not in the database"));
    // Check that at least 36 files were found.
    $session->elementContains('xpath', '//*[@id="notindatabase"]/div[1]/em', $this->t("Found at least 36 files on the server that are not in the database"));
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
    $session->pageTextContains($this->t("Not in database"));
    // Check box for file to delete from server, and submit form.
    $edit = [
      'edit-files-html-2html' => TRUE,
    ];
    $this->submitForm($edit, $this->t('Delete selected items from the server'));
    // Check for correct confirmation page and submit.
    $session->pageTextContains($this->t("Delete these files from the server?"));
    $edit = [];
    $this->submitForm($edit, $this->t('Confirm'));
    // Check that target file is no longer listed.
    $session->pageTextContains($this->t("Not in database"));
    $session->pageTextContains($this->t("Sucessfully deleted html-2.html from the server."));
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
    $session->pageTextContains($this->t("Not in database"));
    // Check box for file to add to database, and submit form.
    $edit = [
      'edit-files-image-1png' => TRUE,
    ];
    $this->submitForm($edit, $this->t('Add selected items to the database'));
    // Check for correct confirmation page and submit.
    $session->pageTextContains($this->t("Add these files to the database?"));
    $edit = [];
    $this->submitForm($edit, $this->t('Confirm'));
    // Check that target file is no longer listed.
    $session->pageTextContains($this->t("Not in database"));
    $session->pageTextContains($this->t("Sucessfully added image-1.png to the database."));
  }

}
