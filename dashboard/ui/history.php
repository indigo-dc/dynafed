<h2 class="sub-header">History</h2>

<p class="lead">Showing last <?php echo count($history); ?> polls.</p>

<?php
    foreach($currentStatus->plugins as $plugin) {
?>
      <p class="lead"><?php echo $plugin->name ?></p>
      <div class="progress">
<?php
      $availabilityStats = new AvailabilityStats($history, $plugin->name);
      foreach($availabilityStats->chunks as $chunk) {
        $percentage = $chunk->normalized * 100;
        $color="success";
        if($chunk->status->state != 1) {
          $color="danger";
        }
?>
          <div data-toggle="tooltip" title="<?php echo "From " . gmdate("h:i:s Y-m-d", $chunk->start) . " to " . gmdate("h:i:s Y-m-d", $chunk->end) ?>" class="progress-bar progress-bar-<?php echo $color ?>" style="width: <?php echo $percentage ?>%;"></div>
<?php
      }
?>
      </div>

<?php
    }
?>
