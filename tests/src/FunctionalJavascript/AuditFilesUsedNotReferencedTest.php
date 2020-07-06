<?php

namespace Drupal\Tests\auditfiles\FunctionalJavascript;

use Drupal\FunctionalJavascriptTests\WebDriverTestBase;
use Drupal\user\RoleInterface;
use Drupal\Core\Url;
use Drupal\file\Entity\File;
use Drupal\node\Entity\Node;

/**
 * Tests that the "Managed not used" report is reachable with no errors.
 *
 * @group auditfiles
 */
class AuditFilesUsedNotReferencedTest extends WebDriverTestBase {


  /**
   * {@inheritdoc}
   */
  protected $profile = 'standard';

  /**
   * {@inheritdoc}
   */
  protected static $modules = ['auditfiles'];

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

    // Array of data for file_usage, files_managed, and entity node creation.
    $values = [
      ['file', 'node', 1, 1],
      ['file', 'node', 2, 1],
      ['file', 'node', 3, 1],
    ];

    foreach ($values as $key => $value) {
      // Create file_usage entry.
      \Drupal::database()->insert('file_usage')->fields([
        'fid' => $key + 1,
        'module' => $value[0],
        'type' => $value[1],
        'id' => $value[2],
        'count' => $value[3],
      ])->execute();

      // Create file_managed entry.
      $fileno = $key + 1;
      $path = "public://example_$fileno.png";
      $image = File::create([
        'uri' => $path,
        'status' => TRUE,
      ]);
      $image->save();

      $node = Node::create([
        'type'        => 'article',
        'title'       => 'Sample Node',
        'field_image' => [
          'target_id' => $key + 1,
          'alt' => 'Sample',
          'title' => 'Sample File',
        ],
      ]);
      $node->save();
    }

  }

  /**
   * Tests report page returns correct HTTP response code.
   *
   * 403 for anonymous users (also for users without permission).
   * 200 for authenticated user with 'access audit files reports' perm.
   */
  public function testReportPage() {
    // Form to test.
    $path = URL::fromRoute('auditfiles.audit_files_usednotreferenced');
    // Establish session.
    $session = $this->assertSession();
    // Visit page as anonymous user, should get Access Denied message.
    $this->drupalGet($path);
    $session->pageTextContains('Access denied');
    // Log in as admin user.
    $this->drupalLogin($this->user);
    // Test that report page returns the report page.
    $this->drupalGet($path);
    $session->pageTextContains('Used not referenced');
  }

  /**
   * Tests that an orphan file can be deleted.
   *
   * An "orphan" file entity is one with an entry in the
   * file_managed table that has no corresponding file in the
   * file_usage table.
   */
  public function testFileEntityCanBeDeleted() {
    // Delete file_usage entry.
    \Drupal::database()->query("DELETE FROM {node__field_image} WHERE field_image_target_id='1'")->execute();
    \Drupal::database()->query("DELETE FROM {node_revision__field_image} WHERE field_image_target_id='1'")->execute();
    // Form to test.
    $path = URL::fromRoute('auditfiles.audit_files_usednotreferenced');
    // Establish session.
    $session = $this->assertSession();
    // Log in as admin user.
    $this->drupalLogin($this->user);
    // Load the report page.
    $this->drupalGet($path);
    // Check for the report title.
    $session->pageTextContains("Used not referenced");
    // Check boxes for file IDs to delete from database, and delete.
    $edit = [
      'edit-files-1' => TRUE,
    ];
    $this->submitForm($edit, 'Delete selected items from the file_usage table');
    // Check for correct confirmation page and submit.
    $session->pageTextContains("Delete these items from the file_usage table?");
    $edit = [];
    $this->submitForm($edit, 'Confirm');
    // Check that target file is no longer listed.
    $session->pageTextContains("Used not referenced");
    $session->pageTextContains("Sucessfully deleted File ID : 1 from the file_usage table.");
  }

}
